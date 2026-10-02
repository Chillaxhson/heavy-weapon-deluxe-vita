#include "FontRenderer.h"
#include "Vfs.h"
#include <iostream>
#include <sstream>
#include <regex>

namespace HeavyWeapon {

std::unordered_map<std::string, Font> FontRenderer::sFonts;

void FontRenderer::Init() {
    LoadFont("Normal");
    LoadFont("Computer");
    LoadFont("Keypunch16");
    LoadFont("Outline");
    LoadFont("RubberStampLET20");
    LoadFont("RubberStampLET42");
    LoadFont("StationFont");
}

bool FontRenderer::LoadFont(const std::string& fontName) {
    if (sFonts.find(fontName) != sFonts.end()) {
        return true;
    }

    std::string txtPath = "Fonts/" + fontName + ".txt";
    std::string textData = Vfs::ReadTextFile(txtPath);
    if (textData.empty()) {
        std::cerr << "[FontRenderer] Could not read font file: " << txtPath << std::endl;
        return false;
    }

    // Load texture
    std::string pngPath = "Fonts/_" + fontName + ".png";
    if (!Vfs::Exists(pngPath)) {
        pngPath = "Fonts/" + fontName + ".png";
    }
    Texture* tex = TextureManager::Load(pngPath);
    if (!tex) {
        std::cerr << "[FontRenderer] Could not load font texture: " << pngPath << std::endl;
        return false;
    }

    Font font;
    font.name = fontName;
    font.texture = tex;

    // Parse CharList
    std::vector<char> chars;
    size_t charListPos = textData.find("Define CharList");
    if (charListPos != std::string::npos) {
        size_t openParen = textData.find('(', charListPos);
        size_t closeParen = textData.find(')', openParen);
        if (openParen != std::string::npos && closeParen != std::string::npos) {
            std::string sub = textData.substr(openParen, closeParen - openParen);
            for (size_t i = 0; i < sub.size(); ++i) {
                if (sub[i] == '\'') {
                    if (i + 1 < sub.size()) {
                        char c = sub[i + 1];
                        if (c == '\\' && i + 2 < sub.size()) {
                            c = sub[i + 2];
                            i++;
                        }
                        chars.push_back(c);
                        i += 2;
                    }
                }
            }
        }
    }

    // Parse WidthList
    std::vector<int> widths;
    size_t widthPos = textData.find("Define WidthList");
    if (widthPos != std::string::npos) {
        size_t openParen = textData.find('(', widthPos);
        size_t closeParen = textData.find(')', openParen);
        if (openParen != std::string::npos && closeParen != std::string::npos) {
            std::string sub = textData.substr(openParen + 1, closeParen - openParen - 1);
            std::stringstream ss(sub);
            int val;
            char comma;
            while (ss >> val) {
                widths.push_back(val);
                ss >> comma;
            }
        }
    }

    // Parse RectList
    std::vector<Rect> rects;
    size_t rectPos = textData.find("Define RectList");
    if (rectPos != std::string::npos) {
        size_t openParen = textData.find('(', rectPos);
        size_t closeParen = textData.find(';', openParen);
        if (openParen != std::string::npos && closeParen != std::string::npos) {
            std::string sub = textData.substr(openParen + 1, closeParen - openParen - 1);
            size_t p = 0;
            while ((p = sub.find('(', p)) != std::string::npos) {
                size_t ep = sub.find(')', p);
                if (ep == std::string::npos) break;
                std::string item = sub.substr(p + 1, ep - p - 1);
                std::stringstream ss(item);
                float rx, ry, rw, rh;
                char c;
                if (ss >> rx >> c >> ry >> c >> rw >> c >> rh) {
                    rects.push_back({ rx, ry, rw, rh });
                    if ((int)rh > font.height) font.height = (int)rh;
                }
                p = ep + 1;
            }
        }
    }

    // Parse OffsetList
    std::vector<std::pair<int, int>> offsets;
    size_t offsetPos = textData.find("Define OffsetList");
    if (offsetPos != std::string::npos) {
        size_t openParen = textData.find('(', offsetPos);
        size_t closeParen = textData.find(';', openParen);
        if (openParen != std::string::npos && closeParen != std::string::npos) {
            std::string sub = textData.substr(openParen + 1, closeParen - openParen - 1);
            size_t p = 0;
            while ((p = sub.find('(', p)) != std::string::npos) {
                size_t ep = sub.find(')', p);
                if (ep == std::string::npos) break;
                std::string item = sub.substr(p + 1, ep - p - 1);
                std::stringstream ss(item);
                int ox, oy;
                char c;
                if (ss >> ox >> c >> oy) {
                    offsets.push_back({ ox, oy });
                }
                p = ep + 1;
            }
        }
    }

    // Assemble glyphs
    size_t count = std::min({ chars.size(), widths.size(), rects.size() });
    for (size_t i = 0; i < count; ++i) {
        FontGlyph glyph;
        glyph.srcRect = rects[i];
        glyph.advance = widths[i];
        if (i < offsets.size()) {
            glyph.ox = offsets[i].first;
            glyph.oy = offsets[i].second;
        }
        font.glyphs[chars[i]] = glyph;
    }

    // Space character
    if (font.glyphs.find(' ') == font.glyphs.end()) {
        FontGlyph spaceGlyph;
        spaceGlyph.advance = font.height / 3;
        font.glyphs[' '] = spaceGlyph;
    }

    sFonts[fontName] = font;
    std::cout << "[FontRenderer] Loaded font " << fontName << " with " << font.glyphs.size() << " glyphs." << std::endl;
    return true;
}

float FontRenderer::GetStringWidth(const std::string& fontName, const std::string& text, float scale) {
    auto it = sFonts.find(fontName);
    if (it == sFonts.end()) {
        if (!sFonts.empty()) it = sFonts.begin();
        else return (float)text.size() * 10.0f * scale;
    }

    const Font& font = it->second;
    float width = 0.0f;
    for (char c : text) {
        auto git = font.glyphs.find(c);
        if (git != font.glyphs.end()) {
            width += (float)git->second.advance * scale;
        } else {
            width += (float)font.height * 0.4f * scale;
        }
    }
    return width;
}

float FontRenderer::GetStringHeight(const std::string& fontName, float scale) {
    auto it = sFonts.find(fontName);
    if (it != sFonts.end()) {
        return (float)it->second.height * scale;
    }
    return 20.0f * scale;
}

void FontRenderer::DrawString(const std::string& fontName, const std::string& text, float x, float y, const Color4f& color, float scale, TextAlign align) {
    auto it = sFonts.find(fontName);
    if (it == sFonts.end()) {
        if (!sFonts.empty()) it = sFonts.begin();
        else return;
    }

    const Font& font = it->second;
    if (!font.texture) return;

    float totalW = GetStringWidth(fontName, text, scale);
    float curX = x;
    if (align == ALIGN_CENTER) {
        curX = x - totalW * 0.5f;
    } else if (align == ALIGN_RIGHT) {
        curX = x - totalW;
    }

    Renderer::SetColor(color);

    for (char c : text) {
        auto git = font.glyphs.find(c);
        if (git != font.glyphs.end()) {
            const FontGlyph& g = git->second;
            if (g.srcRect.w > 0 && g.srcRect.h > 0) {
                Rect dst = {
                    curX + (float)g.ox * scale,
                    y + (float)g.oy * scale,
                    g.srcRect.w * scale,
                    g.srcRect.h * scale
                };
                Renderer::DrawTexture(font.texture, dst, g.srcRect);
            }
            curX += (float)g.advance * scale;
        } else {
            curX += (float)font.height * 0.4f * scale;
        }
    }

    Renderer::SetColor(Color4f::White());
}

} // namespace HeavyWeapon
