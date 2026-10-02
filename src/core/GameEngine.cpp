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

    // Preload essential PopCap audio
    for (const char* sfx : {
        "tankfire1", "tankfire2", "tankfire3", "tankfire4",
        "tankexplode", "bigexplode", "smallexplode", "bullethit",
        "nukeblast", "alert", "airraid", "flak", "missile", "bombfall",
        "laser", "megalaser", "upgrade", "gunpowerup", "laserpowerup",
        "diesel", "buttondown", "buttonup", "stats",
        "v_atomictank", "v_getready", "v_levelcomplete", "v_gameover",
        "v_megalaser", "v_prepare", "v_danger"
    }) {
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
        if (dt > 0.05f) dt = 0.05f; // Cap at 20fps minimum
        lastTicks = currentTicks;

        Update(dt);
        Render();
    }
}

void GameEngine::Update(float dt) {
    InputManager::Update();
    Renderer::UpdateScreenShake(dt);

    if (mNukeFlashAlpha > 0.0f) {
        mNukeFlashAlpha -= 1.6f * dt;
        if (mNukeFlashAlpha < 0.0f) mNukeFlashAlpha = 0.0f;
    }

    if (mMushCloudTimer > 0.0f) {
        mMushCloudTimer -= dt;
    }

    mMenuGlowAnim += 3.0f * dt;

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

    if (input.upPressed) {
        mMenuSelection = (mMenuSelection - 1 + 3) % 3;
        AudioSystem::PlaySound("stats", 0.5f);
    } else if (input.downPressed) {
        mMenuSelection = (mMenuSelection + 1) % 3;
        AudioSystem::PlaySound("stats", 0.5f);
    }

    if (input.confirmPressed || input.pausePressed || input.touchPressed) {
        AudioSystem::PlaySound("buttondown");
        if (mMenuSelection == 0) {
            // Mission Campaign
            mCurrentLevelIndex = 0;
            mStats = PlayerStats();
            mState = STATE_MISSION_SELECT;
        } else if (mMenuSelection == 1) {
            // Survival Mode
            mCurrentLevelIndex = 0;
            mStats = PlayerStats();
            StartLevel(0);
        } else {
            // Deploy directly
            mCurrentLevelIndex = 0;
            mStats = PlayerStats();
            mState = STATE_MISSION_SELECT;
        }
    }
}

void GameEngine::UpdateMissionSelect(float dt) {
    (void)dt;
    const InputState& input = InputManager::GetState();

    if (input.confirmPressed || input.pausePressed || input.touchPressed) {
        AudioSystem::PlaySound("buttondown");
        AudioSystem::PlaySound("v_atomictank");
        StartLevel(mCurrentLevelIndex);
    } else if (input.cancelPressed) {
        AudioSystem::PlaySound("buttonup");
        mState = STATE_TITLE;
    }
}

void GameEngine::StartLevel(int levelIndex) {
    mCurrentLevelIndex = levelIndex;
    mCurrentWaveIndex = 0;
    mWaveTimer = 0.0f;
    mLevelProgress = 0.0f;
    mSpawnTimer = 0.5f;
    mBossSpawned = false;

    // Approximate total level scroll distance from wave durations
    float totalWaveLen = 60.0f;
    if (mCurrentLevelIndex < (int)mLevels.size()) {
        totalWaveLen = 0.0f;
        for (const auto& w : mLevels[mCurrentLevelIndex].waves) {
            totalWaveLen += (float)w.length;
        }
        if (totalWaveLen <= 0.0f) totalWaveLen = 80.0f;
    }
    mLevelLength = totalWaveLen * 70.0f; // 70 px/sec scroll speed

    mEnemies.clear();
    mProjectiles.clear();
    mExplosions.clear();
    mCraters.clear();
    mPowerUps.clear();
    mParticles.clear();

    mPlayerTank.Init();

    std::string theme = "antagonistan";
    if (mCurrentLevelIndex < (int)mLevels.size()) {
        theme = mLevels[mCurrentLevelIndex].bgTheme;
    }

    std::vector<AnimDef> anims;
    if (mCurrentLevelIndex < (int)mLevelAnims.size()) {
        anims = mLevelAnims[mCurrentLevelIndex];
    }

    WorldRenderer::SetTheme(theme, anims);

    AudioSystem::PlayMusic("Music/AtomicTank.mo3");
    AudioSystem::PlaySound("v_getready");
    AudioSystem::PlaySound("airraid");

    mState = STATE_PLAYING;
}

void GameEngine::SpawnNextEnemy() {
    if (mCurrentLevelIndex >= (int)mLevels.size()) return;
    const LevelDef& level = mLevels[mCurrentLevelIndex];
    if (mCurrentWaveIndex >= level.waves.size()) return;

    const WaveDef& wave = level.waves[mCurrentWaveIndex];
    if (wave.craftList.empty()) return;

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
        e.y = GROUND_Y - 24.0f;
        e.vx = -85.0f;
    } else {
        // Airborne enemy
        e.y = 80.0f + (float)(rand() % 230);
        e.vx = -140.0f - (float)(rand() % 90);
    }

    mEnemies.push_back(e);
}

void GameEngine::SpawnExplosion(float x, float y, float size, bool isNuke) {
    ExplosionInstance exp;
    exp.x = x;
    exp.y = y;
    exp.animFrame = 0.0f;
    exp.scale = size;
    exp.isNuke = isNuke;
    mExplosions.push_back(exp);

    Renderer::AddScreenShake(isNuke ? 30.0f : (10.0f * size), isNuke ? 1.0f : 0.25f);
    AudioSystem::PlaySound(isNuke ? "nukeblast" : (size > 1.2f ? "bigexplode" : "smallexplode"));
}

void GameEngine::TriggerNuke() {
    if (mStats.nukes <= 0) return;
    mStats.nukes--;

    mNukeFlashAlpha = 1.0f;
    mMushCloudTimer = 2.5f;
    mMushCloudX = mPlayerTank.x;

    Renderer::AddScreenShake(30.0f, 1.2f);
    AudioSystem::PlaySound("nukeblast");
    AudioSystem::PlaySound("earthquake");

    WorldRenderer::TriggerNuke();

    // Destroy all enemies on screen
    for (auto& e : mEnemies) {
        if (e.active) {
            e.active = false;
            mStats.score += e.def.points;
            SpawnExplosion(e.x, e.y, 1.8f);
        }
    }

    // Destroy all enemy projectiles
    for (auto& p : mProjectiles) {
        if (p.type != PROJ_PLAYER_CANNON && p.type != PROJ_PLAYER_MISSILE && p.type != PROJ_PLAYER_LASER) {
            p.active = false;
        }
    }
}

void GameEngine::UpdatePlaying(float dt) {
    const InputState& input = InputManager::GetState();

    if (input.pausePressed) {
        AudioSystem::UpdateEngineSound(false);
        mState = STATE_PAUSED;
        return;
    }

    if (input.fireNukePressed) {
        TriggerNuke();
    }

    if (input.fireMegalaserPressed && mStats.megalaserCharge >= 100) {
        mStats.megalaserCharge = 0;
        mPlayerTank.megalaserTimer = 3.0f;
        AudioSystem::PlaySound("v_megalaser");
        AudioSystem::PlaySound("bosslaser");
    }

    // Engine diesel sound loop
    bool moving = (std::abs(mPlayerTank.vx) > 10.0f);
    AudioSystem::UpdateEngineSound(moving);

    // Advance world scroll
    float scrollSpeed = 70.0f;
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
        mWaveTimer += dt;
        if (mCurrentWaveIndex < level.waves.size()) {
            if (mWaveTimer >= (float)level.waves[mCurrentWaveIndex].length) {
                mWaveTimer = 0.0f;
                mCurrentWaveIndex++;
            }
        } else if (!mBossSpawned && mEnemies.empty()) {
            // Level complete!
            AudioSystem::UpdateEngineSound(false);
            mStats.availableUpgradePoints++;
            mStats.score += 5000;
            AudioSystem::PlaySound("v_levelcomplete");
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
        p.animFrame += 20.0f * dt;
        p.life -= dt;

        // Ground bomb impact
        if ((p.type == PROJ_ENEMY_BOMB || p.type == PROJ_ENEMY_ARMORED_BOMB ||
             p.type == PROJ_ENEMY_FRAG_BOMB || p.type == PROJ_ENEMY_FATBOY) && p.y >= GROUND_Y - 5.0f) {
            p.active = false;
            SpawnExplosion(p.x, GROUND_Y - 10.0f, (p.type == PROJ_ENEMY_FATBOY) ? 2.0f : 1.0f);

            // Ground crater
            CraterInstance cr;
            cr.x = p.x;
            cr.y = GROUND_Y - 5.0f;
            cr.frame = rand() % 5;
            mCraters.push_back(cr);
            continue;
        }

        if (p.life <= 0.0f || p.x < -60.0f || p.x > SCREEN_WIDTH + 60.0f || p.y < -60.0f || p.y > GROUND_Y + 30.0f) {
            p.active = false;
        }

        // Player Projectiles vs Enemies
        if (p.type == PROJ_PLAYER_CANNON || p.type == PROJ_PLAYER_MISSILE || p.type == PROJ_PLAYER_FLAK || p.type == PROJ_PLAYER_LASER) {
            for (auto& e : mEnemies) {
                if (!e.active) continue;
                Rect hb = e.GetHitbox();
                if (p.x >= hb.x && p.x <= hb.x + hb.w && p.y >= hb.y && p.y <= hb.y + hb.h) {
                    e.TakeDamage(p.damage);
                    p.active = (p.type == PROJ_PLAYER_LASER); // Lasers pierce

                    if (mStats.megalaserCharge < 100) {
                        mStats.megalaserCharge += 1;
                    }

                    if (e.hp <= 0) {
                        e.active = false;
                        mStats.score += e.def.points;
                        SpawnExplosion(e.x, e.y, std::clamp(hb.w * 0.02f, 0.8f, 2.2f));

                        // Chance to drop power-up supply
                        if (rand() % 6 == 0) {
                            PowerUpItem pup;
                            pup.x = e.x;
                            pup.y = e.y;
                            pup.cel = rand() % 12;
                            pup.isNuke = (rand() % 8 == 0);
                            mPowerUps.push_back(pup);
                        }
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
                    SpawnExplosion(p.x, p.y, 1.2f);
                    mStats.lives--;
                    if (mStats.lives <= 0) {
                        AudioSystem::UpdateEngineSound(false);
                        AudioSystem::PlaySound("v_gameover");
                        mState = STATE_GAMEOVER;
                        return;
                    }
                }
            }
        }
    }

    // Update Explosions
    for (auto& exp : mExplosions) {
        if (exp.active) {
            exp.Update(dt);
        }
    }

    // Update PowerUps
    for (auto& pup : mPowerUps) {
        if (!pup.active) continue;
        pup.y += pup.vy * dt;
        if (pup.y >= GROUND_Y - 15.0f) {
            pup.y = GROUND_Y - 15.0f;
        }

        // Collect powerup
        Rect tankHb = mPlayerTank.GetHitbox();
        if (pup.x >= tankHb.x && pup.x <= tankHb.x + tankHb.w && pup.y >= tankHb.y && pup.y <= tankHb.y + tankHb.h + 20.0f) {
            pup.active = false;
            if (pup.isNuke) {
                if (mStats.nukes < MAX_NUKES) mStats.nukes++;
                AudioSystem::PlaySound("powerup");
            } else {
                mStats.score += 250;
                AudioSystem::PlaySound("gunpowerup");
            }
        }
    }

    // Cleanup dead entities
    mEnemies.erase(std::remove_if(mEnemies.begin(), mEnemies.end(), [](const Enemy& e){ return !e.active; }), mEnemies.end());
    mProjectiles.erase(std::remove_if(mProjectiles.begin(), mProjectiles.end(), [](const Projectile& p){ return !p.active; }), mProjectiles.end());
    mExplosions.erase(std::remove_if(mExplosions.begin(), mExplosions.end(), [](const ExplosionInstance& exp){ return !exp.active; }), mExplosions.end());
    mPowerUps.erase(std::remove_if(mPowerUps.begin(), mPowerUps.end(), [](const PowerUpItem& pup){ return !pup.active; }), mPowerUps.end());
}

void GameEngine::UpdateArmory(float dt) {
    (void)dt;
    const InputState& input = InputManager::GetState();

    if (input.upPressed) {
        mArmorySelection = (mArmorySelection - 1 + WEAPON_COUNT) % WEAPON_COUNT;
        AudioSystem::PlaySound("stats", 0.5f);
    } else if (input.downPressed) {
        mArmorySelection = (mArmorySelection + 1) % WEAPON_COUNT;
        AudioSystem::PlaySound("stats", 0.5f);
    }

    // Upgrade weapon
    if (input.rightPressed || input.altFirePressed) {
        int maxLvl = (mArmorySelection == WEAPON_CANNON) ? 5 : 3;
        if (mStats.availableUpgradePoints > 0 && mStats.weaponLevels[mArmorySelection] < maxLvl) {
            mStats.weaponLevels[mArmorySelection]++;
            mStats.availableUpgradePoints--;
            AudioSystem::PlaySound("upgrade");
        } else {
            AudioSystem::PlaySound("denied", 0.5f);
        }
    }
    // Downgrade weapon
    else if (input.leftPressed) {
        int minLvl = (mArmorySelection == WEAPON_CANNON) ? 1 : 0;
        if (mStats.weaponLevels[mArmorySelection] > minLvl) {
            mStats.weaponLevels[mArmorySelection]--;
            mStats.availableUpgradePoints++;
            AudioSystem::PlaySound("buttonup");
        }
    }

    // Advance to next mission
    if (input.confirmPressed || input.pausePressed) {
        AudioSystem::PlaySound("buttondown");
        mCurrentLevelIndex++;
        if (mCurrentLevelIndex >= NUM_CAMPAIGN_MISSIONS) {
            mState = STATE_TITLE; // Victory!
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
        mStats = PlayerStats();
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
    Texture* titleTex = TextureManager::Get("mainmenu");
    if (titleTex) {
        // Center 640x480 title on 960x544 screen
        float drawW = 725.0f;
        float drawH = 544.0f;
        float drawX = (SCREEN_WIDTH - drawW) * 0.5f;
        Renderer::DrawTexture(titleTex, drawX, 0.0f, drawW, drawH);
    }

    // Main Menu Buttons
    const char* options[] = { "CAMPAIGN MISSION", "SURVIVAL MODE", "WAR ROOM INTEL" };
    Texture* btnTex = TextureManager::Get("mainbigbtn");
    Texture* glowTex = TextureManager::Get("mainglow");

    float startY = 160.0f;
    for (int i = 0; i < 3; ++i) {
        float btnY = startY + (float)i * 90.0f;
        float btnX = SCREEN_WIDTH * 0.5f;

        if (mMenuSelection == i && glowTex) {
            Renderer::SetAdditiveBlend(true);
            float pulse = 0.7f + 0.3f * std::sin(mMenuGlowAnim);
            Renderer::SetColor({ 1.0f, 0.8f, 0.2f, pulse });
            Renderer::DrawTexture(glowTex, (SCREEN_WIDTH - 725.0f) * 0.5f, 0.0f, 725.0f, 544.0f);
            Renderer::SetColor(Color4f::White());
            Renderer::SetAdditiveBlend(false);
        }

        if (btnTex) {
            Renderer::DrawCel(btnTex, 0, 0, btnX, btnY, true, 1.2f, 0.7f);
        }

        Color4f textCol = (mMenuSelection == i) ? Color4f{ 1.0f, 0.9f, 0.1f, 1.0f } : Color4f{ 0.8f, 0.8f, 0.8f, 1.0f };
        FontRenderer::DrawString("RubberStampLET20", options[i], btnX, btnY - 10.0f, textCol, 1.0f, ALIGN_CENTER);
    }

    FontRenderer::DrawString("Computer", "D-PAD: SELECT    CROSS: COMMENCE", SCREEN_WIDTH * 0.5f, 495.0f, { 0.2f, 1.0f, 0.4f, 1.0f }, 1.0f, ALIGN_CENTER);
}

void GameEngine::RenderMissionSelect() {
    // Strategic War Room tactical map (map.jpg)
    Texture* mapTex = TextureManager::Get("map");
    if (mapTex) {
        float drawW = 725.0f;
        float drawH = 544.0f;
        float drawX = (SCREEN_WIDTH - drawW) * 0.5f;
        Renderer::DrawTexture(mapTex, drawX, 0.0f, drawW, drawH);
    }

    // Country overlay
    std::string missionMapName = "mission" + std::to_string(std::clamp(mCurrentLevelIndex + 1, 1, 10));
    Texture* missionTex = TextureManager::Get(missionMapName);
    if (missionTex) {
        Renderer::SetAdditiveBlend(true);
        float pulse = 0.6f + 0.4f * std::sin(mMenuGlowAnim * 1.5f);
        Renderer::SetColor({ 1.0f, 0.2f, 0.2f, pulse });
        Renderer::DrawTexture(missionTex, (SCREEN_WIDTH - 725.0f) * 0.5f, 0.0f, 725.0f, 544.0f);
        Renderer::SetColor(Color4f::White());
        Renderer::SetAdditiveBlend(false);
    }

    // Animated target indicator (mappointer.png)
    Texture* pointerTex = TextureManager::Get("mappointer");
    if (pointerTex) {
        float ptrX = SCREEN_WIDTH * 0.5f + std::sin(mMenuGlowAnim) * 10.0f;
        float ptrY = 220.0f;
        Renderer::DrawCel(pointerTex, 0, 0, ptrX, ptrY, true, 2.5f, 2.5f);
    }

    // Mission brief tactical overlay
    Renderer::DrawFillRect(140.0f, 320.0f, 680.0f, 160.0f, { 0.04f, 0.06f, 0.09f, 0.90f });
    Renderer::DrawRect(140.0f, 320.0f, 680.0f, 160.0f, { 0.2f, 0.6f, 0.9f, 0.8f });

    if (mCurrentLevelIndex < (int)mLevels.size()) {
        const LevelDef& level = mLevels[mCurrentLevelIndex];
        std::string title = "MISSION " + std::to_string(mCurrentLevelIndex + 1) + ": " + level.name;
        FontRenderer::DrawString("RubberStampLET20", title, 160.0f, 335.0f, { 1.0f, 0.85f, 0.2f, 1.0f }, 1.0f, ALIGN_LEFT);

        if (!level.intelList.empty()) {
            FontRenderer::DrawString("Normal", level.intelList[0].text, 160.0f, 375.0f, { 0.9f, 0.9f, 0.9f, 1.0f }, 0.95f, ALIGN_LEFT);
        }
    }

    FontRenderer::DrawString("Computer", "PRESS CROSS TO COMMENCE INVASION", SCREEN_WIDTH * 0.5f, 500.0f, { 0.2f, 1.0f, 0.3f, 1.0f }, 1.0f, ALIGN_CENTER);
}

void GameEngine::RenderPlaying() {
    // Parallax background
    WorldRenderer::Render();

    // Ground craters (crater.png, 5 frames)
    Texture* craterTex = TextureManager::Get("crater");
    if (craterTex) {
        for (const auto& cr : mCraters) {
            Renderer::DrawCel(craterTex, cr.frame % 5, 0, cr.x, cr.y, true);
        }
    }

    // PowerUp supply crates
    for (const auto& pup : mPowerUps) {
        pup.Render();
    }

    // Player Tank (authentic multi-layer)
    mPlayerTank.Render(mStats);

    // Enemies (authentic rotor animations, shadows, prop frames)
    for (const auto& e : mEnemies) {
        if (e.active) {
            e.Render();
        }
    }

    // Projectiles
    for (const auto& p : mProjectiles) {
        p.Render();
    }

    // Explosions (authentic PopCap 20-frame high-res sheet)
    for (const auto& exp : mExplosions) {
        exp.Render();
    }

    // Nuke Mushroom Cloud (mushsmoke.png, mushfire.jpg)
    if (mMushCloudTimer > 0.0f) {
        Texture* smokeTex = TextureManager::Get("mushsmoke");
        Texture* fireTex = TextureManager::Get("mushfire");
        float cloudProgress = 1.0f - (mMushCloudTimer / 2.5f);
        float cloudY = GROUND_Y - cloudProgress * 300.0f;
        float cloudScale = 1.0f + cloudProgress * 1.5f;

        if (smokeTex) {
            int frame = ((int)(cloudProgress * 3.0f)) % 3;
            Renderer::DrawCel(smokeTex, frame, 0, mMushCloudX, cloudY, true, cloudScale, cloudScale);
        }
        if (fireTex && cloudProgress < 0.6f) {
            Renderer::SetAdditiveBlend(true);
            int fFrame = ((int)(cloudProgress * 5.0f)) % 3;
            Renderer::DrawCel(fireTex, fFrame, 0, mMushCloudX, cloudY + 40.0f, true, cloudScale, cloudScale);
            Renderer::SetAdditiveBlend(false);
        }
    }

    // Nuke Screen Flash
    if (mNukeFlashAlpha > 0.0f) {
        Renderer::DrawFillRect(0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, { 1.0f, 1.0f, 1.0f, mNukeFlashAlpha });
    }

    // In-game HUD
    RenderHUD();
}

void GameEngine::RenderHUD() {
    // Metallic bottom status bar (statusbar.png, 640x30 stretched across 960 width)
    Texture* barTex = TextureManager::Get("statusbar");
    float barY = SCREEN_HEIGHT - 32.0f;
    if (barTex) {
        Renderer::DrawTexture(barTex, 0.0f, barY, (float)SCREEN_WIDTH, 32.0f);
    }

    // Sliding mile marker (milemarker.png, 65x101) tracking stage progress
    Texture* markerTex = TextureManager::Get("milemarker");
    if (markerTex) {
        float progressFrac = std::clamp(mLevelProgress / mLevelLength, 0.0f, 1.0f);
        float markerStartX = 240.0f;
        float markerEndX = 720.0f;
        float markerX = markerStartX + progressFrac * (markerEndX - markerStartX);
        Renderer::DrawCel(markerTex, 0, 0, markerX, barY - 20.0f, true, 0.65f, 0.65f);
    }

    // Score on top left
    std::string scoreStr = "SCORE: " + std::to_string(mStats.score);
    FontRenderer::DrawString("Normal", scoreStr, 24.0f, 16.0f, { 1.0f, 1.0f, 1.0f, 1.0f }, 1.0f);

    // Lives icons (tankicon.png)
    Texture* tankIcon = TextureManager::Get("tankicon");
    for (int i = 0; i < mStats.lives; ++i) {
        if (tankIcon) {
            Renderer::DrawCel(tankIcon, 0, 0, 24.0f + (float)i * 26.0f, 44.0f, false);
        }
    }

    // Nukes icons (nukeicon.png)
    Texture* nukeIcon = TextureManager::Get("nukeicon");
    for (int i = 0; i < mStats.nukes; ++i) {
        if (nukeIcon) {
            Renderer::DrawCel(nukeIcon, 0, 0, SCREEN_WIDTH - 40.0f - (float)i * 32.0f, 14.0f, false);
        }
    }

    // Megameter bar (megameter.png)
    Texture* meterTex = TextureManager::Get("megameter");
    if (meterTex) {
        Renderer::DrawCel(meterTex, 0, 0, SCREEN_WIDTH - 150.0f, 46.0f, false);
    }
    float meterW = 100.0f;
    float fillW = meterW * ((float)mStats.megalaserCharge / 100.0f);
    Renderer::DrawFillRect(SCREEN_WIDTH - 146.0f, 48.0f, fillW, 8.0f, { 0.2f, 0.7f, 1.0f, 0.95f });
    FontRenderer::DrawString("Normal", "MEGA", SCREEN_WIDTH - 192.0f, 44.0f, { 0.2f, 0.7f, 1.0f, 1.0f }, 0.8f);
}

void GameEngine::RenderArmory() {
    // PopCap authentic Armory Station (armory.jpg)
    Texture* armoryTex = TextureManager::Get("armory");
    if (armoryTex) {
        float drawW = 725.0f;
        float drawH = 544.0f;
        float drawX = (SCREEN_WIDTH - drawW) * 0.5f;
        Renderer::DrawTexture(armoryTex, drawX, 0.0f, drawW, drawH);
    }

    FontRenderer::DrawString("RubberStampLET20", "ARMORY UPGRADE STATION", SCREEN_WIDTH * 0.5f, 42.0f, { 1.0f, 0.85f, 0.2f, 1.0f }, 1.0f, ALIGN_CENTER);

    std::string pointsText = "UPGRADE POINTS AVAILABLE: " + std::to_string(mStats.availableUpgradePoints);
    FontRenderer::DrawString("Normal", pointsText, SCREEN_WIDTH * 0.5f, 75.0f, { 0.2f, 1.0f, 0.4f, 1.0f }, 1.0f, ALIGN_CENTER);

    // Weapon icons (upgrades.png, 72x72 cels) & Level indicators (upgradelvl.jpg)
    Texture* upgradesTex = TextureManager::Get("upgrades");
    Texture* lvlTex = TextureManager::Get("upgradelvl");
    Texture* btnTex = TextureManager::Get("upgradebtns");

    const char* weaponNames[WEAPON_COUNT] = {
        "HEAVY CANNON", "DEFENSE ORBS", "HOMING MISSILES", "LASER CANNON", "FLAK SHELLS", "THUNDERSTRIKE"
    };

    float startY = 110.0f;
    for (int i = 0; i < WEAPON_COUNT; ++i) {
        float rowY = startY + (float)i * 55.0f;
        float iconX = 180.0f;

        // Selection highlight
        if (mArmorySelection == i) {
            Renderer::DrawFillRect(iconX - 25.0f, rowY - 18.0f, 620.0f, 48.0f, { 0.15f, 0.35f, 0.6f, 0.4f });
            Renderer::DrawRect(iconX - 25.0f, rowY - 18.0f, 620.0f, 48.0f, { 0.3f, 0.7f, 1.0f, 0.8f });
        }

        // Weapon icon (72x72 cel scaled to 36x36)
        if (upgradesTex) {
            Renderer::DrawCel(upgradesTex, i, 0, iconX, rowY + 6.0f, true, 0.55f, 0.55f);
        }

        // Name
        Color4f nameCol = (mArmorySelection == i) ? Color4f{ 1.0f, 0.9f, 0.2f, 1.0f } : Color4f{ 1.0f, 1.0f, 1.0f, 1.0f };
        FontRenderer::DrawString("Normal", weaponNames[i], iconX + 30.0f, rowY, nameCol, 0.95f);

        // Level indicator (upgradelvl.jpg)
        int curLvl = mStats.weaponLevels[i];
        if (lvlTex) {
            int maxLvl = (i == WEAPON_CANNON) ? 5 : 3;
            for (int p = 0; p < maxLvl; ++p) {
                int frame = (p < curLvl) ? 1 : 0;
                Renderer::DrawCel(lvlTex, frame, 0, 480.0f + (float)p * 26.0f, rowY + 6.0f, true, 1.0f, 1.0f);
            }
        }

        // Plus / Minus buttons (upgradebtns.png)
        if (btnTex) {
            Renderer::DrawCel(btnTex, 0, 0, 640.0f, rowY + 6.0f, true, 0.65f, 0.65f); // Minus
            Renderer::DrawCel(btnTex, 1, 0, 680.0f, rowY + 6.0f, true, 0.65f, 0.65f); // Plus
        }
    }

    // Advance button (advancebtn.jpg)
    Texture* advanceTex = TextureManager::Get("advancebtn");
    if (advanceTex) {
        Renderer::DrawTexture(advanceTex, SCREEN_WIDTH - 220.0f, SCREEN_HEIGHT - 70.0f, 140.0f, 45.0f);
    }
    FontRenderer::DrawString("Computer", "D-PAD: SELECT/CHANGE    CROSS: DEPLOY", SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT - 25.0f, { 0.2f, 1.0f, 0.3f, 1.0f }, 0.9f, ALIGN_CENTER);
}

void GameEngine::RenderPaused() {
    Renderer::DrawFillRect(0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, { 0.0f, 0.0f, 0.0f, 0.65f });
    FontRenderer::DrawString("RubberStampLET42", "PAUSED", SCREEN_WIDTH * 0.5f, 210.0f, { 1.0f, 1.0f, 1.0f, 1.0f }, 1.0f, ALIGN_CENTER);
    FontRenderer::DrawString("Normal", "START: RESUME    CIRCLE: RETIRE TO TITLE", SCREEN_WIDTH * 0.5f, 290.0f, { 0.85f, 0.85f, 0.85f, 1.0f }, 1.0f, ALIGN_CENTER);
}

void GameEngine::RenderGameOver() {
    Renderer::DrawFillRect(0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, { 0.3f, 0.04f, 0.04f, 0.82f });
    FontRenderer::DrawString("RubberStampLET42", "MISSION FAILED", SCREEN_WIDTH * 0.5f, 190.0f, { 1.0f, 0.2f, 0.2f, 1.0f }, 1.0f, ALIGN_CENTER);

    std::string scoreStr = "FINAL SCORE: " + std::to_string(mStats.score);
    FontRenderer::DrawString("Normal", scoreStr, SCREEN_WIDTH * 0.5f, 275.0f, { 1.0f, 1.0f, 1.0f, 1.0f }, 1.2f, ALIGN_CENTER);

    FontRenderer::DrawString("Computer", "PRESS CROSS TO RETURN TO HEADQUARTERS", SCREEN_WIDTH * 0.5f, 370.0f, { 1.0f, 0.9f, 0.2f, 1.0f }, 1.0f, ALIGN_CENTER);
}

} // namespace HeavyWeapon
