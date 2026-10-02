#include "TextureManager.h"
#include "Vfs.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cstring>

namespace HeavyWeapon {

struct ImageMeta {
    const char* name;
    int cols;
    int rows;
};

static const ImageMeta sImageMetaTable[] = {
    { "title", 1, 1 },
    { "atomictank", 1, 1 },
    { "statusbar", 1, 1 },
    { "statuscovers", 1, 1 },
    { "tank", 10, 1 },
    { "tankshadow", 1, 1 },
    { "gun", 21, 5 },
    { "bullets", 21, 5 },
    { "rotors", 7, 1 },
    { "spark", 10, 1 },
    { "dumbbomb", 10, 1 },
    { "fragbomb", 10, 1 },
    { "ironbomb", 10, 1 },
    { "fatboy", 10, 1 },
    { "fatboyflash", 1, 1 },
    { "lgb", 21, 1 },
    { "hellfire", 21, 1 },
    { "crater", 5, 1 },
    { "rock", 1, 1 },
    { "tankflame", 20, 1 },
    { "nukeicon", 1, 1 },
    { "muzzleflash", 5, 1 },
    { "powerups", 12, 1 },
    { "shield", 10, 1 },
    { "shieldzap", 10, 1 },
    { "smoke", 1, 1 },
    { "dialog", 1, 1 },
    { "bombfrag", 1, 1 },
    { "missile", 20, 1 },
    { "flakflash", 5, 1 },
    { "buttonbracket", 3, 1 },
    { "button", 3, 1 },
    { "buttonglow", 3, 1 },
    { "tracks", 7, 1 },
    { "armory", 1, 1 },
    { "armoryglow", 1, 1 },
    { "upgrades", 6, 1 },
    { "upgradelvl", 4, 1 },
    { "upgradebtns", 2, 2 },
    { "upgradebubble", 1, 1 },
    { "laserglare", 10, 1 },
    { "rocket", 21, 1 },
    { "upgradeglow", 1, 1 },
    { "pupcopter", 1, 1 },
    { "puprotor", 7, 1 },
    { "copterblades", 10, 1 },
    { "crates", 4, 1 },
    { "casing", 1, 1 },
    { "satlaser", 10, 1 },
    { "enemygun", 10, 2 },
    { "enemytankshadow", 1, 1 },
    { "bolt", 5, 1 },
    { "staticspark", 10, 1 },
    { "staticglow", 5, 1 },
    { "megameter", 1, 1 },
    { "mainmenu", 1, 1 },
    { "mainsmallbtn", 1, 1 },
    { "mainbigbtn", 1, 1 },
    { "mainglow", 1, 1 },
    { "map", 1, 1 },
    { "mappointer", 1, 1 },
    { "maprect", 4, 1 },
    { "megalaser", 4, 1 },
    { "tankicon", 1, 1 },
    { "largeinsignia", 19, 1 },
    { "smallinsignia", 19, 1 },
    { "nukebg", 1, 1 },
    { "fire", 1, 1 },
    { "orb", 2, 1 },
    { "rpg", 10, 1 },
    { "explosion", 20, 1 },
    { "blimp", 1, 1 },
    { "blimpprop", 5, 1 },
    { "milemarker", 1, 1 },
    { "miletext", 5, 1 },
    { "tankflash", 1, 1 },
    { "headmissile", 20, 1 },
    { "sanddust", 1, 1 },
    { "boulder", 5, 1 },
    { "airmine", 5, 1 },
    { "meteorite", 10, 1 },
    { "bigdebris", 6, 1 },
    { "rocketpod", 21, 1 },
    { "flakguns", 21, 3 },
    { "lasers", 21, 3 },
    { "staticstrike", 1, 3 },
    { "homing", 3, 1 },
    { "laserburn", 10, 1 },
    { "burstrocket", 8, 1 },
    { "moon", 1, 1 },
    { "mushfire", 3, 1 },
    { "mushsmoke", 3, 1 },
    { "medrotor", 6, 1 },
    { "dozershadow", 1, 1 },
    { "deflectshield", 4, 1 },
    { "advancebtn", 1, 1 },
    { "bulletglow", 2, 1 },
    { "propfighter", 4, 1 },
    { "smalljet", 1, 1 },
    { "bomber", 1, 1 },
    { "jetfighter", 1, 1 },
    { "truck", 10, 1 },
    { "bigbomber", 1, 1 },
    { "smallcopter", 5, 1 },
    { "medcopter", 7, 1 },
    { "bigcopter", 9, 1 },
    { "deltabomber", 1, 1 },
    { "deltajet", 1, 1 },
    { "bigmissile", 10, 2 },
    { "superbomber", 1, 1 },
    { "fatbomber", 1, 1 },
    { "satellite", 10, 1 },
    { "strafer", 10, 1 },
    { "enemytank", 10, 1 },
    { "dozer", 10, 1 },
    { "deflector", 1, 1 },
    { "cruise", 8, 1 }
};

static std::string NormalizeKey(const std::string& path) {
    std::string s = path;
    std::replace(s.begin(), s.end(), '\\', '/');
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);

    // Strip "images/" prefix if present
    if (s.rfind("images/", 0) == 0) {
        s = s.substr(7);
    }
    // Strip trailing extension
    size_t dot = s.find_last_of('.');
    if (dot != std::string::npos) {
        std::string ext = s.substr(dot);
        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".gif") {
            s = s.substr(0, dot);
        }
    }
    return s;
}

std::unordered_map<std::string, Texture> TextureManager::sTextures;

void TextureManager::Init() {
    IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
}

void TextureManager::Shutdown() {
    Clear();
    IMG_Quit();
}

void TextureManager::GetCelInfo(const std::string& name, int& outCols, int& outRows) {
    std::string key = NormalizeKey(name);
    for (const auto& meta : sImageMetaTable) {
        if (key == meta.name) {
            outCols = meta.cols;
            outRows = meta.rows;
            return;
        }
    }
    outCols = 1;
    outRows = 1;
}

Texture TextureManager::LoadFromFiles(const std::string& colorPath, const std::string& maskPath, int cols, int rows) {
    Texture tex = { 0, 0, 0, cols, rows, false };

    SDL_Surface* srcSurface = IMG_Load(colorPath.c_str());
    if (!srcSurface) {
        std::cerr << "[TextureManager] Failed to load image: " << colorPath << " (" << IMG_GetError() << ")" << std::endl;
        return tex;
    }

    SDL_Surface* rgbaSurface = SDL_ConvertSurfaceFormat(srcSurface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(srcSurface);
    if (!rgbaSurface) {
        return tex;
    }

    tex.width = rgbaSurface->w;
    tex.height = rgbaSurface->h;

    // Apply alpha mask if present
    if (!maskPath.empty() && Vfs::Exists(maskPath)) {
        std::string resolvedMask = Vfs::Resolve(maskPath);
        SDL_Surface* maskSurface = IMG_Load(resolvedMask.c_str());
        if (maskSurface) {
            SDL_Surface* maskRgba = SDL_ConvertSurfaceFormat(maskSurface, SDL_PIXELFORMAT_RGBA32, 0);
            SDL_FreeSurface(maskSurface);

            if (maskRgba) {
                uint32_t* colorPixels = static_cast<uint32_t*>(rgbaSurface->pixels);
                uint32_t* maskPixels = static_cast<uint32_t*>(maskRgba->pixels);
                int count = std::min(rgbaSurface->w * rgbaSurface->h, maskRgba->w * maskRgba->h);

                for (int i = 0; i < count; ++i) {
                    uint8_t alpha = maskPixels[i] & 0xFF; // red channel as alpha
                    colorPixels[i] = (colorPixels[i] & 0x00FFFFFF) | (static_cast<uint32_t>(alpha) << 24);
                }
                SDL_FreeSurface(maskRgba);
                tex.hasAlpha = true;
            }
        }
    } else {
        tex.hasAlpha = true; // standard PNG already has alpha
    }

    // Upload to VitaGL
    glGenTextures(1, &tex.id);
    glBindTexture(GL_TEXTURE_2D, tex.id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width, tex.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaSurface->pixels);

    SDL_FreeSurface(rgbaSurface);
    return tex;
}

Texture* TextureManager::Load(const std::string& relativePath) {
    std::string key = NormalizeKey(relativePath);
    auto it = sTextures.find(key);
    if (it != sTextures.end()) {
        return &it->second;
    }

    int cols = 1, rows = 1;
    GetCelInfo(key, cols, rows);

    // Try finding the file across standard formats
    std::string colorPath = "";
    std::string maskPath = "";

    // Candidate base directories and names
    std::vector<std::string> candidates = {
        "Images/" + key,
        key,
        "Images/Backgrounds/" + key,
        "Fonts/" + key
    };

    // If original string had a full relative path that exists directly, prefer it
    if (Vfs::Exists(relativePath)) {
        colorPath = relativePath;
        size_t dot = relativePath.find_last_of('.');
        if (dot != std::string::npos) {
            std::string ext = relativePath.substr(dot);
            if (ext == ".jpg" || ext == ".JPG") {
                maskPath = relativePath.substr(0, dot) + "_.png";
            }
        }
    } else {
        for (const auto& base : candidates) {
            if (Vfs::Exists(base + ".png")) {
                colorPath = base + ".png";
                break;
            } else if (Vfs::Exists(base + ".jpg")) {
                colorPath = base + ".jpg";
                maskPath = base + "_.png";
                break;
            }
        }
    }

    if (colorPath.empty()) {
        std::cerr << "[TextureManager] File does not exist for asset: " << relativePath << " (key: " << key << ")" << std::endl;
        return nullptr;
    }

    std::string resolvedColor = Vfs::Resolve(colorPath);
    Texture tex = LoadFromFiles(resolvedColor, maskPath, cols, rows);
    if (tex.id == 0) {
        return nullptr;
    }

    sTextures[key] = tex;
    // Also map full path for direct hits
    sTextures[relativePath] = tex;
    return &sTextures[key];
}

Texture* TextureManager::Get(const std::string& relativePath) {
    std::string key = NormalizeKey(relativePath);
    auto it = sTextures.find(key);
    if (it != sTextures.end()) {
        return &it->second;
    }
    it = sTextures.find(relativePath);
    if (it != sTextures.end()) {
        return &it->second;
    }
    return Load(relativePath);
}

void TextureManager::Unload(const std::string& relativePath) {
    std::string key = NormalizeKey(relativePath);
    auto it = sTextures.find(key);
    if (it != sTextures.end()) {
        if (it->second.id != 0) {
            glDeleteTextures(1, &it->second.id);
        }
        sTextures.erase(it);
    }
}

void TextureManager::Clear() {
    for (auto& pair : sTextures) {
        if (pair.second.id != 0) {
            glDeleteTextures(1, &pair.second.id);
        }
    }
    sTextures.clear();
}

} // namespace HeavyWeapon
