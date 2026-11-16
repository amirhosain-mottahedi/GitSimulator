
#ifndef SHELL_H
#define SHELL_H

#include <iostream>
#include <string>

#include "../filesystem/FileSystem.h"
#include "../repository/Repository.h"
#include "../commands/CommandHandler.h"

using namespace std;

class Shell {
private:
    FileSystem& fs;
    Repository& repo;
    CommandHandler gitHandler;

public:
    Shell(FileSystem& fileSystem, Repository& repository);

    bool executeLine(const string& line);
};

#endif
