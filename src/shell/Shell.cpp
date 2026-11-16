
#include "Shell.h"

#include <sstream>
#include <stdexcept>

Shell::Shell(
    FileSystem& fileSystem,
    Repository& repository
)
    : fs(fileSystem),
      repo(repository),
      gitHandler(fileSystem, repository) {
}

bool Shell::executeLine(const string& line) {
    if (line.empty()) {
        return true;
    }

    istringstream input(line);

    string command;
    input >> command;

    if (command == "exit") {
        return false;
    }

    if (command == "pwd") {
        string extra;

        if (input >> extra) {
            throw invalid_argument("Usage: pwd");
        }

        fs.pwd();
    }
    else if (command == "ls") {
        string extra;

        if (input >> extra) {
            throw invalid_argument("Usage: ls");
        }

        fs.ls();
    }
    else if (command == "mkdir") {
        string name;
        string extra;

        if (!(input >> name) || (input >> extra)) {
            throw invalid_argument("Usage: mkdir <name>");
        }

        fs.mkdir(name);
    }
    else if (command == "cd") {
        string path;
        string extra;

        if (!(input >> path) || (input >> extra)) {
            throw invalid_argument("Usage: cd <path>");
        }

        fs.cd(path);
    }
    else if (command == "touch") {
        string name;
        string extra;

        if (!(input >> name) || (input >> extra)) {
            throw invalid_argument("Usage: touch <filename>");
        }

        fs.touch(name);
    }
    else if (command == "cat") {
        string name;
        string extra;

        if (!(input >> name) || (input >> extra)) {
            throw invalid_argument("Usage: cat <filename>");
        }

        fs.cat(name);
    }
    else if (command == "rm") {
        string name;
        string extra;

        if (!(input >> name) || (input >> extra)) {
            throw invalid_argument("Usage: rm <filename>");
        }

        fs.rm(name);
    }
    else if (command == "echo") {
        size_t redirect = line.find(">>");
        bool append = true;

        if (redirect == string::npos) {
            redirect = line.find('>');
            append = false;
        }

        if (redirect == string::npos) {
            throw invalid_argument(
                "Usage: echo \"text\" > filename"
            );
        }

        string text = line.substr(4, redirect - 4);
        string fileName = line.substr(
            redirect + (append ? 2 : 1)
        );

        size_t first = text.find('"');
        size_t last = text.find_last_of('"');

        if (first != string::npos && last > first) {
            text = text.substr(first + 1, last - first - 1);
        }

        size_t start = fileName.find_first_not_of(" \t");
        size_t end = fileName.find_last_not_of(" \t");

        if (start == string::npos) {
            throw invalid_argument("Filename is missing!");
        }

        fileName = fileName.substr(start, end - start + 1);

        if (fileName.find('/') != string::npos ||
            fileName.find(' ') != string::npos) {
            throw invalid_argument("Invalid filename!");
        }

        fs.writeFile(fileName, text, append);
    }
    else if (command == "git") {
        gitHandler.execute(line);
    }
    else {
        throw invalid_argument("Unknown command: " + command);
    }

    return true;
}
