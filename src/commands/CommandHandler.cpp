
#include "CommandHandler.h"

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace std;

CommandHandler::CommandHandler(
    FileSystem& fileSystem,
    Repository& repository
) : fs(fileSystem),
    repo(repository),
    mergeInProgress(false),
    mergeParentId(-1),
    mergeBranchName("") {
}

const Commit::FileSnapshot* CommandHandler::findFile(
    const vector<Commit::FileSnapshot>& files,
    const string& path
) const {
    for (const auto& file : files) {
        if (file.path == path) {
            return &file;
        }
    }

    return nullptr;
}

void CommandHandler::removeResolvedConflict(const string& path) {
    auto it = find(
        unresolvedConflicts.begin(),
        unresolvedConflicts.end(),
        path
    );

    if (it != unresolvedConflicts.end()) {
        unresolvedConflicts.erase(it);
    }
}

int CommandHandler::findCommonAncestor(
    int firstCommitId,
    int secondCommitId
) const {
    vector<pair<int, int>> firstAncestors;
    vector<pair<int, int>> secondAncestors;

    vector<pair<int, int>> pending;
    pending.push_back({firstCommitId, 0});

    for (size_t i = 0; i < pending.size(); i++) {
        int id = pending[i].first;
        int distance = pending[i].second;

        bool visited = false;

        for (const auto& item : firstAncestors) {
            if (item.first == id) {
                visited = true;
                break;
            }
        }

        if (visited) {
            continue;
        }

        firstAncestors.push_back({id, distance});

        const Commit* commit = repo.findCommit(id);

        if (commit == nullptr) {
            continue;
        }

        if (commit->getParentId() != -1) {
            pending.push_back({
                commit->getParentId(),
                distance + 1
            });
        }

        if (commit->getSecondParentId() != -1) {
            pending.push_back({
                commit->getSecondParentId(),
                distance + 1
            });
        }
    }

    pending.clear();
    pending.push_back({secondCommitId, 0});

    for (size_t i = 0; i < pending.size(); i++) {
        int id = pending[i].first;
        int distance = pending[i].second;

        bool visited = false;

        for (const auto& item : secondAncestors) {
            if (item.first == id) {
                visited = true;
                break;
            }
        }

        if (visited) {
            continue;
        }

        secondAncestors.push_back({id, distance});

        const Commit* commit = repo.findCommit(id);

        if (commit == nullptr) {
            continue;
        }

        if (commit->getParentId() != -1) {
            pending.push_back({
                commit->getParentId(),
                distance + 1
            });
        }

        if (commit->getSecondParentId() != -1) {
            pending.push_back({
                commit->getSecondParentId(),
                distance + 1
            });
        }
    }

    int commonAncestor = -1;
    int bestDistance = -1;

    for (const auto& first : firstAncestors) {
        for (const auto& second : secondAncestors) {
            if (first.first == second.first) {
                int totalDistance = first.second + second.second;

                if (bestDistance == -1 ||
                    totalDistance < bestDistance) {
                    bestDistance = totalDistance;
                    commonAncestor = first.first;
                }
            }
        }
    }

    return commonAncestor;
}

string CommandHandler::getRelativePath(
    const string& fullPath
) const {
    string root = repo.getRootPath();

    if (root == "/") {
        if (!fullPath.empty() && fullPath[0] == '/') {
            return fullPath.substr(1);
        }

        return fullPath;
    }

    if (fullPath == root) {
        return "";
    }

    if (fullPath.find(root + "/") == 0) {
        return fullPath.substr(root.size() + 1);
    }

    throw runtime_error("Path is outside repository.");
}

vector<Commit::FileSnapshot> CommandHandler::getWorkingFiles(
    const string& directoryPath
) {
    vector<Commit::FileSnapshot> result;

    vector<FileSystem::FileData> files =
        fs.getAllFiles(directoryPath);

    string prefix = getRelativePath(directoryPath);

    for (int i = 0; i < static_cast<int>(files.size()); i++) {
        Commit::FileSnapshot file;

        if (prefix.empty()) {
            file.path = files[i].path;
        }
        else if (files[i].path.empty()) {
            file.path = prefix;
        }
        else {
            file.path = prefix + "/" + files[i].path;
        }

        file.content = files[i].content;

        result.push_back(file);
    }

    sort(
        result.begin(),
        result.end(),
        [](const Commit::FileSnapshot& a,
           const Commit::FileSnapshot& b) {
            return a.path < b.path;
        }
    );

    return result;
}

void CommandHandler::initializeRepository() {
    repo.initialize(fs.getCurrentPath());

    cout << "Initialized empty Git repository." << endl;
}

void CommandHandler::addFile(const string& name) {
    if (name.empty() ||
        name.find('/') != string::npos ||
        name.find(' ') != string::npos) {
        throw invalid_argument("Invalid filename.");
    }

    string currentPath = fs.getCurrentPath();
    string fullPath;

    if (currentPath == "/") {
        fullPath = "/" + name;
    }
    else {
        fullPath = currentPath + "/" + name;
    }

    string relativePath = getRelativePath(fullPath);

    vector<string> content;

    if (!fs.readFile(fullPath, content)) {
        bool tracked = false;

        const vector<Commit::FileSnapshot>& staged =
            repo.getStaging();

        for (int i = 0; i < static_cast<int>(staged.size()); i++) {
            if (staged[i].path == relativePath) {
                tracked = true;
                break;
            }
        }

        if (repo.getHeadCommitId() != 0) {
            const Commit* head =
                repo.findCommit(repo.getHeadCommitId());

            if (head != nullptr) {
                const vector<Commit::FileSnapshot>& previous =
                    head->getSnapshot();

                for (int i = 0;
                     i < static_cast<int>(previous.size());
                     i++) {
                    if (previous[i].path == relativePath) {
                        tracked = true;
                        break;
                    }
                }
            }
        }

        if (!tracked) {
            throw runtime_error("File not found.");
        }

        repo.removeFromStaging(relativePath);
        removeResolvedConflict(relativePath);

        cout << "File deletion added to staging." << endl;
        return;
    }

    Commit::FileSnapshot file;
    file.path = relativePath;
    file.content = content;

    repo.addToStaging(file);

    bool hasConflictMarkers = false;

    for (const string& line : content) {
        if (line.find("<<<<<<<") != string::npos ||
            line.find("=======") != string::npos ||
            line.find(">>>>>>>") != string::npos) {
            hasConflictMarkers = true;
            break;
        }
    }

    if (!hasConflictMarkers) {
        removeResolvedConflict(relativePath);
    }

    cout << "File added to staging." << endl;
}

void CommandHandler::addAllFiles() {
    string currentPath = fs.getCurrentPath();

    vector<Commit::FileSnapshot> working =
        getWorkingFiles(currentPath);

    string prefix = getRelativePath(currentPath);

    for (int i = 0; i < static_cast<int>(working.size()); i++) {
        repo.addToStaging(working[i]);
    }

    vector<Commit::FileSnapshot> staged = repo.getStaging();

    for (int i = 0; i < static_cast<int>(staged.size()); i++) {
        string path = staged[i].path;

        bool insideDirectory = prefix.empty() ||
            path == prefix ||
            path.find(prefix + "/") == 0;

        if (!insideDirectory) {
            continue;
        }

        bool exists = false;

        for (int j = 0; j < static_cast<int>(working.size()); j++) {
            if (working[j].path == path) {
                exists = true;
                break;
            }
        }

        if (!exists) {
            repo.removeFromStaging(path);
        }
    }

    cout << "All changes added to staging." << endl;
}

void CommandHandler::resetFile(const string& name) {
    if (name.empty() ||
        name.find('/') != string::npos ||
        name.find(' ') != string::npos) {
        throw invalid_argument("Invalid filename.");
    }

    string currentPath = fs.getCurrentPath();
    string fullPath;

    if (currentPath == "/") {
        fullPath = "/" + name;
    }
    else {
        fullPath = currentPath + "/" + name;
    }

    string relativePath = getRelativePath(fullPath);

    const Commit* head =
        repo.findCommit(repo.getHeadCommitId());

    if (head != nullptr) {
        const vector<Commit::FileSnapshot>& previous =
            head->getSnapshot();

        for (int i = 0; i < static_cast<int>(previous.size()); i++) {
            if (previous[i].path == relativePath) {
                repo.addToStaging(previous[i]);

                cout << "Staging updated." << endl;
                return;
            }
        }
    }

    repo.removeFromStaging(relativePath);

    cout << "Staging updated." << endl;
}

void CommandHandler::createCommit(const string& message) {
    if (repo.getUserName().empty() ||
        repo.getUserEmail().empty()) {
        throw runtime_error(
            "Please configure user.name and user.email first."
        );
    }

    if (message.empty()) {
        throw invalid_argument("Commit message cannot be empty.");
    }

    if (mergeInProgress && !unresolvedConflicts.empty()) {
        throw runtime_error(
            "Cannot commit: unresolved merge conflicts remain."
        );
    }

    const vector<Commit::FileSnapshot>& staged =
        repo.getStaging();

    const Commit* previous =
        repo.findCommit(repo.getHeadCommitId());

    bool sameAsPrevious = true;

    if (previous == nullptr) {
        sameAsPrevious = staged.empty();
    }
    else {
        const vector<Commit::FileSnapshot>& oldFiles =
            previous->getSnapshot();

        if (oldFiles.size() != staged.size()) {
            sameAsPrevious = false;
        }
        else {
            for (int i = 0; i < static_cast<int>(oldFiles.size()); i++) {
                bool found = false;

                for (int j = 0; j < static_cast<int>(staged.size()); j++) {
                    if (oldFiles[i].path == staged[j].path &&
                        oldFiles[i].content == staged[j].content) {
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    sameAsPrevious = false;
                    break;
                }
            }
        }
    }

    if (sameAsPrevious && !mergeInProgress) {
        vector<Commit::FileSnapshot> working =
            getWorkingFiles(repo.getRootPath());

        bool sameAsWorking = true;

        if (working.size() != staged.size()) {
            sameAsWorking = false;
        }
        else {
            for (int i = 0; i < static_cast<int>(working.size()); i++) {
                bool found = false;

                for (int j = 0; j < static_cast<int>(staged.size()); j++) {
                    if (working[i].path == staged[j].path &&
                        working[i].content == staged[j].content) {
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    sameAsWorking = false;
                    break;
                }
            }
        }

        if (sameAsWorking) {
            cout << "nothing to commit, working tree clean" << endl;
        }
        else {
            cout << "no changes added to commit" << endl;
        }

        return;
    }

    time_t now = time(nullptr);
    tm* localTime = localtime(&now);

    if (localTime == nullptr) {
        throw runtime_error("Could not get current date.");
    }

    stringstream dateStream;
    dateStream << put_time(localTime, "%Y-%m-%d %H:%M:%S");

    int id = repo.getNextCommitId();
    int parentId = repo.getHeadCommitId();

    int secondParentId = -1;

    if (mergeInProgress) {
        secondParentId = mergeParentId;
    }

    Commit commit(
        id,
        parentId,
        message,
        repo.getUserName(),
        repo.getUserEmail(),
        dateStream.str(),
        staged,
        secondParentId
    );

    repo.addCommit(commit);

    cout << "[" << repo.getCurrentBranch()
         << " " << id << "] "
         << message << endl;

    if (mergeInProgress) {
        cout << "Merge commit created successfully." << endl;

        mergeInProgress = false;
        mergeParentId = -1;
        mergeBranchName = "";
        unresolvedConflicts.clear();
    }
}

void CommandHandler::reset(const string& mode, int commitId) {
    const Commit* targetCommit = repo.findCommit(commitId);

    if (targetCommit == nullptr) {
        throw runtime_error("Commit not found.");
    }

    if (mode != "--soft" &&
        mode != "--mixed" &&
        mode != "--hard") {
        throw invalid_argument("Invalid reset mode.");
    }

    vector<Commit::FileSnapshot> targetSnapshot =
        targetCommit->getSnapshot();

    if (mode == "--hard") {
        restoreWorkingTree(targetSnapshot);
        repo.setStaging(targetSnapshot);
    }
    else if (mode == "--mixed") {
        repo.setStaging(targetSnapshot);
    }

    repo.resetHead(commitId);

    cout << "HEAD is now at "
         << commitId << " ("
         << targetCommit->getMessage() << ")"
         << endl;

    if (mode == "--soft") {
        cout << "Soft reset completed." << endl;
    }
    else if (mode == "--mixed") {
        cout << "Mixed reset completed." << endl;
    }
    else {
        cout << "Hard reset completed." << endl;
    }
}

void CommandHandler::showStatus() {
    cout << "On branch "
         << repo.getCurrentBranch()
         << endl;

    const Commit* head =
        repo.findCommit(repo.getHeadCommitId());

    vector<Commit::FileSnapshot> previous;

    if (head != nullptr) {
        previous = head->getSnapshot();
    }

    const vector<Commit::FileSnapshot>& staged =
        repo.getStaging();

    vector<Commit::FileSnapshot> working =
        getWorkingFiles(repo.getRootPath());

    vector<string> stagedMessages;
    vector<string> modifiedMessages;
    vector<string> untrackedMessages;

    for (int i = 0; i < static_cast<int>(staged.size()); i++) {
        bool found = false;

        for (int j = 0; j < static_cast<int>(previous.size()); j++) {
            if (staged[i].path == previous[j].path) {
                found = true;

                if (staged[i].content != previous[j].content) {
                    stagedMessages.push_back(
                        "  modified: " + staged[i].path
                    );
                }

                break;
            }
        }

        if (!found) {
            stagedMessages.push_back(
                "  new file: " + staged[i].path
            );
        }
    }

    for (int i = 0; i < static_cast<int>(previous.size()); i++) {
        bool found = false;

        for (int j = 0; j < static_cast<int>(staged.size()); j++) {
            if (previous[i].path == staged[j].path) {
                found = true;
                break;
            }
        }

        if (!found) {
            stagedMessages.push_back(
                "  deleted: " + previous[i].path
            );
        }
    }

    for (int i = 0; i < static_cast<int>(staged.size()); i++) {
        bool found = false;

        for (int j = 0; j < static_cast<int>(working.size()); j++) {
            if (staged[i].path == working[j].path) {
                found = true;

                if (staged[i].content != working[j].content) {
                    modifiedMessages.push_back(
                        "  modified: " + staged[i].path
                    );
                }

                break;
            }
        }

        if (!found) {
            modifiedMessages.push_back(
                "  deleted: " + staged[i].path
            );
        }
    }

    for (int i = 0; i < static_cast<int>(working.size()); i++) {
        bool found = false;

        for (int j = 0; j < static_cast<int>(staged.size()); j++) {
            if (working[i].path == staged[j].path) {
                found = true;
                break;
            }
        }

        if (!found) {
            untrackedMessages.push_back(
                "  " + working[i].path
            );
        }
    }

    sort(stagedMessages.begin(), stagedMessages.end());
    sort(modifiedMessages.begin(), modifiedMessages.end());
    sort(untrackedMessages.begin(), untrackedMessages.end());

    if (!stagedMessages.empty()) {
        cout << "\nStaged changes:" << endl;

        for (int i = 0; i < static_cast<int>(stagedMessages.size()); i++) {
            cout << stagedMessages[i] << endl;
        }
    }

    if (!modifiedMessages.empty()) {
        cout << "\nModified (not staged):" << endl;

        for (int i = 0; i < static_cast<int>(modifiedMessages.size()); i++) {
            cout << modifiedMessages[i] << endl;
        }
    }

    if (!untrackedMessages.empty()) {
        cout << "\nUntracked files:" << endl;

        for (int i = 0; i < static_cast<int>(untrackedMessages.size()); i++) {
            cout << untrackedMessages[i] << endl;
        }
    }

    if (stagedMessages.empty() &&
        modifiedMessages.empty() &&
        untrackedMessages.empty()) {
        cout << "\nnothing to commit, working tree clean" << endl;
    }
}

void CommandHandler::showBranches() const {
    vector<string> branches = repo.getBranchNames();

    for (int i = 0; i < static_cast<int>(branches.size()); i++) {
        if (branches[i] == repo.getCurrentBranch()) {
            cout << "* ";
        }
        else {
            cout << "  ";
        }

        cout << branches[i] << endl;
    }
}

bool CommandHandler::hasUncommittedChanges() {
    vector<Commit::FileSnapshot> working =
        getWorkingFiles(repo.getRootPath());

    const Commit* head = repo.findCommit(repo.getHeadCommitId());
    vector<Commit::FileSnapshot> committed;

    if (head != nullptr) {
        committed = head->getSnapshot();
    }

    const vector<Commit::FileSnapshot>& staged = repo.getStaging();

    if (working.size() != committed.size() ||
        staged.size() != committed.size()) {
        return true;
    }

    for (int i = 0; i < static_cast<int>(committed.size()); i++) {
        bool foundWorking = false;
        bool foundStaged = false;

        for (int j = 0; j < static_cast<int>(working.size()); j++) {
            if (working[j].path == committed[i].path &&
                working[j].content == committed[i].content) {
                foundWorking = true;
                break;
            }
        }

        for (int j = 0; j < static_cast<int>(staged.size()); j++) {
            if (staged[j].path == committed[i].path &&
                staged[j].content == committed[i].content) {
                foundStaged = true;
                break;
            }
        }

        if (!foundWorking || !foundStaged) {
            return true;
        }
    }

    return false;
}

void CommandHandler::restoreWorkingTree(
    const vector<Commit::FileSnapshot>& snapshot
) {
    vector<FileSystem::FileData> current =
        fs.getAllFiles(repo.getRootPath());

    for (int i = 0; i < static_cast<int>(current.size()); i++) {
        string fullPath = repo.getRootPath();

        if (fullPath == "/") {
            fullPath += current[i].path;
        }
        else {
            fullPath += "/" + current[i].path;
        }

        if (!fs.deleteFile(fullPath)) {
            throw runtime_error("Could not remove a working file.");
        }
    }

    for (int i = 0; i < static_cast<int>(snapshot.size()); i++) {
        string fullPath = repo.getRootPath();

        if (fullPath == "/") {
            fullPath += snapshot[i].path;
        }
        else {
            fullPath += "/" + snapshot[i].path;
        }

        if (!fs.createFile(fullPath, snapshot[i].content)) {
            throw runtime_error(
                "Could not restore file: " + snapshot[i].path
            );
        }
    }
}


void CommandHandler::checkoutBranch(const string& name) {
    bool isBranch = repo.branchExists(name);
    int targetHeadId = -1;

    if (isBranch) {
        if (name == repo.getCurrentBranch()) {
            cout << "Already on branch " << name << "." << endl;
            return;
        }

        targetHeadId = repo.getBranchHeadCommitId(name);
    }
    else {
        try {
            size_t position;
            targetHeadId = stoi(name, &position);

            if (position != name.size() || targetHeadId <= 0) {
                throw runtime_error("Invalid commit ID.");
            }
        }
        catch (...) {
            throw runtime_error("Branch or commit not found.");
        }

        if (repo.findCommit(targetHeadId) == nullptr) {
            throw runtime_error("Commit not found.");
        }
    }

    if (hasUncommittedChanges()) {
        throw runtime_error(
            "You have uncommitted changes. Commit or clean them before switching."
        );
    }

    if (targetHeadId < 0) {
        throw runtime_error("Could not find target information.");
    }

    vector<Commit::FileSnapshot> targetSnapshot;

    if (targetHeadId != 0) {
        const Commit* targetCommit = repo.findCommit(targetHeadId);

        if (targetCommit == nullptr) {
            throw runtime_error("Target commit not found.");
        }

        targetSnapshot = targetCommit->getSnapshot();
    }

    restoreWorkingTree(targetSnapshot);

    if (!repo.checkoutBranch(name)) {
        throw runtime_error("Could not switch to target.");
    }

    repo.setStaging(targetSnapshot);

    if (isBranch) {
        cout << "Switched to branch '" << name << "'." << endl;
    }
    else {
        cout << "HEAD is now detached at commit " << name << "." << endl;
    }
}

void CommandHandler::showDiff(const string& filename) {
    if (filename.empty() ||
        filename.find('/') != string::npos ||
        filename.find(' ') != string::npos) {
        throw invalid_argument("Invalid filename.");
    }

    string currentPath = fs.getCurrentPath();
    string fullPath;

    if (currentPath == "/") {
        fullPath = "/" + filename;
    }
    else {
        fullPath = currentPath + "/" + filename;
    }

    string relativePath = getRelativePath(fullPath);

    const vector<Commit::FileSnapshot>& staged = repo.getStaging();
    const Commit::FileSnapshot* stagedFile = nullptr;

    for (int i = 0; i < static_cast<int>(staged.size()); i++) {
        if (staged[i].path == relativePath) {
            stagedFile = &staged[i];
            break;
        }
    }

    if (stagedFile == nullptr) {
        throw runtime_error("File not found in staging.");
    }

    vector<string> workingContent;

    if (!fs.readFile(fullPath, workingContent)) {
        workingContent.clear();
    }

    const vector<string>& stagedContent = stagedFile->content;

    int stagedSize = static_cast<int>(stagedContent.size());
    int workingSize = static_cast<int>(workingContent.size());

    vector<vector<int>> lcs(
        stagedSize + 1,
        vector<int>(workingSize + 1, 0)
    );

    for (int i = stagedSize - 1; i >= 0; i--) {
        for (int j = workingSize - 1; j >= 0; j--) {
            if (stagedContent[i] == workingContent[j]) {
                lcs[i][j] = 1 + lcs[i + 1][j + 1];
            }
            else {
                lcs[i][j] = max(
                    lcs[i + 1][j],
                    lcs[i][j + 1]
                );
            }
        }
    }

    if (stagedContent == workingContent) {
        cout << "No differences found." << endl;
        return;
    }

    cout << "diff -- " << relativePath << endl;

    int i = 0;
    int j = 0;

    while (i < stagedSize && j < workingSize) {
        if (stagedContent[i] == workingContent[j]) {
            i++;
            j++;
        }
        else if (lcs[i + 1][j] >= lcs[i][j + 1]) {
            cout << "- " << stagedContent[i] << endl;
            i++;
        }
        else {
            cout << "+ " << workingContent[j] << endl;
            j++;
        }
    }

    while (i < stagedSize) {
        cout << "- " << stagedContent[i] << endl;
        i++;
    }

    while (j < workingSize) {
        cout << "+ " << workingContent[j] << endl;
        j++;
    }
}

void CommandHandler::showLog() {
    if (repo.getHeadCommitId() == 0) {
        throw runtime_error("No commits yet.");
    }

    int currentId = repo.getHeadCommitId();

    while (currentId != 0) {
        const Commit* commit = repo.findCommit(currentId);

        if (commit == nullptr) {
            break;
        }

        cout << "commit " << commit->getId();

        if (commit->getSecondParentId() != -1) {
            cout << " (Merge Commit)";
        }

        cout << endl;

        cout << "Author: "
             << commit->getAuthorName()
             << " <"
             << commit->getAuthorEmail()
             << ">"
             << endl;

        cout << "Date:   "
             << commit->getDate()
             << endl;

        if (commit->getSecondParentId() != -1) {
            cout << "Merge:  "
                 << commit->getParentId()
                 << " "
                 << commit->getSecondParentId()
                 << endl;
        }

        cout << endl;
        cout << "    " << commit->getMessage() << endl;
        cout << endl;

        currentId = commit->getParentId();
    }
}

void CommandHandler::mergeBranch(const string& branchName) {
    if (!repo.isInitialized()) {
        throw runtime_error("Repository is not initialized.");
    }

    if (!repo.branchExists(branchName)) {
        throw runtime_error("Branch not found.");
    }

    if (branchName == repo.getCurrentBranch()) {
        throw runtime_error("Cannot merge a branch into itself.");
    }

    if (hasUncommittedChanges()) {
        throw runtime_error(
            "You have uncommitted changes. Commit or reset them first."
        );
    }

    int currentHead = repo.getHeadCommitId();
    int targetHead = repo.getBranchHeadCommitId(branchName);

    if (currentHead == targetHead) {
        cout << "Already up to date." << endl;
        return;
    }

    const Commit* currentCommit = repo.findCommit(currentHead);
    const Commit* targetCommit = repo.findCommit(targetHead);

    vector<Commit::FileSnapshot> ours;
    vector<Commit::FileSnapshot> theirs;
    vector<Commit::FileSnapshot> base;

    if (currentCommit != nullptr) {
        ours = currentCommit->getSnapshot();
    }

    if (targetCommit != nullptr) {
        theirs = targetCommit->getSnapshot();
    }

int commonAncestor = findCommonAncestor(
    currentHead,
    targetHead
);

if (commonAncestor == targetHead) {
    mergeInProgress = false;
    mergeParentId = -1;
    mergeBranchName = "";
    unresolvedConflicts.clear();

    cout << "Already up to date." << endl;
    return;
}

if (commonAncestor == currentHead &&
    currentHead != targetHead) {

    restoreWorkingTree(theirs);

    repo.setStaging(theirs);

    if (!repo.fastForwardCurrentBranch(targetHead)) {
        throw runtime_error("Fast-forward failed.");
    }

    mergeInProgress = false;
    mergeParentId = -1;
    mergeBranchName = "";
    unresolvedConflicts.clear();

    cout << "Fast-forward completed successfully." << endl;
    return;
}

const Commit* baseCommit = repo.findCommit(commonAncestor);

    if (baseCommit != nullptr) {
        base = baseCommit->getSnapshot();
    }

    vector<string> allPaths;

    auto addPaths = [&](const vector<Commit::FileSnapshot>& files) {
        for (const auto& file : files) {
            if (find(allPaths.begin(), allPaths.end(), file.path)
                == allPaths.end()) {
                allPaths.push_back(file.path);
            }
        }
    };

    addPaths(base);
    addPaths(ours);
    addPaths(theirs);

    vector<Commit::FileSnapshot> mergedFiles;
    unresolvedConflicts.clear();

    auto sameFile = [](
        const Commit::FileSnapshot* first,
        const Commit::FileSnapshot* second
    ) {
        if (first == nullptr && second == nullptr) {
            return true;
        }

        if (first == nullptr || second == nullptr) {
            return false;
        }

        return first->content == second->content;
    };

    for (const string& path : allPaths) {
        const auto* baseFile = findFile(base, path);
        const auto* ourFile = findFile(ours, path);
        const auto* theirFile = findFile(theirs, path);

        const Commit::FileSnapshot* selectedFile = nullptr;

        if (sameFile(ourFile, theirFile)) {
            selectedFile = ourFile;
        }
        else if (sameFile(ourFile, baseFile)) {
            selectedFile = theirFile;
        }
        else if (sameFile(theirFile, baseFile)) {
            selectedFile = ourFile;
        }
        else {
            Commit::FileSnapshot conflictFile;
            conflictFile.path = path;

            conflictFile.content.push_back("<<<<<<< HEAD");

            if (ourFile != nullptr) {
                for (const string& line : ourFile->content) {
                    conflictFile.content.push_back(line);
                }
            }

            conflictFile.content.push_back("=======");

            if (theirFile != nullptr) {
                for (const string& line : theirFile->content) {
                    conflictFile.content.push_back(line);
                }
            }

            conflictFile.content.push_back(
                ">>>>>>> " + branchName
            );

            mergedFiles.push_back(conflictFile);
            unresolvedConflicts.push_back(path);
            continue;
        }

        if (selectedFile != nullptr) {
            mergedFiles.push_back(*selectedFile);
        }
    }

    restoreWorkingTree(mergedFiles);
    repo.setStaging(mergedFiles);

    mergeInProgress = true;
    mergeParentId = targetHead;
    mergeBranchName = branchName;

    if (unresolvedConflicts.empty()) {
        cout << "Merge completed without conflicts." << endl;
        cout << "Merged files are staged." << endl;
    }
    else {
        cout << "Merge has conflicts!" << endl;

        for (const string& path : unresolvedConflicts) {
            cout << "CONFLICT: " << path << endl;
        }

        cout << "Resolve conflicts, then use git add." << endl;
    }
}

void CommandHandler::execute(const string& command) {
    istringstream input(command);

    string git;
    string operation;

    input >> git >> operation;

    if (git != "git" || operation.empty()) {
        throw invalid_argument("Invalid Git command.");
    }

    if (operation == "config") {
        string key;

        if (!(input >> key)) {
            throw invalid_argument("Invalid config command.");
        }

        string value;
        getline(input, value);

        size_t first = value.find_first_not_of(" \t");
        size_t last = value.find_last_not_of(" \t");

        if (first == string::npos) {
            if (key == "user.name") {
                cout << repo.getUserName() << endl;
            }
            else if (key == "user.email") {
                cout << repo.getUserEmail() << endl;
            }
            else {
                throw invalid_argument("Unknown config key.");
            }

            return;
        }

        value = value.substr(first, last - first + 1);

        if (value.size() >= 2 &&
            value.front() == '"' &&
            value.back() == '"') {
            value = value.substr(1, value.size() - 2);
        }

        if (value.empty()) {
            throw invalid_argument("Config value cannot be empty.");
        }

        if (key == "user.name") {
            repo.setUserName(value);
            cout << "Username saved successfully." << endl;
        }
        else if (key == "user.email") {
            repo.setUserEmail(value);
            cout << "Email saved successfully." << endl;
        }
        else {
            throw invalid_argument("Unknown config key.");
        }

        return;
    }

    if (operation == "init") {
        string extra;

        if (input >> extra) {
            throw invalid_argument("Usage: git init");
        }

        initializeRepository();
        return;
    }

    if (!repo.isInitialized()) {
        throw runtime_error("Not a Git repository!");
    }

    if (operation == "branch") {
        string argument;

        if (!(input >> argument)) {
            showBranches();
            return;
        }

        if (argument == "-d") {
            string branchName;
            string extra;

            if (!(input >> branchName) || (input >> extra)) {
                throw invalid_argument("Usage: git branch -d <name>");
            }

            if (!repo.branchExists(branchName)) {
                throw runtime_error("Branch not found.");
            }

            if (!repo.deleteBranch(branchName)) {
                throw runtime_error("Cannot delete the current branch.");
            }

            cout << "Deleted branch " << branchName << "." << endl;
            return;
        }

        string extra;

        if (input >> extra) {
            throw invalid_argument("Usage: git branch [<name>|-d <name>]");
        }

        if (argument[0] == '-') {
            throw invalid_argument("Usage: git branch [<name>|-d <name>]");
        }

        if (argument.find_first_not_of(
                "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_"
            ) != string::npos) {
            throw invalid_argument("Invalid branch name.");
        }

        if (!repo.createBranch(argument)) {
            throw runtime_error("Branch already exists.");
        }

        cout << "Branch " << argument << " created." << endl;
    }
    else if (operation == "checkout") {
        string branchName;
        string extra;

        if (!(input >> branchName) || (input >> extra)) {
            throw invalid_argument("Usage: git checkout <branch>");
        }

        checkoutBranch(branchName);
    }
    else if (operation == "merge") {
    string branchName;
    string extra;

    if (!(input >> branchName) || (input >> extra)) {
        throw invalid_argument("Usage: git merge <branch>");
    }

    mergeBranch(branchName);
}
else if (operation == "reset") {
    string mode;
    string commitIdText;
    string extra;

    if (!(input >> mode >> commitIdText) || (input >> extra)) {
        throw invalid_argument(
            "Usage: git reset <--soft|--mixed|--hard> <commit>"
        );
    }

    try {
        size_t position;
        int commitId = stoi(commitIdText, &position);

        if (position != commitIdText.size() || commitId <= 0) {
            throw invalid_argument("Invalid commit ID.");
        }

        reset(mode, commitId);
    }
    catch (const invalid_argument&) {
        throw invalid_argument("Invalid reset mode or commit ID.");
    }
    catch (const out_of_range&) {
        throw invalid_argument("Commit ID is out of range.");
    }
}
    else if (operation == "add") {
        string name;
        string extra;

        if (!(input >> name)) {
            throw invalid_argument("Usage: git add <filename|.>");
        }

        if (input >> extra) {
            throw invalid_argument("Too many arguments.");
        }

        if (name == ".") {
            addAllFiles();
        }
        else {
            addFile(name);
        }
    }
    else if (operation == "reset") {
        string name;
        string extra;

        if (!(input >> name) || (input >> extra)) {
            throw invalid_argument("Usage: git reset <filename>");
        }

        resetFile(name);
    }
    else if (operation == "commit") {
        string option;
        string message;

        if (!(input >> option) || option != "-m") {
            throw invalid_argument(
                "Usage: git commit -m \"message\""
            );
        }

        input >> ws;

        if (input.peek() != '"') {
            throw invalid_argument("Commit message must be in quotes.");
        }

        input.get();
        getline(input, message, '"');

        if (message.empty()) {
            throw invalid_argument("Commit message cannot be empty.");
        }

        string extra;

        if (input >> extra) {
            throw invalid_argument("Invalid commit message.");
        }

        createCommit(message);
    }
    else if (operation == "status") {
        string extra;

        if (input >> extra) {
            throw invalid_argument("Usage: git status");
        }

        showStatus();
    }
    else if (operation == "log") {
        string extra;

        if (input >> extra) {
            throw invalid_argument("Usage: git log");
        }

        showLog();
    }
        else if (operation == "diff") {
        string filename;
        string extra;

        if (!(input >> filename) || (input >> extra)) {
            throw invalid_argument("Usage: git diff <filename>");
        }

        showDiff(filename);
    }
    else {
        throw invalid_argument("Unknown Git command: " + operation);
    }
}
