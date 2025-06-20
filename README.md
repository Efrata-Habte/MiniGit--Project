# MiniGit--Project
#include "minigit.h"
#include "hash.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <map>
#include <vector>
#include <set>

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
        if (line.rfind("date: ", 0) == 0) date = line.substr(6);
        std::getline(commitFile, line);
        if (line.rfind("message: ", 0) == 0) message = line.substr(9);
        std::cout << "commit " << commitHash << "\n";
        std::cout << "Date:   " << date;
        std::cout << "    " << message << "\n\n";
        commitHash = parent;
    }
}

void MiniGit::branch(const std::string& branchName) {
    // Create a new branch pointer to current commit
    std::ifstream headFile(".minigit/HEAD");
    std::string refLine;
    std::getline(headFile, refLine);
    std::string branchRef = refLine.substr(5); // skip "ref: "
    headFile.close();
    std::ifstream branchHead(".minigit/" + branchRef);
    std::string currentCommit;
    std::getline(branchHead, currentCommit);
    branchHead.close();
    std::ofstream newBranch(".minigit/refs/heads/" + branchName);
    newBranch << currentCommit << "\n";
    newBranch.close();
    std::cout << "Branch '" << branchName << "' created at " << currentCommit.substr(0, 8) << "\n";
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
void MiniGit::merge(const std::string& branchName) {
    // Simple merge: just copy files from target branch's latest commit
    fs::path branchPath = ".minigit/refs/heads/" + branchName;
    if (!fs::exists(branchPath)) {
        std::cout << "Branch not found: " << branchName << "\n";
        return;
    }
    std::string targetCommit;
    std::ifstream branchFile(branchPath);
    std::getline(branchFile, targetCommit);
    branchFile.close();

    // Find current branch
    std::ifstream headFile(".minigit/HEAD");
    std::string refLine;
    std::getline(headFile, refLine);
    std::string branchRef = refLine.substr(5);
    headFile.close();

    std::ifstream currBranch(".minigit/" + branchRef);
    std::string currCommit;
    std::getline(currBranch, currCommit);
    currBranch.close();

    // Naive LCA
    std::string lca = currCommit;

    // Read files from commits
    std::map<std::string, std::string> currFiles, targetFiles;
    auto readFiles = [](const std::string& commit, std::map<std::string, std::string>& files) {
        fs::path commitPath = ".minigit/objects/" + commit;
        std::ifstream commitFile(commitPath);
        std::string line;
        for (int i = 0; i < 3; ++i) std::getline(commitFile, line);
        while (std::getline(commitFile, line)) {
            if (line.empty()) continue;
            std::istringstream iss(line);
            std::string fname, blob;
            iss >> fname >> blob;
            if (!fname.empty() && !blob.empty()) files[fname] = blob;
        }
    };
    readFiles(currCommit, currFiles);
    readFiles(targetCommit, targetFiles);

    // Merge
    bool conflict = false;
    for (const auto& [fname, tblob] : targetFiles) {
        auto it = currFiles.find(fname);
        if (it != currFiles.end() && it->second != tblob) {
            std::cout << "CONFLICT: both modified " << fname << "\n";
            conflict = true;
            continue;
        }
        std::ifstream blobFile(".minigit/objects/" + tblob, std::ios::binary);
        std::ofstream outFile(fname, std::ios::binary);
        outFile << blobFile.rdbuf();
    }

    if (!conflict)
        std::cout << "Merge completed.\n";
    else
        std::cout << "Merge completed with conflicts.\n";
}

void MiniGit::diff(const std::string& commit1, const std::string& commit2) {
    namespace fs = std::filesystem;
    // Helper to read file contents into vector of lines
    auto readLines = [](const std::string& blob) -> std::vector<std::string> {
        std::ifstream file(".minigit/objects/" + blob);
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(file, line)) {
            lines.push_back(line);
        }
        return lines;
    };
}


