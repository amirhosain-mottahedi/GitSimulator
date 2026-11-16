
#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <iostream>
#include <string>
#include <vector>

using namespace std;

class FileSystem {
public:
    struct FileData {
        string path;
        vector<string> content;
    };

private:
    struct Node {
        string name;
        bool isDirectory;
        vector<string> content;

        Node* parent;
        vector<Node*> children;
    };

    Node* root;
    Node* current;

    Node* resolvePath(const string& path);
    Node* findChild(Node* parent, const string& name) const;

    void destroyTree(Node* node);

    void collectFiles(
        Node* directory,
        const string& relativePath,
        vector<FileData>& result
    ) const;

    bool isValidName(const string& name) const;

public:
    FileSystem();
    ~FileSystem();

    void mkdir(const string& name);
    void cd(const string& path);
    void touch(const string& name);

    void ls() const;
    void pwd() const;

    void writeFile(
        const string& name,
        const string& text,
        bool append
    );

    void cat(const string& name) const;
    void rm(const string& name);

    // Information for Git
    string getCurrentPath() const;

    vector<FileData> getAllFiles(
        const string& directoryPath
    ) const;

    bool readFile(
        const string& path,
        vector<string>& content
    ) const;

    bool createFile(
        const string& path,
        const vector<string>& content
    );

    bool deleteFile(const string& path);

};

#endif
