
#ifndef REPOSITORY_H
#define REPOSITORY_H

#include <string>
#include <vector>

#include "../commit/Commit.h"

using namespace std;

class Repository {
private:
    string userName;
    string userEmail;

    bool initialized;

    string rootPath;
    string currentBranch;

    int headCommitId;
    int nextCommitId;

    vector<Commit> commits;

    vector<Commit::FileSnapshot> staging;

public:
    Repository();

    // User configuration
    void setUserName(const string& name);
    void setUserEmail(const string& email);

    string getUserName() const;
    string getUserEmail() const;

    // Repository information
    bool isInitialized() const;
    void initialize(const string& path);

    string getRootPath() const;
    string getCurrentBranch() const;

    int getHeadCommitId() const;
    int getNextCommitId() const;

    // Staging
    const vector<Commit::FileSnapshot>& getStaging() const;

    void setStaging(
        const vector<Commit::FileSnapshot>& files
    );

    void addToStaging(
        const Commit::FileSnapshot& file
    );

    void removeFromStaging(const string& path);

    // Commit management
    void addCommit(const Commit& commit);

    const vector<Commit>& getCommits() const;

    const Commit* findCommit(int id) const;
};

#endif
