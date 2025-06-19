#ifndef MINIGIT_H
#define MINIGIT_H
#include <string>

namespace MiniGit {
    
    // Initialize a new MiniGit repo in the current directory
    void init(); 
    
    // Adds a file to staging index 
    void add(const std::string& filename);

    // Commits all staged files with a given message
    void commit(const std::string& message);
    
    // Displays the commit hisotry 
    void log();

    // Cretes new branches pointing to the current branch
    void branch(const std::string& branchName);

    // Checks out a branch or a specific commit
    void checkout(const std::string& name);

    // Joins the changes from another branch into the current branch
    void merge(const std::string& branchName);
}

#endif // MINIGIT_H



