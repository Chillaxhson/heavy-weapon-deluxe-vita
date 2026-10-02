#include "TextureManager.h"
#include "Vfs.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <vector>

namespace HeavyWeapon {

std::unordered_map<std::string, Texture> TextureManager::sTextures;

void TextureManager::Init() {
    IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
}

void TextureManager::Shutdown() {
    Clear();
    IMG_Quit();
}

Texture TextureManager::LoadFromFiles(const std::string& colorPath, const std::string& maskPath) {
    Texture tex = { 0, 0, 0, false };

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
                    uint8_t alpha = maskPixels[i] & 0xFF; // Use red channel of mask as alpha
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
    auto it = sTextures.find(relativePath);
    if (it != sTextures.end()) {
        return &it->second;
    }

    std::string colorPath = relativePath;
    std::string maskPath = "";

    // Check if path has extension
    size_t dotPos = relativePath.find_last_of('.');
    std::string ext = (dotPos != std::string::npos) ? relativePath.substr(dotPos) : "";
    std::string base = (dotPos != std::string::npos) ? relativePath.substr(0, dotPos) : relativePath;

    if (ext == ".jpg" || ext == ".JPG") {
        maskPath = base + "_.png";
    } else if (ext.empty()) {
        if (Vfs::Exists(base + ".png")) {
            colorPath = base + ".png";
        } else if (Vfs::Exists(base + ".jpg")) {
            colorPath = base + ".jpg";
            maskPath = base + "_.png";
        }
    }

    std::string resolvedColor = Vfs::Resolve(colorPath);
    if (!Vfs::Exists(resolvedColor)) {
        std::cerr << "[TextureManager] File does not exist: " << colorPath << std::endl;
        return nullptr;
    }

    Texture tex = LoadFromFiles(resolvedColor, maskPath);
    if (tex.id == 0) {
        return nullptr;
    }

    sTextures[relativePath] = tex;
    return &sTextures[relativePath];
}

Texture* TextureManager::Get(const std::string& relativePath) {
    auto it = sTextures.find(relativePath);
    if (it != sTextures.end()) {
        return &it->second;
    }
    return Load(relativePath);
}

void TextureManager::Unload(const std::string& relativePath) {
    auto it = sTextures.find(relativePath);
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
