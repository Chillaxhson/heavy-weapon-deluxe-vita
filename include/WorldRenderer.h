#pragma once

#include "DataModels.h"
#include "TextureManager.h"
#include <string>
#include <vector>

namespace HeavyWeapon {

class WorldRenderer {
public:
    static void Init();
    static void SetTheme(const std::string& themeName, const std::vector<AnimDef>& levelAnims);
    static void Update(float dt, float scrollSpeed);
    static void Render();

    // Trigger nuke effect across world animations
    static void TriggerNuke();

    static float GetScrollOffset() { return sScrollX; }

private:
    static std::string sCurrentTheme;
    static float sScrollX;

    // Background plane textures
    static Texture* sSkyTex;
    static Texture* sBg2Tex; // Far BG
    static Texture* sBgTex;  // Mid BG
    static Texture* sGroundTex;

    // Active ambient animations
    static std::vector<ActiveAnim> sAnims;

    static void RenderPlane(Texture* tex, float scrollFactor, float yOffset, float planeH);
};

} // namespace HeavyWeapon
