#include "Vfs.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <algorithm>
#include "core/Perf.h"

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

// Case-insensitive lookup of a single directory entry.
static bool FindEntryCaseInsensitive(const std::string& dirPath, const std::string& name, std::string& outName) {
    DIR* dir = opendir(dirPath.empty() ? "." : dirPath.c_str());
    if (!dir) return false;

    std::string lowerTarget = name;
    std::transform(lowerTarget.begin(), lowerTarget.end(), lowerTarget.begin(), ::tolower);

    bool found = false;
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string lowerEntry = entry->d_name;
        std::transform(lowerEntry.begin(), lowerEntry.end(), lowerEntry.begin(), ::tolower);
        if (lowerEntry == lowerTarget) {
            outName = entry->d_name;
            found = true;
            break;
        }
    }
    closedir(dir);
    return found;
}

// Case-insensitive path search for POSIX systems. PopCap data references paths with
// arbitrary casing (e.g. "frigistan\yetti" for Images/Anims/Frigistan/yetti.png), so every
// component after the search root is matched case-insensitively.
static std::string FindCaseInsensitive(const std::string& root, const std::string& relative) {
    std::string direct = root + relative;
    if (access(direct.c_str(), F_OK) == 0) {
        return direct;
    }

    std::string current = root;
    size_t start = 0;
    while (start <= relative.size()) {
        size_t slash = relative.find('/', start);
        std::string part = relative.substr(start, slash == std::string::npos ? std::string::npos : slash - start);
        if (!part.empty()) {
            std::string match;
            std::string dirPath = current.empty() ? "." : current;
            if (!dirPath.empty() && dirPath.back() == '/' && dirPath.size() > 1) dirPath.pop_back();
            if (!FindEntryCaseInsensitive(dirPath, part, match)) {
                return direct;
            }
            current += match;
            if (slash != std::string::npos) current += "/";
        }
        if (slash == std::string::npos) break;
        start = slash + 1;
    }
    return current;
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
    PERF_SCOPE_MIN("Vfs::Resolve " + relativePath, 5.0);
    std::string clean = NormalizeSlashes(relativePath);
    if (!clean.empty() && clean[0] == '/') {
        clean = clean.substr(1);
    }

    for (const auto& base : sSearchPaths) {
        std::string root = base;
        if (!root.empty() && root.back() != '/' && root.back() != ':') {
            root += "/";
        }

        std::string resolved = FindCaseInsensitive(root, clean);
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
