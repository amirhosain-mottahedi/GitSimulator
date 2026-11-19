
#include "Commit.h"

Commit::Commit(
    int commitId,
    int parentCommitId,
    const string& commitMessage,
    const string& name,
    const string& email,
    const string& commitDate,
    const vector<FileSnapshot>& files,
    int secondParentCommitId
) {
    id = commitId;
    parentId = parentCommitId;
    secondParentId = secondParentCommitId;

    message = commitMessage;
    authorName = name;
    authorEmail = email;
    date = commitDate;

    snapshot = files;
}

int Commit::getId() const {
    return id;
}

int Commit::getParentId() const {
    return parentId;
}

int Commit::getSecondParentId() const {
    return secondParentId;
}

string Commit::getMessage() const {
    return message;
}

string Commit::getAuthorName() const {
    return authorName;
}

string Commit::getAuthorEmail() const {
    return authorEmail;
}

string Commit::getDate() const {
    return date;
}

const vector<Commit::FileSnapshot>& Commit::getSnapshot() const {
    return snapshot;
}
