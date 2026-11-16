
#include <iostream>
#include <string>

#include "filesystem/FileSystem.h"
#include "shell/Shell.h"

using namespace std;

int main() {
    FileSystem fs;
    Shell shell(fs);

    string command;

    cout << "================================" << endl;
    cout << "       GitSimulator v1.0" << endl;
    cout << "================================" << endl;

    cout << "Type 'help' for available commands." << endl;
    cout << "Type 'exit' to close the program." << endl;

    while (true) {
        cout << "\nGitSimulator> ";

        if (!getline(cin, command)) {
            break;
        }

        if (command == "help") {
            cout << "\nAvailable commands:" << endl;
            cout << "pwd       - Show current directory" << endl;
            cout << "ls        - List directory contents" << endl;
            cout << "mkdir     - Create a directory" << endl;
            cout << "cd        - Change directory" << endl;
            cout << "touch     - Create a file" << endl;
            cout << "echo      - Write text into a file" << endl;
            cout << "cat       - Display file contents" << endl;
            cout << "rm        - Remove a file or directory" << endl;
            cout << "exit      - Close the program" << endl;

            continue;
        }

        if (!shell.executeLine(command)) {
            break;
        }
    }

    cout << "\nGitSimulator closed." << endl;

    return 0;
}
