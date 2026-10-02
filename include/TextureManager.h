#pragma once

#include <string>
#include <unordered_map>
#include <vitaGL.h>

namespace HeavyWeapon {

struct Texture {
    GLuint id = 0;
    int width = 0;
    int height = 0;
    bool hasAlpha = false;
};

class TextureManager {
public:
    static void Init();
    static void Shutdown();

    // Loads a texture by relative path, automatically detecting alpha masks (.jpg + _ .png)
    static Texture* Load(const std::string& relativePath);
    static Texture* Get(const std::string& relativePath);
    static void Unload(const std::string& relativePath);
    static void Clear();

private:
    static std::unordered_map<std::string, Texture> sTextures;
    static Texture LoadFromFiles(const std::string& colorPath, const std::string& maskPath);
};

} // namespace HeavyWeapon
