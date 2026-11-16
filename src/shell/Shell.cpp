
#include "Shell.h"
#include <sstream>

// Constructor
Shell::Shell(FileSystem& fileSystem) : fs(fileSystem) {
}

// Execute command
bool Shell::executeLine(const string& line) {
    if (line.empty()) {
        return true;
    }

    string command;
    string arguments;

    stringstream input(line);

    input >> command;
    getline(input, arguments);

    // Remove spaces from beginning of arguments
    size_t start = arguments.find_first_not_of(" \t");

    if (start == string::npos) {
        arguments = "";
    }
    else {
        arguments = arguments.substr(start);
    }

    // pwd
    if (command == "pwd") {
        fs.pwd();
    }

    // ls
    else if (command == "ls") {
        fs.ls();
    }

    // mkdir
    else if (command == "mkdir") {
        if (arguments.empty()) {
            cout << "Usage: mkdir <name>" << endl;
        }
        else {
            fs.mkdir(arguments);
        }
    }

    // cd
    else if (command == "cd") {
        if (arguments.empty()) {
            fs.cd("/");
        }
        else {
            fs.cd(arguments);
        }
    }

    // touch
    else if (command == "touch") {
        if (arguments.empty()) {
            cout << "Usage: touch <filename>" << endl;
        }
        else {
            fs.touch(arguments);
        }
    }

    // cat
    else if (command == "cat") {
        if (arguments.empty()) {
            cout << "Usage: cat <filename>" << endl;
        }
        else {
            fs.cat(arguments);
        }
    }

    // rm
    else if (command == "rm") {
        if (arguments.empty()) {
            cout << "Usage: rm <name>" << endl;
        }
        else {
            fs.rm(arguments);
        }
    }

    // echo
    else if (command == "echo") {
        size_t overwritePosition = arguments.find(">");
        size_t appendPosition = arguments.find(">>");

        if (overwritePosition == string::npos) {
            cout << arguments << endl;
        }
        else {
            bool append = (appendPosition != string::npos);

            size_t operatorPosition = append
                ? appendPosition
                : overwritePosition;

            size_t operatorLength = append ? 2 : 1;

            string text = arguments.substr(0, operatorPosition);
            string filename = arguments.substr(
                operatorPosition + operatorLength
            );

            // Trim spaces from filename
            size_t filenameStart =
                filename.find_first_not_of(" \t");

            if (filenameStart == string::npos) {
                cout << "Usage: echo text > filename" << endl;
                return true;
            }

            filename = filename.substr(filenameStart);

            size_t filenameEnd =
                filename.find_last_not_of(" \t");

            filename = filename.substr(0, filenameEnd + 1);

            // Trim spaces from text
            size_t textStart = text.find_first_not_of(" \t");

            if (textStart == string::npos) {
                text = "";
            }
            else {
                text = text.substr(textStart);
            }

            size_t textEnd = text.find_last_not_of(" \t");

            if (textEnd != string::npos) {
                text = text.substr(0, textEnd + 1);
            }

            fs.writeFile(filename, text, append);
        }
    }

    // exit
    else if (command == "exit") {
        return false;
    }

    // Unknown command
    else {
        cout << "Command not found: " << command << endl;
    }

    return true;
}
