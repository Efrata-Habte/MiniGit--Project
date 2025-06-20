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

