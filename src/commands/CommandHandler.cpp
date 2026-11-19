
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
) : fs(fileSystem), repo(repository) {
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

        cout << "File deletion added to staging." << endl;
        return;
    }

    Commit::FileSnapshot file;
    file.path = relativePath;
    file.content = content;

    repo.addToStaging(file);

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

    if (sameAsPrevious) {
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

    Commit commit(
        id,
        parentId,
        message,
        repo.getUserName(),
        repo.getUserEmail(),
        dateStream.str(),
        staged
    );

    repo.addCommit(commit);

    cout << "[" << repo.getCurrentBranch()
         << " " << id << "] "
         << message << endl;
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

        cout << "commit " << commit->getId() << endl;

        cout << "Author: "
             << commit->getAuthorName()
             << " <"
             << commit->getAuthorEmail()
             << ">"
             << endl;

        cout << "Date:   "
             << commit->getDate()
             << endl;

        cout << endl;
        cout << "    " << commit->getMessage() << endl;
        cout << endl;

        currentId = commit->getParentId();
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
    else {
        throw invalid_argument("Unknown Git command: " + operation);
    }
}
