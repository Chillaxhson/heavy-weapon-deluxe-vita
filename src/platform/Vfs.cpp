#include "Vfs.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <algorithm>

namespace HeavyWeapon {

std::string Vfs::sBasePath = "";
std::vector<std::string> Vfs::sSearchPaths;

std::string Vfs::NormalizeSlashes(const std::string& path) {
    std::string res = path;
    for (char& c : res) {
        if (c == '\\') c = '/';
    }
    return res;
}

// Case-insensitive path search for POSIX systems
static std::string FindCaseInsensitive(const std::string& fullPath) {
    if (access(fullPath.c_str(), F_OK) == 0) {
        return fullPath;
    }

    size_t lastSlash = fullPath.find_last_of('/');
    if (lastSlash == std::string::npos) return fullPath;

    std::string dirPath = fullPath.substr(0, lastSlash);
    std::string fileName = fullPath.substr(lastSlash + 1);

    DIR* dir = opendir(dirPath.empty() ? "." : dirPath.c_str());
    if (!dir) return fullPath;

    std::string lowerTarget = fileName;
    std::transform(lowerTarget.begin(), lowerTarget.end(), lowerTarget.begin(), ::tolower);

    struct dirent* entry;
    std::string match = "";
    while ((entry = readdir(dir)) != nullptr) {
        std::string entryName = entry->d_name;
        std::string lowerEntry = entryName;
        std::transform(lowerEntry.begin(), lowerEntry.end(), lowerEntry.begin(), ::tolower);
        if (lowerEntry == lowerTarget) {
            match = dirPath + "/" + entryName;
            break;
        }
    }
    closedir(dir);
    return match.empty() ? fullPath : match;
}

void Vfs::Init(const std::string& customBasePath) {
    sSearchPaths.clear();
    if (!customBasePath.empty()) {
        sSearchPaths.push_back(customBasePath);
    }
#ifdef __vita__
    sSearchPaths.push_back("ux0:data/heavyweapon/");
    sSearchPaths.push_back("app0:");
#endif
    sSearchPaths.push_back("Heavy Weapon Deluxe/");
    sSearchPaths.push_back("./");
}

std::string Vfs::Resolve(const std::string& relativePath) {
    std::string clean = NormalizeSlashes(relativePath);
    if (!clean.empty() && clean[0] == '/') {
        clean = clean.substr(1);
    }

    for (const auto& base : sSearchPaths) {
        std::string combined = base;
        if (!combined.empty() && combined.back() != '/') {
            combined += "/";
        }
        combined += clean;

        std::string resolved = FindCaseInsensitive(combined);
        if (access(resolved.c_str(), F_OK) == 0) {
            return resolved;
        }
    }
    return clean;
}

bool Vfs::Exists(const std::string& relativePath) {
    std::string res = Resolve(relativePath);
    return access(res.c_str(), F_OK) == 0;
}

std::string Vfs::ReadTextFile(const std::string& relativePath) {
    std::string path = Resolve(relativePath);
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

} // namespace HeavyWeapon
