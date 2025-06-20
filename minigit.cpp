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

//Compares the differences between two MiniGit commits.
void MiniGit::merge(const std::string& branchName) {
    // Check if branch exists
    fs::path branchPath = ".minigit/refs/heads/" + branchName;
    if (!fs::exists(branchPath)) {
        std::cout << "Branch not found: " << branchName << "\n";
        return;
    }

    // Get commit hashes
    std::string ourCommit = getCurrentCommitHash();
    std::ifstream targetFile(branchPath);
    std::string theirCommit;
    std::getline(targetFile, theirCommit);

    // Find common ancestor (LCA)
    std::string ancestor = findCommonAncestor(ourCommit, theirCommit);
    if (ancestor.empty()) {
        std::cout << "Cannot find common ancestor.\n";
        return;
    }

    // Read files from all three points (ours, theirs, ancestor)
    std::map<std::string, std::string> ourFiles, theirFiles, baseFiles;
    auto readFiles = [](const std::string& commit, auto& files) {
        std::ifstream commitFile(".minigit/objects/" + commit);
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

    readFiles(ourCommit, ourFiles);
    readFiles(theirCommit, theirFiles);
    readFiles(ancestor, baseFiles);

    // Perform three-way merge
    bool conflict = false;
    for (const auto& [fname, theirBlob] : theirFiles) {
        auto ourIt = ourFiles.find(fname);
        auto baseIt = baseFiles.find(fname);

        // Conflict detection
        if (ourIt != ourFiles.end() && ourIt->second != theirBlob && 
            (baseIt == baseFiles.end() || ourIt->second != baseIt->second)) {
            std::cout << "CONFLICT: " << fname << "\n";
            conflict = true;
            
            // Write conflict markers
            std::ofstream conflictFile(fname + ".rej");
            conflictFile << "<<<<<<< our changes\n"
                         << readBlobContent(ourIt->second) 
                         << "=======\n"
                         << readBlobContent(theirBlob)
                         << ">>>>>>> their changes\n";
            continue;
        }

        // Safe to apply their changes
        std::ifstream blobFile(".minigit/objects/" + theirBlob, std::ios::binary);
        std::ofstream outFile(fname, std::ios::binary);
        outFile << blobFile.rdbuf();
    }

    // Create merge commit if no conflicts
    if (!conflict) {
        std::ofstream(".minigit/MERGE_HEAD") << theirCommit;
        commit("Merge branch '" + branchName + "'", true);
    } else {
        std::cout << "Resolve conflicts and commit manually\n";
    }
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

    // Load file lists for both commits
    std::map<std::string, std::string> files1, files2;

    auto readFiles = [](const std::string& commit, std::map<std::string, std::string>& files) {
        std::ifstream commitFile(".minigit/objects/" + commit);
        std::string line;
        for (int i = 0; i < 3; ++i) std::getline(commitFile, line); // skip metadata
        while (std::getline(commitFile, line)) {
            if (line.empty()) continue;
            std::istringstream iss(line);
            std::string fname, blob;
            iss >> fname >> blob;
            if (!fname.empty() && !blob.empty()) files[fname] = blob;
        }
    };

    readFiles(commit1, files1);
    readFiles(commit2, files2);

    // Combine all unique file names
    std::set<std::string> allFiles;
    for (const auto& [fname, _] : files1) allFiles.insert(fname);
    for (const auto& [fname, _] : files2) allFiles.insert(fname);

    // Compare each file
    for (const auto& fname : allFiles) {
        auto it1 = files1.find(fname);
        auto it2 = files2.find(fname);

        if (it1 == files1.end()) {
            std::cout << "\nFile added in " << commit2 << ": " << fname << "\n";
            std::vector<std::string> lines2 = readLines(it2->second);
            for (const auto& line : lines2) {
                std::cout << "+ " << line << "\n";
            }
            continue;
        }
        if (it2 == files2.end()) {
            std::cout << "\nFile removed in " << commit2 << ": " << fname << "\n";
            std::vector<std::string> lines1 = readLines(it1->second);
            for (const auto& line : lines1) {
                std::cout << "- " << line << "\n";
            }
            continue;
        }
        
        // If we get here, the file exists in both commits
        if (it1->second != it2->second) {
            std::cout << "\nFile modified: " << fname << "\n";
        }

        // Both commits have the file, compare content line by line
        std::vector<std::string> lines1 = readLines(it1->second);
        std::vector<std::string> lines2 = readLines(it2->second);

        std::cout << "--- " << fname << "\n";

        size_t maxLines = std::max(lines1.size(), lines2.size());
        for (size_t i = 0; i < maxLines; ++i) {
            std::string l1 = (i < lines1.size()) ? lines1[i] : "";
            std::string l2 = (i < lines2.size()) ? lines2[i] : "";

            if (l1 == l2) {
                continue; // no change
            }
            if (i >= lines1.size()) {
                std::cout << "+ " << l2 << "\n"; // new line added
            } else if (i >= lines2.size()) {
                std::cout << "- " << l1 << "\n"; // line deleted
            } else {
                std::cout << "- " << l1 << "\n";
                std::cout << "+ " << l2 << "\n";
            }
        }
    }
}
