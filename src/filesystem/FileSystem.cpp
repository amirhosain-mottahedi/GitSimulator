
#include "FileSystem.h"

// Constructor
FileSystem::FileSystem() {
    root = new Node;

    root->name = "/";
    root->isDirectory = true;
    root->parent = nullptr;

    current = root;
}

// Destroy tree recursively
void FileSystem::destroyTree(Node* node) {
    if (node == nullptr) {
        return;
    }

    for (size_t i = 0; i < node->children.size(); i++) {
        destroyTree(node->children[i]);
    }

    delete node;
}

// Destructor
FileSystem::~FileSystem() {
    destroyTree(root);
}

// Find child
FileSystem::Node* FileSystem::findChild(
    Node* parent,
    const string& name
) const {
    if (parent == nullptr || !parent->isDirectory) {
        return nullptr;
    }

    for (size_t i = 0; i < parent->children.size(); i++) {
        if (parent->children[i]->name == name) {
            return parent->children[i];
        }
    }

    return nullptr;
}

// Resolve path
FileSystem::Node* FileSystem::resolvePath(const string& path) {
    if (path.empty()) {
        return current;
    }

    Node* temp = (path[0] == '/') ? root : current;

    string part;
    size_t i = (path[0] == '/') ? 1 : 0;

    while (i <= path.length()) {
        if (i == path.length() || path[i] == '/') {

            if (part == "..") {
                if (temp->parent != nullptr) {
                    temp = temp->parent;
                }
            }
            else if (part != "" && part != ".") {
                temp = findChild(temp, part);

                if (temp == nullptr) {
                    return nullptr;
                }
            }

            part = "";
        }
        else {
            part += path[i];
        }

        i++;
    }

    return temp;
}

// Create directory
void FileSystem::mkdir(const string& name) {
    if (name.empty() || name == "." || name == "..") {
        cout << "Invalid directory name!" << endl;
        return;
    }

    if (findChild(current, name) != nullptr) {
        cout << "Name already exists!" << endl;
        return;
    }

    Node* dir = new Node;

    dir->name = name;
    dir->isDirectory = true;
    dir->parent = current;

    current->children.push_back(dir);

    cout << "Directory created successfully." << endl;
}

// Change directory
void FileSystem::cd(const string& path) {
    Node* target = resolvePath(path);

    if (target == nullptr) {
        cout << "Path not found!" << endl;
        return;
    }

    if (!target->isDirectory) {
        cout << "This is not a directory!" << endl;
        return;
    }

    current = target;
}

// Create file
void FileSystem::touch(const string& name) {
    if (name.empty() || name == "." || name == "..") {
        cout << "Invalid file name!" << endl;
        return;
    }

    if (findChild(current, name) != nullptr) {
        cout << "Name already exists!" << endl;
        return;
    }

    Node* file = new Node;

    file->name = name;
    file->isDirectory = false;
    file->parent = current;

    current->children.push_back(file);

    cout << "File created successfully." << endl;
}

// List directory
void FileSystem::ls() const {
    for (size_t i = 0; i < current->children.size(); i++) {
        Node* child = current->children[i];

        if (child->isDirectory) {
            cout << "[DIR]  ";
        }
        else {
            cout << "[FILE] ";
        }

        cout << child->name << endl;
    }
}

// Print current path
void FileSystem::pwd() const {
    if (current == root) {
        cout << "/" << endl;
        return;
    }

    vector<string> paths;
    Node* temp = current;

    while (temp != root) {
        paths.push_back(temp->name);
        temp = temp->parent;
    }

    cout << "/";

    for (size_t i = paths.size(); i > 0; i--) {
        cout << paths[i - 1];

        if (i != 1) {
            cout << "/";
        }
    }

    cout << endl;
}

// Write or append file
void FileSystem::writeFile(
    const string& name,
    const string& text,
    bool append
) {
    Node* file = findChild(current, name);

    if (file == nullptr) {
        cout << "File not found!" << endl;
        return;
    }

    if (file->isDirectory) {
        cout << "Cannot write into a directory!" << endl;
        return;
    }

    if (!append) {
        file->content.clear();
    }

    file->content.push_back(text);

    cout << "File updated successfully." << endl;
}

// Display file
void FileSystem::cat(const string& name) const {
    Node* file = findChild(current, name);

    if (file == nullptr) {
        cout << "File not found!" << endl;
        return;
    }

    if (file->isDirectory) {
        cout << "Cannot read a directory!" << endl;
        return;
    }

    for (size_t i = 0; i < file->content.size(); i++) {
        cout << file->content[i] << endl;
    }
}

// Remove file or directory
void FileSystem::rm(const string& name) {
    if (name.empty() || name == "." || name == "..") {
        cout << "Invalid name!" << endl;
        return;
    }

    Node* target = findChild(current, name);

    if (target == nullptr) {
        cout << "File or directory not found!" << endl;
        return;
    }

    // Prevent deleting the current directory or its parent directories
    Node* temp = current;

    while (temp != nullptr) {
        if (temp == target) {
            cout << "Cannot remove the current directory or its parent!" << endl;
            return;
        }

        temp = temp->parent;
    }

    for (size_t i = 0; i < current->children.size(); i++) {
        if (current->children[i] == target) {
            destroyTree(target);

            current->children.erase(
                current->children.begin() + i
            );

            cout << "Removed successfully." << endl;
            return;
        }
    }
}
