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

void MiniGit::init() {
    fs::path minigitDir = ".minigit";
    if (fs::exists(minigitDir)) {
        std::cout << ".minigit already exists.\n";
        return;
    }

    fs::create_directory(minigitDir);
    fs::create_directory(minigitDir / "objects");
    fs::create_directories(minigitDir / "refs" / "heads");
    std::ofstream headFile(minigitDir / "HEAD");
    headFile << "ref: refs/heads/main\n";
    headFile.close();
    
    std::cout << "Initialized empty MiniGit repository in .minigit/\n";

}

void MiniGit::add(const std::string& filename) {
    fs::path filePath = filename;
    if (!fs::exists(filePath)) {
        std::cout << "File not found: " << filename << "\n";
        return;
    }

    std::ifstream inFile(filePath, std::ios::binary);
    std::ostringstream buffer;
    buffer << inFile.rdbuf();
    std::string content = buffer.str();
    std::string hash = sha1(content);
    
    fs::path objectPath = ".minigit/objects/" + hash;
    if (!fs::exists(objectPath)) {
        std::ofstream outFile(objectPath, std::ios::binary);
        outFile << content;
        outFile.close();
    }
    std::ofstream indexFile("index", std::ios::app);
    indexFile << filename << " " << hash << "\n";
    indexFile.close();
    std::cout << "Added " << filename << " (" << hash.substr(0, 8) << ")\n";

    }

void MiniGit::commit(const std::string& message, bool isMerge) {
    // Check if there are staged files to commit
    std::ifstream indexFile(".minigit/index");
    if (!indexFile) {
        std::cout << "Nothing to commit.\n";
        return;
    }

    // Read all staged files and their hashes
    std::string files_blobs, line;
    while (std::getline(indexFile, line)) {
        files_blobs += line + "\n";
    }

    // Get current commit hash (parent of this new commit)
    std::string headCommitHash;
    std::ifstream headFile(".minigit/HEAD");
    std::string refLine;
    std::getline(headFile, refLine);
    if (refLine.find("ref:") == 0) {
        std::string branchFile = ".minigit/" + refLine.substr(5);
        std::ifstream bFile(branchFile);
        std::getline(bFile, headCommitHash);
    }

    // Create timestamp for commit
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    
    // Build commit object content
    std::ostringstream commitObj;
    
    // Add parent commit reference
    if (!headCommitHash.empty()) {
        commitObj << "parent: " << headCommitHash << "\n";
    }
    
    // For merge commits, add second parent
    if (isMerge) {
        std::ifstream mergeFile(".minigit/MERGE_HEAD");
        std::string mergeParent;
        if (mergeFile && std::getline(mergeFile, mergeParent)) {
            commitObj << "parent: " << mergeParent << "\n";
        }
        fs::remove(".minigit/MERGE_HEAD"); // Clean up merge state
    }
    
    // Add commit metadata
    commitObj << "date: " << std::ctime(&now_c);  // Timestamp
    commitObj << "message: " << message << "\n";  // Commit message
    commitObj << files_blobs;                     // File listings
    
    // Create unique hash for this commit
    std::string commitStr = commitObj.str();
    std::string commitHash = sha1(commitStr);
    
    // Store commit object
    std::ofstream commitFile(".minigit/objects/" + commitHash);
    commitFile << commitStr;
    commitFile.close();

    // Update branch reference if not in detached HEAD state
    if (refLine.find("ref:") == 0) {
        std::string branchFile = ".minigit/" + refLine.substr(5);
        std::ofstream bFile(branchFile);
        bFile << commitHash;
    }
    
    // Clear staging area
    std::ofstream(".minigit/index", std::ios::trunc).close();
    std::cout << "Committed " << commitHash.substr(0, 8) << ": " << message << "\n";
}
// Helper Functions
// Gets the current commit hash from HEAD
std::string getCurrentCommitHash() {
    std::ifstream headFile(".minigit/HEAD");
    std::string refLine;
    std::getline(headFile, refLine);
    
    // If HEAD points to a branch (not detached)
    if (refLine.find("ref:") == 0) {
        std::string branchFile = ".minigit/" + refLine.substr(5);
        std::ifstream bFile(branchFile);
        std::string commitHash;
        std::getline(bFile, commitHash);
        return commitHash;
    }
    return refLine; // Return raw commit hash in detached state
}

// Finds where two commit histories diverged
std::string findCommonAncestor(const std::string& commit1, const std::string& commit2) {
    std::unordered_set<std::string> commit1Parents;
    std::string current = commit1;
    
    // Record all ancestors of first commit
    while (!current.empty()) {
        commit1Parents.insert(current);
        std::ifstream commitFile(".minigit/objects/" + current);
        std::string line;
        std::getline(commitFile, line); // Read "parent: ..." line
        current = line.find("parent: ") == 0 ? line.substr(8) : "";
    }

    // Check second commit's ancestors for matches
    current = commit2;
    while (!current.empty()) {
        if (commit1Parents.count(current)) return current; // Found common ancestor
        std::ifstream commitFile(".minigit/objects/" + current);
        std::string line;
        std::getline(commitFile, line);
        current = line.find("parent: ") == 0 ? line.substr(8) : "";
    }
    return ""; // No common ancestor found
}

void MiniGit::log() {
    std::string commitHash;
    std::ifstream headFile(".minigit/HEAD");
    std::string refLine;
    std::getline(headFile, refLine);
    
    if (refLine.find("ref:") == 0) {
        std::string branchFile = ".minigit/" + refLine.substr(5);
        std::ifstream bFile(branchFile);
        std::getline(bFile, commitHash);
    } else {
        commitHash = refLine;
    }

    while (!commitHash.empty()) {
        std::ifstream commitFile(".minigit/objects/" + commitHash);
        if (!commitFile) break;

        std::vector<std::string> parents; // Changed to handle multiple parents
        std::string date, message, line;
        
        while (std::getline(commitFile, line)) {
            if (line.find("parent: ") == 0) {
                parents.push_back(line.substr(8)); // Store all parents
            }
            else if (line.find("date: ") == 0) date = line.substr(6);
            else if (line.find("message: ") == 0) message = line.substr(9);
        }

        std::cout << "commit " << commitHash << "\n";
        if (parents.size() > 1) { // New: Show merge info
            std::cout << "Merge: ";
            for (size_t i = 0; i < parents.size(); ++i) {
                std::cout << parents[i].substr(0, 8);
                if (i != parents.size() - 1) std::cout << " ";
            }
            std::cout << "\n";
        }
        std::cout << "Date:   " << date;
        std::cout << "    " << message << "\n\n";
        
        commitHash = parents.empty() ? "" : parents[0]; // Follow first parent
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
    
