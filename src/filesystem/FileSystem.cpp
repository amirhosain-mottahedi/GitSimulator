
#include "FileSystem.h"

#include <algorithm>
#include <stdexcept>

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

// Validate file and directory names
bool FileSystem::isValidName(const string& name) const {
    if (name.empty() || name == "." || name == "..") {
        return false;
    }

    for (size_t i = 0; i < name.length(); i++) {
        char c = name[i];

        if (!((c >= 'a' && c <= 'z') ||
              (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') ||
              c == '.' || c == '-' || c == '_')) {
            return false;
        }
    }

    return true;
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
                if (!temp->isDirectory) {
                    return nullptr;
                }

                temp = findChild(temp, part);

                if (temp == nullptr) {
                    return nullptr;
                }
            }

            part.clear();
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
    if (!isValidName(name)) {
        throw invalid_argument("Invalid directory name!");
    }

    if (findChild(current, name) != nullptr) {
        throw runtime_error("Name already exists!");
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
        throw runtime_error("Path not found!");
    }

    if (!target->isDirectory) {
        throw runtime_error("This is not a directory!");
    }

    current = target;
}

// Create file
void FileSystem::touch(const string& name) {
    if (!isValidName(name)) {
        throw invalid_argument("Invalid file name!");
    }

    Node* existing = findChild(current, name);

    if (existing != nullptr) {
        if (!existing->isDirectory) {
            return;
        }

        throw runtime_error("A directory with this name already exists!");
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
    vector<Node*> sortedChildren = current->children;

    sort(
        sortedChildren.begin(),
        sortedChildren.end(),
        [](Node* a, Node* b) {
            return a->name < b->name;
        }
    );

    for (size_t i = 0; i < sortedChildren.size(); i++) {
        Node* child = sortedChildren[i];

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
string FileSystem::getCurrentPath() const {
    if (current == root) {
        return "/";
    }

    vector<string> paths;
    Node* temp = current;

    while (temp != root) {
        paths.push_back(temp->name);
        temp = temp->parent;
    }

    string result = "";

    for (size_t i = paths.size(); i > 0; i--) {
        result += "/";
        result += paths[i - 1];
    }

    return result;
}

void FileSystem::pwd() const {
    cout << getCurrentPath() << endl;
}

// Write or append file
void FileSystem::writeFile(
    const string& name,
    const string& text,
    bool append
) {
    if (!isValidName(name)) {
        throw invalid_argument("Invalid file name!");
    }

    Node* file = findChild(current, name);

    if (file == nullptr) {
        touch(name);
        file = findChild(current, name);
    }

    if (file->isDirectory) {
        throw runtime_error("Cannot write into a directory!");
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
        throw runtime_error("File not found!");
    }

    if (file->isDirectory) {
        throw runtime_error("Cannot read a directory!");
    }

    for (size_t i = 0; i < file->content.size(); i++) {
        cout << file->content[i] << endl;
    }
}

// Remove file only
void FileSystem::rm(const string& name) {
    Node* target = findChild(current, name);

    if (target == nullptr) {
        throw runtime_error("File not found!");
    }

    if (target->isDirectory) {
        throw runtime_error("rm can only remove files!");
    }

    for (size_t i = 0; i < current->children.size(); i++) {
        if (current->children[i] == target) {
            destroyTree(target);

            current->children.erase(
                current->children.begin() + i
            );

            cout << "File removed successfully." << endl;
            return;
        }
    }
}

// Collect files recursively
void FileSystem::collectFiles(
    Node* directory,
    const string& relativePath,
    vector<FileData>& result
) const {
    if (directory == nullptr || !directory->isDirectory) {
        return;
    }

    for (size_t i = 0; i < directory->children.size(); i++) {
        Node* child = directory->children[i];

        string childPath;

        if (relativePath.empty()) {
            childPath = child->name;
        }
        else {
            childPath = relativePath + "/" + child->name;
        }

        if (child->isDirectory) {
            collectFiles(child, childPath, result);
        }
        else {
            FileData data;

            data.path = childPath;
            data.content = child->content;

            result.push_back(data);
        }
    }
}

// Get all files inside a directory
vector<FileSystem::FileData> FileSystem::getAllFiles(
    const string& directoryPath
) const {
    vector<FileData> result;

    Node* directory = const_cast<FileSystem*>(this)->resolvePath(
        directoryPath
    );

    if (directory == nullptr || !directory->isDirectory) {
        throw runtime_error("Directory not found!");
    }

    collectFiles(directory, "", result);

    sort(
        result.begin(),
        result.end(),
        [](const FileData& a, const FileData& b) {
            return a.path < b.path;
        }
    );

    return result;
}

// Read file by path
bool FileSystem::readFile(
    const string& path,
    vector<string>& content
) const {
    Node* file = const_cast<FileSystem*>(this)->resolvePath(path);

    if (file == nullptr || file->isDirectory) {
        return false;
    }

    content = file->content;

    return true;
}

// Create file by path
bool FileSystem::createFile(
    const string& path,
    const vector<string>& content
) {
    size_t separator = path.find_last_of('/');

    string directoryPath;
    string fileName;

    if (separator == string::npos) {
        directoryPath = getCurrentPath();
        fileName = path;
    }
    else {
        directoryPath = path.substr(0, separator);

        if (directoryPath.empty()) {
            directoryPath = "/";
        }

        fileName = path.substr(separator + 1);
    }

    if (!isValidName(fileName)) {
        return false;
    }

    Node* directory = resolvePath(directoryPath);

    if (directory == nullptr || !directory->isDirectory) {
        return false;
    }

    if (findChild(directory, fileName) != nullptr) {
        return false;
    }

    Node* file = new Node;

    file->name = fileName;
    file->isDirectory = false;
    file->parent = directory;
    file->content = content;

    directory->children.push_back(file);

    return true;
}

// Delete file by path
bool FileSystem::deleteFile(const string& path) {
    Node* target = resolvePath(path);

    if (target == nullptr || target->isDirectory) {
        return false;
    }

    Node* parent = target->parent;

    for (size_t i = 0; i < parent->children.size(); i++) {
        if (parent->children[i] == target) {
            destroyTree(target);

            parent->children.erase(
                parent->children.begin() + i
            );

            return true;
        }
    }

    return false;
}
