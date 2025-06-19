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
