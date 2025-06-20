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


    }


