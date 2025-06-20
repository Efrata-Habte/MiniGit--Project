#ifndef MINIGIT_H
#define MINIGIT_H
#include <string>
#include <set>
#include <fstream>
#include <sstream>

namespace MiniGit {
    // Intializes a new Minigit repo in the current directory
    void init();

    // Add a file to the staging index
    void add(const std::string& filename);

    // Commits all staged files with the given message 
    void commit(const std::string& message, bool isMerge = false);

    // Shows the commit history 
    void log();

    // Creates a new branch pointing to the current commit
    void branch(const std::string& branchName);

    // Checks out a branch or specific commit
    void checkout(const std::string& name);

    // Joins the changes from another branch into the current branch
    void merge(const std::string& branchName);

    //compare the contents of two commits and shows their differences
    void diff(const std::string& commit1, const std::string& commit2);
    
    // Helper function to read blob content
    static std::string readBlobContent(const std::string& hash) {
        std::ifstream blobFile(".minigit/objects/" + hash);
        if (!blobFile) {
            return "";
        }
        std::ostringstream content;
        content << blobFile.rdbuf();
        return content.str();
    }
}
#endif