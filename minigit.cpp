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
    
