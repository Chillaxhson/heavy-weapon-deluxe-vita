#pragma once

#include <string>
#include <unordered_map>
#include <vitaGL.h>

namespace HeavyWeapon {

struct Texture {
    GLuint id = 0;
    int width = 0;
    int height = 0;
    int cols = 1;
    int rows = 1;
    bool hasAlpha = false;

    int GetCelWidth() const { return cols > 0 ? (width / cols) : width; }
    int GetCelHeight() const { return rows > 0 ? (height / rows) : height; }
};

class TextureManager {
public:
    static void Init();
    static void Shutdown();

    // Loads or retrieves a texture by asset name or path, automatically handling alpha masks (.jpg + _.png)
    static Texture* Load(const std::string& relativePath);
    static Texture* Get(const std::string& relativePath);
    static void Unload(const std::string& relativePath);
    static void Clear();

    // Query dimensions and grid layout
    static void GetCelInfo(const std::string& name, int& outCols, int& outRows);

private:
    static std::unordered_map<std::string, Texture> sTextures;
    static Texture LoadFromFiles(const std::string& colorPath, const std::string& maskPath, int cols, int rows);
};

} // namespace HeavyWeapon
