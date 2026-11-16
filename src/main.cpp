
#include <iostream>
#include <string>
#include <exception>

#include "filesystem/FileSystem.h"
#include "shell/Shell.h"
#include "repository/Repository.h"

using namespace std;

int main() {
    FileSystem fs;
    Repository repository;

    Shell shell(fs, repository);

    string command;

    cout << "================================" << endl;
    cout << "       GitSimulator v1.0" << endl;
    cout << "================================" << endl;

    cout << "Type 'help' for available commands." << endl;
    cout << "Type 'exit' to close the program." << endl;

    while (true) {
        cout << "\n$ ";

        if (!getline(cin, command)) {
            break;
        }

        try {
            if (command == "help") {
                cout << "\nAvailable commands:" << endl;
                cout << "pwd, ls, mkdir, cd" << endl;
                cout << "touch, echo, cat, rm" << endl;
                cout << "git config, git init" << endl;
                cout << "git add, git reset" << endl;
                cout << "git commit, git status, git log" << endl;
                cout << "exit" << endl;

                continue;
            }

            if (!shell.executeLine(command)) {
                break;
            }
        }
        catch (const exception& error) {
            cout << "Error: " << error.what() << endl;
        }
    }

    cout << "\nGitSimulator closed." << endl;

    return 0;
}
