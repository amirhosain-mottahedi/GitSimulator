#include "Repository.h"
#include <stdexcept>

Repository::Repository() {
    userName = "";
    userEmail = "";
    initialized = false;
    rootPath = "";
    currentBranch = "";
    headCommitId = 0;
    nextCommitId = 1;
}

void Repository::setUserName(const string& name) { userName = name; }
void Repository::setUserEmail(const string& email) { userEmail = email; }
string Repository::getUserName() const { return userName; }
string Repository::getUserEmail() const { return userEmail; }
bool Repository::isInitialized() const { return initialized; }

void Repository::initialize(const string& path) {
    if (initialized) throw runtime_error("Repository already initialized.");
    initialized = true;
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

string Repository::getRootPath() const { return rootPath; }
string Repository::getCurrentBranch() const { return currentBranch; }
int Repository::getHeadCommitId() const { return headCommitId; }
int Repository::getNextCommitId() const { return nextCommitId; }

const vector<string> Repository::getBranchNames() const {
    vector<string> names;
    for (int i = 0; i < static_cast<int>(branches.size()); i++) names.push_back(branches[i].name);
    return names;
}

bool Repository::branchExists(const string& name) const {
    for (int i = 0; i < static_cast<int>(branches.size()); i++)
        if (branches[i].name == name) return true;
    return false;
}

bool Repository::createBranch(const string& name) {
    if (branchExists(name)) return false;
    Branch newBranch;
    newBranch.name = name;
    newBranch.headCommitId = headCommitId;
    branches.push_back(newBranch);
    return true;
}

bool Repository::deleteBranch(const string& name) {
    if (name == currentBranch) return false;
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
            return true;
        }
    }
    return false;
}

const vector<Commit::FileSnapshot>& Repository::getStaging() const { return staging; }
void Repository::setStaging(const vector<Commit::FileSnapshot>& files) { staging = files; }

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
    for (int i = 0; i < static_cast<int>(branches.size()); i++) {
        if (branches[i].name == currentBranch) {
            branches[i].headCommitId = headCommitId;
            break;
        }
    }
}

const vector<Commit>& Repository::getCommits() const { return commits; }

const Commit* Repository::findCommit(int id) const {
    for (int i = 0; i < static_cast<int>(commits.size()); i++)
        if (commits[i].getId() == id) return &commits[i];
    return nullptr;
}
