
#ifndef COMMIT_H
#define COMMIT_H

#include <string>
#include <vector>

using namespace std;

class Commit {
public:
    struct FileSnapshot {
        string path;
        vector<string> content;
    };

private:
    int id;
    int parentId;

    string message;
    string authorName;
    string authorEmail;
    string date;

    vector<FileSnapshot> snapshot;

public:
    Commit(
        int commitId,
        int parentCommitId,
        const string& commitMessage,
        const string& name,
        const string& email,
        const string& commitDate,
        const vector<FileSnapshot>& files
    );

    int getId() const;
    int getParentId() const;

    string getMessage() const;
    string getAuthorName() const;
    string getAuthorEmail() const;
    string getDate() const;

    const vector<FileSnapshot>& getSnapshot() const;
};

#endif
