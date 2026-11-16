
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

    vector<Commit::FileSnapshot> getWorkingFiles(
        const string& directoryPath
    );

    string getRelativePath(const string& fullPath) const;

    void initializeRepository();

    void addFile(const string& name);
    void addAllFiles();
    void resetFile(const string& name);

    void createCommit(const string& message);

    void showStatus();
    void showLog();

public:
    CommandHandler(
        FileSystem& fileSystem,
        Repository& repository
    );

    void execute(const string& command);
};

#endif
