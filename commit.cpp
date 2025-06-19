#include "minigit.h"
#include "hash.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <map>

namespace fs = std::filesystem;
// create commit function 
void MiniGit::commit(const std::string& message) {
    // Read index
    std::ifstream indexFile("index");
    if (!indexFile) {
        std::cout << "Nothing to commit.\n";
        return;
    }
    std::string files_blobs, line;
    while (std::getline(indexFile, line)) {
        files_blobs += line + "\n";
    }
    if (files_blobs.empty()) {
        std::cout << "Nothing to commit.\n";
        return;
    }
    // Get parent commit
    std::string headCommitHash;
    std::ifstream headFile(".minigit/refs/heads/main");
    if (headFile) {
        std::getline(headFile, headCommitHash);
    }
    // Timestamp
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    // Create commit object
    std::ostringstream commitObj;
    commitObj << "parent: " << headCommitHash << "\n";
    commitObj << "date: " << std::ctime(&now_c);
    commitObj << "message: " << message << "\n";
    commitObj << files_blobs;
    std::string commitStr = commitObj.str();
    std::string commitHash = sha1(commitStr);
    // Store commit object
    std::ofstream commitFile(".minigit/objects/" + commitHash);
    commitFile << commitStr;
    commitFile.close();
    // Update branch ref
    std::ofstream branchFile(".minigit/refs/heads/main");
    branchFile << commitHash << "\n";
    branchFile.close();
    // Clear index
    std::ofstream clearIndex("index", std::ios::trunc);
    clearIndex.close();
    std::cout << "Committed as " << commitHash.substr(0, 8) << ": " << message << "\n";
}
