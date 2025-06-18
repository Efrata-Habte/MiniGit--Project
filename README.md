# MiniGit--Project
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>


void MiniGit::log() {
    // Get latest commit hash
    std::ifstream branchFile(".minigit/refs/heads/main");
    std::string commitHash;
    if (!branchFile || !(branchFile >> commitHash) || commitHash.empty()) {
        std::cout << "No commits yet.\n";
        return;
    }
    while (!commitHash.empty()) {
        fs::path commitPath = ".minigit/objects/" + commitHash;
        std::ifstream commitFile(commitPath);
        if (!commitFile) break;
        std::string line, parent, date, message;
        std::getline(commitFile, line);
        if (line.rfind("parent: ", 0) == 0) parent = line.substr(8);
        std::getline(commitFile, line);
       
    }
}
