#pragma once

#include "Renderer.h"
#include <string>
#include <unordered_map>

namespace HeavyWeapon {

enum TextAlign {
    ALIGN_LEFT = 0,
    ALIGN_CENTER,
    ALIGN_RIGHT
};

struct FontGlyph {
    Rect srcRect;
    int advance = 0;
    int ox = 0;
    int oy = 0;
};

struct Font {
    std::string name;
    Texture* texture = nullptr;
    int height = 20;
    std::unordered_map<char, FontGlyph> glyphs;
};

class FontRenderer {
public:
    static void Init();
    static bool LoadFont(const std::string& fontName);
    static void DrawString(const std::string& fontName, const std::string& text, float x, float y, const Color4f& color = Color4f::White(), float scale = 1.0f, TextAlign align = ALIGN_LEFT);
    static float GetStringWidth(const std::string& fontName, const std::string& text, float scale = 1.0f);
    static float GetStringHeight(const std::string& fontName, float scale = 1.0f);

private:
    static std::unordered_map<std::string, Font> sFonts;
};

} // namespace HeavyWeapon
