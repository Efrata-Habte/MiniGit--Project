#include "minigit.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <command> [args]\n";
        std::cerr << "Commands:\n";
        std::cerr << "  init\n";
        std::cerr << "  add <filename>\n";
        std::cerr << "  commit -m \"message\"\n";
        std::cerr << "  log\n";
        std::cerr << "  branch <branchname>\n";
        std::cerr << "  checkout <commit/branch>\n";
        std::cerr << "  merge <branch>\n";
        std::cerr << "  diff <commit1> <commit2>\n";
        return 1;
    }

    std::string command = argv[1];

    // Handle init command
    if (command == "init") {
        MiniGit::init();
    }
    // Handle add command
    else if (command == "add" && argc > 2) {
        MiniGit::add(argv[2]);
    }
    // Handle commit command
    else if (command == "commit" && argc > 3 && std::string(argv[2]) == "-m") {
        MiniGit::commit(argv[3]);
    }
    // Handle log command
    else if (command == "log") {
        MiniGit::log();
    }
    // Handle branch command
    else if (command == "branch" && argc > 2) {
        MiniGit::branch(argv[2]);
    }
    // Handle checkout command
    else if (command == "checkout" && argc > 2) {
        MiniGit::checkout(argv[2]);
