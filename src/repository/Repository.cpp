#include "Repository.h"
#include <stdexcept>

Repository::Repository() {
    userName = "";
    userEmail = "";
    initialized = false;
    detachedHead = false;
    rootPath = "";
    currentBranch = "";
    headCommitId = 0;
    nextCommitId = 1;
}

void Repository::setUserName(const string& name) {
    userName = name;
}

void Repository::setUserEmail(const string& email) {
    userEmail = email;
}

string Repository::getUserName() const {
    return userName;
}

string Repository::getUserEmail() const {
    return userEmail;
}

bool Repository::isInitialized() const {
    return initialized;
}

bool Repository::isDetachedHead() const {
    return detachedHead;
}

void Repository::initialize(const string& path) {
    if (initialized) {
        throw runtime_error("Repository already initialized.");
    }

    initialized = true;
    detachedHead = false;
    rootPath = path;
    currentBranch = "main";
    headCommitId = 0;
    nextCommitId = 1;

    commits.clear();
    staging.clear();
    branches.clear();

    Branch mainBranch;
    mainBranch.name = "main";
    mainBranch.headCommitId = 0;

    branches.push_back(mainBranch);
}

string Repository::getRootPath() const {
    return rootPath;
}

string Repository::getCurrentBranch() const {
    return currentBranch;
}

int Repository::getHeadCommitId() const {
    return headCommitId;
}

int Repository::getNextCommitId() const {
    return nextCommitId;
}

const vector<string> Repository::getBranchNames() const {
    vector<string> names;

    for (int i = 0; i < static_cast<int>(branches.size()); i++) {
        names.push_back(branches[i].name);
    }

    return names;
}

bool Repository::branchExists(const string& name) const {
    for (int i = 0; i < static_cast<int>(branches.size()); i++) {
        if (branches[i].name == name) {
            return true;
        }
    }

    return false;
}

bool Repository::createBranch(const string& name) {
    if (branchExists(name)) {
        return false;
    }

    Branch newBranch;
    newBranch.name = name;
    newBranch.headCommitId = headCommitId;

    branches.push_back(newBranch);

    return true;
}

bool Repository::deleteBranch(const string& name) {
    if (name == currentBranch || detachedHead) {
        return false;
    }

    for (int i = 0; i < static_cast<int>(branches.size()); i++) {
        if (branches[i].name == name) {
            branches.erase(branches.begin() + i);
            return true;
        }
    }

    return false;
}

bool Repository::checkoutBranch(const string& name) {
    for (int i = 0; i < static_cast<int>(branches.size()); i++) {
        if (branches[i].name == name) {
            currentBranch = name;
            headCommitId = branches[i].headCommitId;
            detachedHead = false;

            return true;
        }
    }

    try {
        size_t position;
        int commitId = stoi(name, &position);

        if (position != name.size() || commitId <= 0) {
            return false;
        }

        const Commit* commit = findCommit(commitId);

        if (commit == nullptr) {
            return false;
        }

        headCommitId = commitId;
        currentBranch = "HEAD detached at " + name;
        detachedHead = true;

        return true;
    }
    catch (...) {
        return false;
    }
}
bool Repository::fastForwardCurrentBranch(int commitId) {
    if (!initialized || detachedHead) {
        return false;
    }

    if (findCommit(commitId) == nullptr) {
        return false;
    }

    headCommitId = commitId;

    for (int i = 0; i < static_cast<int>(branches.size()); i++) {
        if (branches[i].name == currentBranch) {
            branches[i].headCommitId = commitId;
            return true;
        }
    }

    return false;
}

int Repository::getBranchHeadCommitId(const string& name) const {
    for (int i = 0; i < static_cast<int>(branches.size()); i++) {
        if (branches[i].name == name) {
            return branches[i].headCommitId;
        }
    }

    try {
        size_t position;
        int commitId = stoi(name, &position);

        if (position == name.size() &&
            commitId > 0 &&
            findCommit(commitId) != nullptr) {
            return commitId;
        }
    }
    catch (...) {
    }

    return -1;
}

const vector<Commit::FileSnapshot>& Repository::getStaging() const {
    return staging;
}

void Repository::setStaging(
    const vector<Commit::FileSnapshot>& files
) {
    staging = files;
}

void Repository::addToStaging(const Commit::FileSnapshot& file) {
    for (int i = 0; i < static_cast<int>(staging.size()); i++) {
        if (staging[i].path == file.path) {
            staging[i] = file;
            return;
        }
    }

    staging.push_back(file);
}

void Repository::removeFromStaging(const string& path) {
    for (int i = 0; i < static_cast<int>(staging.size()); i++) {
        if (staging[i].path == path) {
            staging.erase(staging.begin() + i);
            return;
        }
    }
}

void Repository::addCommit(const Commit& commit) {
    commits.push_back(commit);

    headCommitId = commit.getId();
    nextCommitId = headCommitId + 1;

    if (detachedHead) {
        currentBranch = "HEAD detached at " +
            to_string(headCommitId);

        return;
    }

    for (int i = 0; i < static_cast<int>(branches.size()); i++) {
        if (branches[i].name == currentBranch) {
            branches[i].headCommitId = headCommitId;
            break;
        }
    }
}

void Repository::resetHead(int commitId) {
    if (findCommit(commitId) == nullptr) {
        throw runtime_error("Commit not found.");
    }

    headCommitId = commitId;

    if (detachedHead) {
        currentBranch = "HEAD detached at " +
            to_string(headCommitId);

        return;
    }

    for (int i = 0; i < static_cast<int>(branches.size()); i++) {
        if (branches[i].name == currentBranch) {
            branches[i].headCommitId = headCommitId;
            return;
        }
    }
}

const vector<Commit>& Repository::getCommits() const {
    return commits;
}

const Commit* Repository::findCommit(int id) const {
    for (int i = 0; i < static_cast<int>(commits.size()); i++) {
        if (commits[i].getId() == id) {
            return &commits[i];
        }
    }

    return nullptr;
}