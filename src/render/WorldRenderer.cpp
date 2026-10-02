#include "WorldRenderer.h"
#include "Renderer.h"
#include <cmath>
#include <iostream>

namespace HeavyWeapon {

std::string WorldRenderer::sCurrentTheme = "";
float WorldRenderer::sScrollX = 0.0f;
Texture* WorldRenderer::sSkyTex = nullptr;
Texture* WorldRenderer::sBg2Tex = nullptr;
Texture* WorldRenderer::sBgTex = nullptr;
Texture* WorldRenderer::sGroundTex = nullptr;
std::vector<ActiveAnim> WorldRenderer::sAnims;

void WorldRenderer::Init() {
    sScrollX = 0.0f;
}

void WorldRenderer::SetTheme(const std::string& themeName, const std::vector<AnimDef>& levelAnims) {
    sCurrentTheme = themeName;
    sScrollX = 0.0f;

    std::string prefix = "Images/Backgrounds/" + themeName;
    sSkyTex = TextureManager::Load(prefix + "_sky.jpg");
    if (!sSkyTex) sSkyTex = TextureManager::Load(prefix + "_sky.png");

    sBg2Tex = TextureManager::Load(prefix + "_bg2.jpg");
    if (!sBg2Tex) sBg2Tex = TextureManager::Load(prefix + "_bg2.png");

    sBgTex = TextureManager::Load(prefix + "_bg.jpg");
    if (!sBgTex) sBgTex = TextureManager::Load(prefix + "_bg.png");

    sGroundTex = TextureManager::Load(prefix + "_ground.jpg");
    if (!sGroundTex) sGroundTex = TextureManager::Load(prefix + "_ground.png");

    sAnims.clear();
    for (const auto& def : levelAnims) {
        ActiveAnim anim;
        anim.def = def;
        anim.currentFrame = 0.0f;
        anim.worldX = (float)def.offset;
        anim.y = (float)def.y;
        anim.nuked = false;
        anim.visible = true;

        // Preload animation texture strip
        std::string animTexPath = "Images/Anims/" + def.name;
        TextureManager::Load(animTexPath);

        sAnims.push_back(anim);
    }
}

void WorldRenderer::Update(float dt, float scrollSpeed) {
    sScrollX += scrollSpeed * dt;

    for (auto& anim : sAnims) {
        // Advance frame
        float animSpeed = anim.def.speed;
        int curIntFrame = (int)anim.currentFrame;
        for (const auto& delay : anim.def.delays) {
            if (delay.frame == curIntFrame) {
                animSpeed = delay.speed;
                break;
            }
        }

        if (anim.def.type == "looping") {
            anim.currentFrame += animSpeed * POPCAP_TICKS_PER_SEC * dt;
            int maxFrames = anim.def.nuke ? anim.def.frames - 1 : anim.def.frames;
            if (maxFrames <= 0) maxFrames = 1;
            if (anim.currentFrame >= (float)maxFrames) {
                anim.currentFrame = std::fmod(anim.currentFrame, (float)maxFrames);
            }
        }

        // Horizontal velocity
        anim.worldX += anim.def.mx * POPCAP_TICKS_PER_SEC * dt;
    }
}

void WorldRenderer::TriggerNuke() {
    for (auto& anim : sAnims) {
        if (anim.def.nuke) {
            anim.nuked = true;
            anim.currentFrame = (float)(anim.def.frames - 1); // Last frame is nuked frame
        }
    }
}

void WorldRenderer::RenderPlane(Texture* tex, float scrollFactor, float yOffset, float planeH) {
    if (!tex || tex->id == 0) return;

    float texW = (float)tex->width;
    float texH = (float)tex->height;
    if (texW <= 0.0f) return;

    float effectiveScroll = std::fmod(sScrollX * scrollFactor, texW);
    if (effectiveScroll < 0.0f) effectiveScroll += texW;

    float startX = -effectiveScroll;
    while (startX < SCREEN_WIDTH) {
        float drawW = std::min(texW, (float)SCREEN_WIDTH - startX);
        Rect dst = { startX, yOffset, drawW, planeH };
        Rect src = { 0.0f, 0.0f, drawW * (texW / (float)tex->width), texH };
        Renderer::DrawTexture(tex, dst, src);
        startX += texW;
    }
}

static void RenderPlaneAnims(int plane, std::vector<ActiveAnim>& anims, float scrollX) {
    float scrollFactor = (plane == 4) ? 0.0f : (plane == 3) ? 0.25f : (plane == 2) ? 0.5f : 1.0f;

    for (auto& anim : anims) {
        if (anim.def.plane != plane || !anim.visible) continue;

        std::string animTexPath = "Images/Anims/" + anim.def.name;
        Texture* tex = TextureManager::Get(animTexPath);
        if (!tex) continue;

        float frameW = (float)tex->width / (float)anim.def.frames;
        float frameH = (float)tex->height;
        int curF = (int)anim.currentFrame;
        if (curF >= anim.def.frames) curF = anim.def.frames - 1;

        Rect src = { (float)curF * frameW, 0.0f, frameW, frameH };

        // Position relative to scrolling plane
        float screenX = anim.worldX - (scrollX * scrollFactor);
        // Wrap smoothly around screen
        float period = 960.0f;
        screenX = std::fmod(screenX, period);
        if (screenX < -frameW) screenX += period;

        Rect dst = { screenX - frameW * 0.5f, anim.y - frameH * 0.5f, frameW, frameH };
        Renderer::DrawTexture(tex, dst, src);
    }
}

void WorldRenderer::Render() {
    // Every plane is drawn at its native size. The sky is a full 640x480 frame and does
    // not scroll; the 300px background planes sit bottom-aligned on the ground strip.
    // Scroll factors are provisional until the original world update is decompiled.

    // Plane 4: Sky
    Renderer::DrawTexture(sSkyTex, 0.0f, 0.0f);
    RenderPlaneAnims(4, sAnims, sScrollX);

    // Plane 3: Far BG
    RenderPlane(sBg2Tex, 0.25f, BG_PLANE_Y, 300.0f);
    RenderPlaneAnims(3, sAnims, sScrollX);

    // Plane 2: Mid BG
    RenderPlane(sBgTex, 0.5f, BG_PLANE_Y, 300.0f);
    RenderPlaneAnims(2, sAnims, sScrollX);

    // Plane 1: Ground
    RenderPlane(sGroundTex, 1.0f, GROUND_PLANE_Y, 60.0f);
    RenderPlaneAnims(1, sAnims, sScrollX);
}

} // namespace HeavyWeapon
