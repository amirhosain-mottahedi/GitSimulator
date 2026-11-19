
#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <string>
#include <vector>

#include "../filesystem/FileSystem.h"
#include "../repository/Repository.h"

using namespace std;

class CommandHandler {
private:
    FileSystem& fs;
    Repository& repo;

    bool mergeInProgress;
    int mergeParentId;
    string mergeBranchName;
    vector<string> unresolvedConflicts;

    vector<Commit::FileSnapshot> getWorkingFiles(
        const string& directoryPath
    );

    string getRelativePath(const string& fullPath) const;

    void initializeRepository();

    void addFile(const string& name);
    void addAllFiles();
    void resetFile(const string& name);

    void createCommit(const string& message);
    void reset(const string& mode, int commitId);

    void showStatus();
    void showLog();
    void showBranches() const;
    void showDiff(const string& filename);

    bool hasUncommittedChanges();

    void restoreWorkingTree(
        const vector<Commit::FileSnapshot>& snapshot
    );

    void checkoutBranch(const string& name);

    void mergeBranch(const string& branchName);

    int findCommonAncestor(
        int firstCommitId,
        int secondCommitId
    ) const;

    const Commit::FileSnapshot* findFile(
        const vector<Commit::FileSnapshot>& files,
        const string& path
    ) const;

    void removeResolvedConflict(const string& path);

public:
    CommandHandler(
        FileSystem& fileSystem,
        Repository& repository
    );

    void execute(const string& command);
};

#endif
