
#include <iostream>
#include "filesystem/FileSystem.h"

using namespace std;

int main() {
    FileSystem fs;

    cout << "=== Recursive Remove Test ===" << endl;

    cout << "\n1. Creating Documents:" << endl;
    fs.mkdir("Documents");

    cout << "\n2. Entering Documents:" << endl;
    fs.cd("Documents");
    fs.pwd();

    cout << "\n3. Creating a file inside Documents:" << endl;
    fs.touch("notes.txt");

    cout << "\n4. Returning to root:" << endl;
    fs.cd("/");
    fs.pwd();

    cout << "\n5. Removing Documents and its contents:" << endl;
    fs.rm("Documents");

    cout << "\n6. Final directory listing:" << endl;
    fs.ls();

    return 0;
}
