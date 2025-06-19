#ifndef MINIGIT_H
#define MINIGIT_H
#include <string>

namespace MiniGit {
    void init();
    void add(const std::string& filename);
    void commit(const std::string& message);
    void log();
    void branch(const std::string& branchName);
    void checkout(const std::string& name);
    void merge(const std::string& branchName);
}

#endif // MINIGIT_H



