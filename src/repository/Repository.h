#ifndef REPOSITORY_H
#define REPOSITORY_H

#include <string>
#include <vector>

#include "../commit/Commit.h"

using namespace std;

class Repository {
private:
    struct Branch {
        string name;
        int headCommitId;
    };

    string userName;
    string userEmail;
    bool initialized;
    bool detachedHead;
    string rootPath;
    string currentBranch;
    int headCommitId;
    int nextCommitId;
    vector<Commit> commits;
    vector<Branch> branches;
    vector<Commit::FileSnapshot> staging;

public:
    Repository();

    void setUserName(const string& name);
    void setUserEmail(const string& email);
    string getUserName() const;
    string getUserEmail() const;

    bool isInitialized() const;
    bool isDetachedHead() const;
    void initialize(const string& path);
    string getRootPath() const;
    string getCurrentBranch() const;
    int getHeadCommitId() const;
    int getNextCommitId() const;

    const vector<string> getBranchNames() const;
    bool branchExists(const string& name) const;
    bool createBranch(const string& name);
    bool deleteBranch(const string& name);
    bool checkoutBranch(const string& name);
    bool fastForwardCurrentBranch(int commitId);
    int getBranchHeadCommitId(const string& name) const;

    const vector<Commit::FileSnapshot>& getStaging() const;
    void setStaging(const vector<Commit::FileSnapshot>& files);
    void addToStaging(const Commit::FileSnapshot& file);
    void removeFromStaging(const string& path);

    void addCommit(const Commit& commit);
    void resetHead(int commitId);
    const vector<Commit>& getCommits() const;
    const Commit* findCommit(int id) const;
};

#endif