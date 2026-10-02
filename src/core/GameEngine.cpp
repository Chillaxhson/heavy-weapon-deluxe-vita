#include "GameEngine.h"
#include "Vfs.h"
#include "InputManager.h"
#include <cmath>
#include <iostream>
#include <algorithm>

namespace HeavyWeapon {

GameEngine::GameEngine() {}

GameEngine::~GameEngine() {
    Shutdown();
}

bool GameEngine::Init() {
    Vfs::Init();
    TextureManager::Init();
    Renderer::Init();
    FontRenderer::Init();
    AudioSystem::Init();
    InputManager::Init();

    // Load game definitions
    XmlLoader::LoadCrafts("data/craft.xml", mCraftDefs);
    XmlLoader::LoadLevels("data/levels.xml", mLevels);
    XmlLoader::LoadWaves("data/waves.xml", mLevels);
    XmlLoader::LoadBosses("data/bosses.xml", mBossDefs);
    XmlLoader::LoadAnims("Images/Anims/Anims.xml", mLevelAnims);

    // Preload common SFX
    for (const char* sfx : { "tankfire", "tankexplode", "bigexplode", "bullethit", "nukeblast",
                            "alert", "airraid", "flak", "boltcharge", "diesel", "buttondown" }) {
        AudioSystem::PreloadSound(sfx);
    }

    // Play title music
    AudioSystem::PlayMusic("Music/LoveTheme.ogg");

    mState = STATE_TITLE;
    return true;
}

void GameEngine::Shutdown() {
    AudioSystem::Shutdown();
    InputManager::Shutdown();
    TextureManager::Shutdown();
}

void GameEngine::Run() {
    uint32_t lastTicks = SDL_GetTicks();
    while (mRunning) {
        uint32_t currentTicks = SDL_GetTicks();
        float dt = (float)(currentTicks - lastTicks) / 1000.0f;
        if (dt > 0.05f) dt = 0.05f; // Cap at 20fps minimum to avoid spiral of death
        lastTicks = currentTicks;

        Update(dt);
        Render();
    }
}

void GameEngine::Update(float dt) {
    InputManager::Update();
    Renderer::UpdateScreenShake(dt);

    if (mNukeFlashAlpha > 0.0f) {
        mNukeFlashAlpha -= 1.5f * dt;
        if (mNukeFlashAlpha < 0.0f) mNukeFlashAlpha = 0.0f;
    }

    switch (mState) {
        case STATE_TITLE:          UpdateTitle(dt); break;
        case STATE_MISSION_SELECT: UpdateMissionSelect(dt); break;
        case STATE_PLAYING:        UpdatePlaying(dt); break;
        case STATE_ARMORY:         UpdateArmory(dt); break;
        case STATE_PAUSED:         UpdatePaused(dt); break;
        case STATE_GAMEOVER:       UpdateGameOver(dt); break;
        default: break;
    }
}

void GameEngine::UpdateTitle(float dt) {
    (void)dt;
    const InputState& input = InputManager::GetState();
    if (input.confirmPressed || input.pausePressed || input.touchPressed) {
        AudioSystem::PlaySound("buttondown");
        mState = STATE_MISSION_SELECT;
    }
}

void GameEngine::UpdateMissionSelect(float dt) {
    (void)dt;
    const InputState& input = InputManager::GetState();
    if (input.confirmPressed || input.pausePressed || input.touchPressed) {
        AudioSystem::PlaySound("buttondown");
        StartLevel(mCurrentLevelIndex);
    }
}

void GameEngine::StartLevel(int levelIndex) {
    mCurrentLevelIndex = levelIndex;
    mCurrentWaveIndex = 0;
    mWaveTimer = 0.0f;
    mLevelProgress = 0.0f;
    mSpawnTimer = 0.5f;
    mBossSpawned = false;

    mEnemies.clear();
    mProjectiles.clear();
    mParticles.clear();

    mPlayerTank.Init();

    std::string theme = "frigistan";
    if (mCurrentLevelIndex < (int)mLevels.size()) {
        theme = mLevels[mCurrentLevelIndex].bgTheme;
    }

    std::vector<AnimDef> anims;
    if (mCurrentLevelIndex < (int)mLevelAnims.size()) {
        anims = mLevelAnims[mCurrentLevelIndex];
    }

    WorldRenderer::SetTheme(theme, anims);

    AudioSystem::PlayMusic("Music/AtomicTank.mo3");
    AudioSystem::PlaySound("airraid");

    mState = STATE_PLAYING;
}

void GameEngine::SpawnNextEnemy() {
    if (mCurrentLevelIndex >= (int)mLevels.size()) return;
    const LevelDef& level = mLevels[mCurrentLevelIndex];
    if (mCurrentWaveIndex >= level.waves.size()) return;

    const WaveDef& wave = level.waves[mCurrentWaveIndex];
    if (wave.craftList.empty()) return;

    // Pick a craft entry from current wave
    int idx = rand() % wave.craftList.size();
    const WaveCraftEntry& entry = wave.craftList[idx];

    auto it = mCraftDefs.find(entry.craftId);
    if (it == mCraftDefs.end()) return;

    Enemy e;
    e.def = it->second;
    e.hp = e.def.armor;
    e.maxHp = e.def.armor;
    e.x = SCREEN_WIDTH + 80.0f;

    if (e.def.name == "TRUCK" || e.def.name == "ENEMYTANK" || e.def.name == "DOZER") {
        e.y = GROUND_Y - 22.0f;
        e.vx = -70.0f;
    } else {
        // Airborne enemy
        e.y = 80.0f + (float)(rand() % 240);
        e.vx = -140.0f - (float)(rand() % 80);
    }

    mEnemies.push_back(e);
}

void GameEngine::TriggerNuke() {
    if (mStats.nukes <= 0) return;
    mStats.nukes--;

    mNukeFlashAlpha = 1.0f;
    Renderer::AddScreenShake(25.0f, 1.2f);
    AudioSystem::PlaySound("nukeblast");
    AudioSystem::PlaySound("earthquake");

    WorldRenderer::TriggerNuke();

    // Destroy all enemies on screen and award points
    for (auto& e : mEnemies) {
        if (e.active) {
            e.active = false;
            mStats.score += e.def.points;
            SpawnExplosion(e.x, e.y, 40.0f);
        }
    }

    // Destroy all enemy projectiles
    for (auto& p : mProjectiles) {
        if (p.type != PROJ_PLAYER_CANNON && p.type != PROJ_PLAYER_MISSILE && p.type != PROJ_PLAYER_LASER) {
            p.active = false;
        }
    }
}

void GameEngine::SpawnExplosion(float x, float y, float size, bool isBoss) {
    (void)isBoss;
    AudioSystem::PlaySound((size > 30.0f) ? "bigexplode" : "bullethit");

    for (int i = 0; i < 16; ++i) {
        Particle p;
        p.x = x;
        p.y = y;
        float angle = (float)(rand() % 360) * 3.14159265f / 180.0f;
        float speed = 50.0f + (float)(rand() % 180);
        p.vx = std::cos(angle) * speed;
        p.vy = std::sin(angle) * speed;
        p.life = 0.4f + (float)(rand() % 40) / 100.0f;
        p.maxLife = p.life;
        p.size = size * (0.3f + (float)(rand() % 50) / 100.0f);
        p.color = { 1.0f, 0.4f + (float)(rand() % 60) / 100.0f, 0.1f, 1.0f };
        p.additive = true;
        mParticles.push_back(p);
    }
}

void GameEngine::UpdatePlaying(float dt) {
    const InputState& input = InputManager::GetState();

    if (input.pausePressed) {
        mState = STATE_PAUSED;
        return;
    }

    if (input.fireNukePressed) {
        TriggerNuke();
    }

    if (input.fireMegalaserPressed && mStats.megalaserCharge >= 100) {
        mStats.megalaserCharge = 0;
        mPlayerTank.megalaserTimer = 3.0f;
        AudioSystem::PlaySound("bosslaser");
    }

    // Advance world scroll
    float scrollSpeed = 80.0f;
    WorldRenderer::Update(dt, scrollSpeed);
    mLevelProgress += scrollSpeed * dt;

    // Player Tank update
    mPlayerTank.Update(dt, mStats, mProjectiles);

    // Enemy Spawning
    mSpawnTimer -= dt;
    if (mSpawnTimer <= 0.0f) {
        mSpawnTimer = 0.8f + (float)(rand() % 12) / 10.0f;
        SpawnNextEnemy();
    }

    // Wave Progression
    if (mCurrentLevelIndex < (int)mLevels.size()) {
        const LevelDef& level = mLevels[mCurrentLevelIndex];
        mWaveTimer += dt * 100.0f;
        if (mCurrentWaveIndex < level.waves.size()) {
            if (mWaveTimer >= (float)level.waves[mCurrentWaveIndex].length) {
                mWaveTimer = 0.0f;
                mCurrentWaveIndex++;
            }
        } else if (!mBossSpawned && mEnemies.empty()) {
            // Level complete! Transition to Armory
            mStats.availableUpgradePoints++;
            mStats.score += 5000;
            AudioSystem::PlaySound("alert");
            mState = STATE_ARMORY;
            return;
        }
    }

    // Update Enemies
    for (auto& e : mEnemies) {
        if (e.active) {
            e.Update(dt, mPlayerTank.x, mPlayerTank.y, mProjectiles);
        }
    }

    // Update Projectiles
    for (auto& p : mProjectiles) {
        if (!p.active) continue;

        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.life -= dt;
        if (p.life <= 0.0f || p.x < -50.0f || p.x > SCREEN_WIDTH + 50.0f || p.y < -50.0f || p.y > GROUND_Y + 20.0f) {
            p.active = false;
        }

        // Player Projectiles vs Enemies
        if (p.type == PROJ_PLAYER_CANNON || p.type == PROJ_PLAYER_MISSILE || p.type == PROJ_PLAYER_FLAK || p.type == PROJ_PLAYER_LASER) {
            for (auto& e : mEnemies) {
                if (!e.active) continue;
                Rect hb = e.GetHitbox();
                if (p.x >= hb.x && p.x <= hb.x + hb.w && p.y >= hb.y && p.y <= hb.y + hb.h) {
                    e.hp -= p.damage;
                    p.active = (p.type == PROJ_PLAYER_LASER); // Lasers pierce

                    // Fill megalaser meter on hit
                    if (mStats.megalaserCharge < 100) {
                        mStats.megalaserCharge += 1;
                    }

                    if (e.hp <= 0) {
                        e.active = false;
                        mStats.score += e.def.points;
                        SpawnExplosion(e.x, e.y, hb.w * 0.8f);
                    } else {
                        AudioSystem::PlaySound("bullethit", 0.5f);
                    }
                    break;
                }
            }
        }
        // Enemy Projectiles vs Player Tank
        else {
            Rect tankHb = mPlayerTank.GetHitbox();
            if (p.x >= tankHb.x && p.x <= tankHb.x + tankHb.w && p.y >= tankHb.y && p.y <= tankHb.y + tankHb.h) {
                p.active = false;
                if (!mPlayerTank.isInvulnerable) {
                    mPlayerTank.TakeDamage();
                    mStats.lives--;
                    if (mStats.lives <= 0) {
                        mState = STATE_GAMEOVER;
                        return;
                    }
                }
            }
        }
    }

    // Update Particles
    for (auto& pt : mParticles) {
        if (!pt.active) continue;
        pt.x += pt.vx * dt;
        pt.y += pt.vy * dt;
        pt.life -= dt;
        if (pt.life <= 0.0f) {
            pt.active = false;
        }
    }

    // Cleanup dead entities
    mEnemies.erase(std::remove_if(mEnemies.begin(), mEnemies.end(), [](const Enemy& e){ return !e.active; }), mEnemies.end());
    mProjectiles.erase(std::remove_if(mProjectiles.begin(), mProjectiles.end(), [](const Projectile& p){ return !p.active; }), mProjectiles.end());
    mParticles.erase(std::remove_if(mParticles.begin(), mParticles.end(), [](const Particle& p){ return !p.active; }), mParticles.end());
}

void GameEngine::UpdateArmory(float dt) {
    (void)dt;
    const InputState& input = InputManager::GetState();

    // Advance button pressed (Cross or Start or Touch on right side)
    if (input.confirmPressed || input.pausePressed) {
        AudioSystem::PlaySound("buttondown");
        mCurrentLevelIndex++;
        if (mCurrentLevelIndex >= NUM_CAMPAIGN_MISSIONS) {
            mState = STATE_TITLE; // Beat game!
        } else {
            mState = STATE_MISSION_SELECT;
        }
    }
}

void GameEngine::UpdatePaused(float dt) {
    (void)dt;
    const InputState& input = InputManager::GetState();
    if (input.pausePressed || input.confirmPressed) {
        mState = STATE_PLAYING;
    } else if (input.cancelPressed) {
        mState = STATE_TITLE;
        AudioSystem::PlayMusic("Music/LoveTheme.ogg");
    }
}

void GameEngine::UpdateGameOver(float dt) {
    (void)dt;
    const InputState& input = InputManager::GetState();
    if (input.confirmPressed || input.pausePressed || input.touchPressed) {
        mStats = PlayerStats(); // Reset stats
        mState = STATE_TITLE;
        AudioSystem::PlayMusic("Music/LoveTheme.ogg");
    }
}

// -------------------------------------------------------------
// Rendering implementations
// -------------------------------------------------------------

void GameEngine::Render() {
    Renderer::BeginFrame();

    switch (mState) {
        case STATE_TITLE:          RenderTitle(); break;
        case STATE_MISSION_SELECT: RenderMissionSelect(); break;
        case STATE_PLAYING:        RenderPlaying(); break;
        case STATE_ARMORY:         RenderArmory(); break;
        case STATE_PAUSED:         RenderPlaying(); RenderPaused(); break;
        case STATE_GAMEOVER:       RenderPlaying(); RenderGameOver(); break;
        default: break;
    }

    Renderer::EndFrame();
}

void GameEngine::RenderTitle() {
    Texture* titleTex = TextureManager::Get("Images/title.jpg");
    if (titleTex) {
        // Center 640x480 title on 960x544 screen
        float drawW = 725.0f;
        float drawH = 544.0f;
        float drawX = (SCREEN_WIDTH - drawW) * 0.5f;
        Renderer::DrawTexture(titleTex, drawX, 0.0f, drawW, drawH);
    }

    FontRenderer::DrawString("RubberStampLET20", "PRESS START / CROSS TO DEPLOY", SCREEN_WIDTH * 0.5f, 490.0f, { 1.0f, 0.9f, 0.2f, 1.0f }, 1.0f, ALIGN_CENTER);
}

void GameEngine::RenderMissionSelect() {
    Texture* menuTex = TextureManager::Get("Images/mainmenu.jpg");
    if (menuTex) {
        float drawW = 725.0f;
        float drawH = 544.0f;
        float drawX = (SCREEN_WIDTH - drawW) * 0.5f;
        Renderer::DrawTexture(menuTex, drawX, 0.0f, drawW, drawH);
    }

    // Mission brief box
    Renderer::DrawFillRect(120.0f, 60.0f, 720.0f, 420.0f, { 0.05f, 0.08f, 0.12f, 0.88f });
    Renderer::DrawRect(120.0f, 60.0f, 720.0f, 420.0f, { 0.3f, 0.6f, 0.9f, 1.0f });

    if (mCurrentLevelIndex < (int)mLevels.size()) {
        const LevelDef& level = mLevels[mCurrentLevelIndex];
        std::string title = "MISSION " + std::to_string(mCurrentLevelIndex + 1) + ": " + level.name;
        FontRenderer::DrawString("RubberStampLET20", title, SCREEN_WIDTH * 0.5f, 90.0f, { 1.0f, 0.3f, 0.2f, 1.0f }, 1.1f, ALIGN_CENTER);

        float textY = 160.0f;
        for (const auto& intel : level.intelList) {
            FontRenderer::DrawString("Normal", intel.text, 160.0f, textY, { 0.9f, 0.9f, 0.9f, 1.0f }, 1.0f, ALIGN_LEFT);
            textY += 90.0f;
        }
    }

    FontRenderer::DrawString("Computer", "PRESS CROSS TO COMMENCE ATTACK", SCREEN_WIDTH * 0.5f, 440.0f, { 0.2f, 1.0f, 0.3f, 1.0f }, 1.0f, ALIGN_CENTER);
}

void GameEngine::RenderPlaying() {
    // Parallax background
    WorldRenderer::Render();

    // Player Tank
    mPlayerTank.Render(mStats);

    // Enemies
    for (auto& e : mEnemies) {
        if (e.active) {
            e.Render();
        }
    }

    // Projectiles
    for (const auto& p : mProjectiles) {
        if (!p.active) continue;
        if (p.type == PROJ_PLAYER_CANNON) {
            Renderer::DrawFillRect(p.x - 3.0f, p.y - 3.0f, 6.0f, 6.0f, { 1.0f, 0.9f, 0.3f, 1.0f });
        } else if (p.type == PROJ_PLAYER_MISSILE) {
            Renderer::DrawFillRect(p.x - 4.0f, p.y - 4.0f, 8.0f, 8.0f, { 1.0f, 0.4f, 0.2f, 1.0f });
        } else if (p.type == PROJ_ENEMY_BULLET) {
            Renderer::DrawFillRect(p.x - 3.0f, p.y - 3.0f, 6.0f, 6.0f, { 1.0f, 0.2f, 0.2f, 1.0f });
        } else if (p.type == PROJ_ENEMY_BOMB || p.type == PROJ_ENEMY_ARMORED_BOMB) {
            Renderer::DrawFillRect(p.x - 5.0f, p.y - 6.0f, 10.0f, 12.0f, { 0.8f, 0.8f, 0.2f, 1.0f });
        }
    }

    // Particles
    for (const auto& pt : mParticles) {
        if (!pt.active) continue;
        Renderer::SetAdditiveBlend(pt.additive);
        float alpha = pt.life / pt.maxLife;
        Color4f col = pt.color;
        col.a = alpha;
        Renderer::DrawFillRect(pt.x - pt.size * 0.5f, pt.y - pt.size * 0.5f, pt.size, pt.size, col);
    }
    Renderer::SetAdditiveBlend(false);

    // Nuke Screen Flash
    if (mNukeFlashAlpha > 0.0f) {
        Renderer::DrawFillRect(0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, { 1.0f, 1.0f, 1.0f, mNukeFlashAlpha });
    }

    // HUD
    RenderHUD();
}

void GameEngine::RenderHUD() {
    // Score
    std::string scoreStr = "SCORE: " + std::to_string(mStats.score);
    FontRenderer::DrawString("Normal", scoreStr, 20.0f, 15.0f, { 1.0f, 1.0f, 1.0f, 1.0f });

    // Lives icons
    Texture* tankIcon = TextureManager::Get("Images/tankicon.png");
    for (int i = 0; i < mStats.lives; ++i) {
        if (tankIcon) {
            Renderer::DrawTexture(tankIcon, 20.0f + (float)i * 28.0f, 40.0f);
        } else {
            Renderer::DrawFillRect(20.0f + (float)i * 24.0f, 40.0f, 18.0f, 12.0f, { 0.2f, 0.8f, 0.3f, 1.0f });
        }
    }

    // Nukes icons
    Texture* nukeIcon = TextureManager::Get("Images/nukeicon.png");
    for (int i = 0; i < mStats.nukes; ++i) {
        if (nukeIcon) {
            Renderer::DrawTexture(nukeIcon, SCREEN_WIDTH - 40.0f - (float)i * 32.0f, 15.0f);
        } else {
            Renderer::DrawFillRect(SCREEN_WIDTH - 40.0f - (float)i * 25.0f, 15.0f, 20.0f, 15.0f, { 1.0f, 0.8f, 0.2f, 1.0f });
        }
    }

    // Megameter bar
    float meterW = 120.0f;
    float fillW = meterW * ((float)mStats.megalaserCharge / 100.0f);
    Renderer::DrawRect(SCREEN_WIDTH - 140.0f, 45.0f, meterW, 10.0f, { 0.5f, 0.5f, 0.5f, 1.0f });
    Renderer::DrawFillRect(SCREEN_WIDTH - 140.0f, 45.0f, fillW, 10.0f, { 0.2f, 0.6f, 1.0f, 0.9f });
    FontRenderer::DrawString("Normal", "MEGA", SCREEN_WIDTH - 185.0f, 42.0f, { 0.2f, 0.6f, 1.0f, 1.0f }, 0.8f);
}

void GameEngine::RenderArmory() {
    Texture* armoryTex = TextureManager::Get("Images/armory.jpg");
    if (armoryTex) {
        float drawW = 725.0f;
        float drawH = 544.0f;
        float drawX = (SCREEN_WIDTH - drawW) * 0.5f;
        Renderer::DrawTexture(armoryTex, drawX, 0.0f, drawW, drawH);
    } else {
        Renderer::DrawFillRect(0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, { 0.1f, 0.12f, 0.15f, 1.0f });
    }

    FontRenderer::DrawString("RubberStampLET20", "ARMORY UPGRADE STATION", SCREEN_WIDTH * 0.5f, 50.0f, { 1.0f, 0.8f, 0.2f, 1.0f }, 1.1f, ALIGN_CENTER);

    std::string pointsText = "UPGRADE POINTS AVAILABLE: " + std::to_string(mStats.availableUpgradePoints);
    FontRenderer::DrawString("Normal", pointsText, SCREEN_WIDTH * 0.5f, 95.0f, { 0.2f, 1.0f, 0.4f, 1.0f }, 1.0f, ALIGN_CENTER);

    // Weapon items list
    const char* weaponNames[WEAPON_COUNT] = {
        "HEAVY CANNON", "DEFENSE PODS", "HOMING MISSILES", "LASER CANNON", "FLAK SHELLS", "THUNDERSTRIKE"
    };

    for (int i = 0; i < WEAPON_COUNT; ++i) {
        float rowY = 140.0f + (float)i * 50.0f;
        std::string label = std::string(weaponNames[i]) + " [LVL " + std::to_string(mStats.weaponLevels[i]) + "]";
        FontRenderer::DrawString("Normal", label, 200.0f, rowY, { 1.0f, 1.0f, 1.0f, 1.0f });

        // Level pips
        for (int p = 0; p < 3; ++p) {
            Color4f pipCol = (p < mStats.weaponLevels[i]) ? Color4f{ 0.2f, 0.9f, 0.3f, 1.0f } : Color4f{ 0.3f, 0.3f, 0.3f, 1.0f };
            Renderer::DrawFillRect(500.0f + (float)p * 25.0f, rowY, 18.0f, 14.0f, pipCol);
        }
    }

    // Advance button
    Texture* advanceTex = TextureManager::Get("Images/advancebtn.jpg");
    if (advanceTex) {
        Renderer::DrawTexture(advanceTex, SCREEN_WIDTH - 200.0f, SCREEN_HEIGHT - 80.0f);
    }
    FontRenderer::DrawString("Computer", "PRESS CROSS TO CONTINUE", SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT - 40.0f, { 0.9f, 0.9f, 0.2f, 1.0f }, 1.0f, ALIGN_CENTER);
}

void GameEngine::RenderPaused() {
    Renderer::DrawFillRect(0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, { 0.0f, 0.0f, 0.0f, 0.65f });
    FontRenderer::DrawString("RubberStampLET42", "PAUSED", SCREEN_WIDTH * 0.5f, 220.0f, { 1.0f, 1.0f, 1.0f, 1.0f }, 1.0f, ALIGN_CENTER);
    FontRenderer::DrawString("Normal", "START: RESUME    CIRCLE: RETIRE", SCREEN_WIDTH * 0.5f, 300.0f, { 0.8f, 0.8f, 0.8f, 1.0f }, 1.0f, ALIGN_CENTER);
}

void GameEngine::RenderGameOver() {
    Renderer::DrawFillRect(0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, { 0.3f, 0.05f, 0.05f, 0.8f });
    FontRenderer::DrawString("RubberStampLET42", "MISSION FAILED", SCREEN_WIDTH * 0.5f, 200.0f, { 1.0f, 0.2f, 0.2f, 1.0f }, 1.0f, ALIGN_CENTER);

    std::string scoreStr = "FINAL SCORE: " + std::to_string(mStats.score);
    FontRenderer::DrawString("Normal", scoreStr, SCREEN_WIDTH * 0.5f, 280.0f, { 1.0f, 1.0f, 1.0f, 1.0f }, 1.2f, ALIGN_CENTER);

    FontRenderer::DrawString("Computer", "PRESS CROSS TO RETURN", SCREEN_WIDTH * 0.5f, 380.0f, { 1.0f, 0.9f, 0.2f, 1.0f }, 1.0f, ALIGN_CENTER);
}

} // namespace HeavyWeapon
