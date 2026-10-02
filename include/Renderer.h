#pragma once

#include "Constants.h"
#include "TextureManager.h"
#include <vitaGL.h>

namespace HeavyWeapon {

struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
};

struct Color4f {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;

    static Color4f White() { return { 1.0f, 1.0f, 1.0f, 1.0f }; }
    static Color4f Red()   { return { 1.0f, 0.2f, 0.2f, 1.0f }; }
    static Color4f Green() { return { 0.2f, 1.0f, 0.2f, 1.0f }; }
    static Color4f Blue()  { return { 0.2f, 0.5f, 1.0f, 1.0f }; }
    static Color4f Yellow(){ return { 1.0f, 1.0f, 0.2f, 1.0f }; }
    static Color4f Black() { return { 0.0f, 0.0f, 0.0f, 1.0f }; }
};

class Renderer {
public:
    static void Init();
    static void BeginFrame();
    static void EndFrame();

    static void SetAdditiveBlend(bool additive);
    static void SetColor(const Color4f& color);

    // Draw texture rectangle
    static void DrawTexture(const Texture* tex, float dx, float dy);
    static void DrawTexture(const Texture* tex, float dx, float dy, float dw, float dh);
    static void DrawTexture(const Texture* tex, const Rect& dst, const Rect& src);
    
    // Draw rotated texture around center or custom pivot
    static void DrawTextureRotated(const Texture* tex, const Rect& dst, const Rect& src, float angleDegrees, float pivotX = -1.0f, float pivotY = -1.0f);

    // Draw primitives
    static void DrawRect(float x, float y, float w, float h, const Color4f& color);
    static void DrawFillRect(float x, float y, float w, float h, const Color4f& color);
    static void DrawLine(float x1, float y1, float x2, float y2, const Color4f& color, float width = 1.0f);

    // Screen Shake effect
    static void AddScreenShake(float intensity, float durationSeconds);
    static void UpdateScreenShake(float dt);

private:
    static Color4f sCurrentColor;
    static bool sIsAdditive;
    static float sShakeIntensity;
    static float sShakeTimer;
    static float sShakeOffsetX;
    static float sShakeOffsetY;
};

} // namespace HeavyWeapon
