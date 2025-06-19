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
void MiniGit::checkout(const std::string& name) {
    // Check if name is a branch
    fs::path branchPath = ".minigit/refs/heads/" + name;
    std::string commitHash;
    if (fs::exists(branchPath)) {
        std::ifstream branchFile(branchPath);
        std::getline(branchFile, commitHash);
        branchFile.close();
        // Update HEAD
        std::ofstream headFile(".minigit/HEAD");
        headFile << "ref: refs/heads/" << name << "\n";
        headFile.close();
        std::cout << "Switched to branch '" << name << "'\n";
    } else {
        // Assume it's a commit hash
        commitHash = name;
        // Validate commit exists
        fs::path commitPath = ".minigit/objects/" + commitHash;
        if (!fs::exists(commitPath)) {
            std::cout << "Unknown branch or commit: " << name << "\n";
            return;
        }
        // Update HEAD to detached
        std::ofstream headFile(".minigit/HEAD");
        headFile << commitHash << "\n";
        headFile.close();
        std::cout << "HEAD is now at " << commitHash.substr(0, 8) << "\n";
    }
    // Restore working directory to commit state
    // Find commit hash
    if (commitHash.empty()) return;
    fs::path commitPath = ".minigit/objects/" + commitHash;
    std::ifstream commitFile(commitPath);
    if (!commitFile) return;
    std::string line;
    // Skip commit metadata
    for (int i = 0; i < 3; ++i) std::getline(commitFile, line);
    // Restore files
    while (std::getline(commitFile, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string fname, blob;
        iss >> fname >> blob;
        if (!fname.empty() && !blob.empty()) {
            std::ifstream blobFile(".minigit/objects/" + blob, std::ios::binary);
            std::ofstream outFile(fname, std::ios::binary);
            outFile << blobFile.rdbuf();
        }
    }
}
