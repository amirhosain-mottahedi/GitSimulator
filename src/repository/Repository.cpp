#include "Repository.h"
#include <stdexcept>
#include "Repository.h"

// Constructor
Repository::Repository() {
    userName = "";
    userEmail = "";

    initialized = false;

    rootPath = "";
    currentBranch = "";

    headCommitId = 0;
    nextCommitId = 1;
}

// User configuration
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

// Repository information
bool Repository::isInitialized() const {
    return initialized;
}

void Repository::initialize(const string& path) {
    if (initialized) {
        throw runtime_error("Repository already initialized.");
    }

    initialized = true;
    rootPath = path;
    currentBranch = "main";

    headCommitId = 0;
    nextCommitId = 1;

    commits.clear();
    staging.clear();
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

// Staging
const vector<Commit::FileSnapshot>& Repository::getStaging() const {
    return staging;
}

void Repository::setStaging(
    const vector<Commit::FileSnapshot>& files
) {
    staging = files;
}

void Repository::addToStaging(
    const Commit::FileSnapshot& file
) {
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

// Commit management
void Repository::addCommit(const Commit& commit) {
    commits.push_back(commit);

    headCommitId = commit.getId();
    nextCommitId = headCommitId + 1;
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
