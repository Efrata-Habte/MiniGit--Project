#include "hash.h"
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>

std::string sha1(const std::string& data) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(data.c_str()), data.size(), hash);
    std::ostringstream oss;
    for (int i = 0; i < SHA_DIGEST_LENGTH; ++i)
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    return oss.str();
}

void MiniGit::init() {
    fs::path minigitDir = ".minigit";
    if (fs::exists(minigitDir)) {
        std::cout << ".minigit already exists.\n";
        return;
    }
}
