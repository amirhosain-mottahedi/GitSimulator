
#ifndef SHELL_H
#define SHELL_H

#include <iostream>
#include <string>
#include "../filesystem/FileSystem.h"

using namespace std;

class Shell {
private:
    FileSystem& fs;

public:
    Shell(FileSystem& fileSystem);

    bool executeLine(const string& line);
};

#endif
