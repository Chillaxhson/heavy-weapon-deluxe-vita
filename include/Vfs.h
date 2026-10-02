#pragma once

#include <string>
#include <vector>

namespace HeavyWeapon {

class Vfs {
public:
    static void Init(const std::string& customBasePath = "");
    static std::string Resolve(const std::string& relativePath);
    static bool Exists(const std::string& relativePath);
    static std::string ReadTextFile(const std::string& relativePath);

private:
    static std::string sBasePath;
    static std::vector<std::string> sSearchPaths;
    static std::string NormalizeSlashes(const std::string& path);
};

} // namespace HeavyWeapon
