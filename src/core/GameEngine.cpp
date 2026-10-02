#include "GameEngine.h"
#include "Vfs.h"
#include "InputManager.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <SDL2/SDL.h>

namespace HeavyWeapon {

GameEngine::GameEngine() {}

GameEngine::~GameEngine() {
    Shutdown();
}

bool GameEngine::Init(SDL_Window* window, const LaunchOptions& opts) {
    mOpts = opts;
    Vfs::Init();
    TextureManager::Init();
    if (!Renderer::Init(window)) {
        return false;
    }
    Renderer::SetScaleMode(opts.stretch ? SCALE_STRETCH : SCALE_ASPECT);
    mInitialized = true;
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

    mState = STATE_TITLE;
    mCurrentLevelIndex = std::clamp(opts.level, 0, std::max(0, (int)mLevels.size() - 1));
    if (opts.startState == "map") {
        mState = STATE_MISSION_SELECT;
    } else if (opts.startState == "play") {
        StartLevel(mCurrentLevelIndex);
    } else if (opts.startState == "armory") {
        mStats.availableUpgradePoints = 3;
        mState = STATE_ARMORY;
    }

    if (mState != STATE_PLAYING) {
        AudioSystem::PlayMusic("Music/LoveTheme.ogg");
    }
    return true;
}

void GameEngine::Shutdown() {
    if (!mInitialized) return;
    mInitialized = false;
    AudioSystem::Shutdown();
    InputManager::Shutdown();
    TextureManager::Shutdown();
    Renderer::Shutdown();
}

void GameEngine::Run() {
    // With --frames the timestep is fixed so captured frames are reproducible.
    const bool fixedRun = mOpts.maxFrames > 0;
    int frame = 0;
    uint32_t lastTicks = SDL_GetTicks();
    while (mRunning) {
        uint32_t currentTicks = SDL_GetTicks();
        float dt = (float)(currentTicks - lastTicks) / 1000.0f;
        if (dt > 0.05f) dt = 0.05f; // Cap at 20fps minimum
        lastTicks = currentTicks;
        if (fixedRun) dt = FIXED_DT;

        Update(dt);
        Render();

        if (InputManager::GetState().quitRequested) mRunning = false;
        if (fixedRun && ++frame >= mOpts.maxFrames) {
            if (!mOpts.screenshotPath.empty()) {
                bool ok = Renderer::SaveScreenshot(mOpts.screenshotPath);
                std::cout << (ok ? "[GameEngine] Saved screenshot " : "[GameEngine] Failed to save screenshot ")
                          << mOpts.screenshotPath << std::endl;
            }
            mRunning = false;
        }
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

// Main menu buttons are baked into mainmenu.jpg; these are their centres (measured from
// the art). The highlight sprites mainbigbtn (110x110) / mainsmallbtn (120x40) are
// additive rim glows centred on the selected button.
struct MenuButton {
    float cx, cy;
    bool big;
};
static const MenuButton kMenuButtons[] = {
    {  99.5f, 250.0f, true  }, // MISSION
    { 229.5f, 380.0f, true  }, // SURVIVAL
    { 534.5f, 215.0f, false }, // HEROES
    { 505.0f, 285.0f, false }, // OPTIONS
    { 504.5f, 354.5f, false }, // HELP
    { 534.0f, 425.0f, false }, // QUIT
};
enum { MENU_MISSION = 0, MENU_SURVIVAL, MENU_HEROES, MENU_OPTIONS, MENU_HELP, MENU_QUIT, MENU_COUNT };

static int HitTestMenu(float x, float y) {
    for (int i = 0; i < MENU_COUNT; ++i) {
        const MenuButton& b = kMenuButtons[i];
        float hw = b.big ? 50.0f : 55.0f;
        float hh = b.big ? 50.0f : 16.0f;
        if (std::abs(x - b.cx) <= hw && std::abs(y - b.cy) <= hh) return i;
    }
    return -1;
}

void GameEngine::UpdateTitle(float dt) {
    (void)dt;
    const InputState& input = InputManager::GetState();

    // Big buttons on the left, small ones in a column on the right.
    int prevSel = mMenuSelection;
    if (input.upPressed) {
        mMenuSelection = (mMenuSelection - 1 + MENU_COUNT) % MENU_COUNT;
    } else if (input.downPressed) {
        mMenuSelection = (mMenuSelection + 1) % MENU_COUNT;
    } else if (input.rightPressed && kMenuButtons[mMenuSelection].big) {
        mMenuSelection = MENU_HEROES;
    } else if (input.leftPressed && !kMenuButtons[mMenuSelection].big) {
        mMenuSelection = MENU_MISSION;
    }

    bool activate = input.confirmPressed;
    if (input.pointerAim || input.touchPressed) {
        int hit = HitTestMenu(input.touchX, input.touchY);
        if (hit >= 0) {
            mMenuSelection = hit;
            activate |= input.touchPressed;
        }
    }
    if (mMenuSelection != prevSel) {
        AudioSystem::PlaySound("mapover", 0.5f);
    }

    if (!activate) return;

    AudioSystem::PlaySound("buttondown");
    switch (mMenuSelection) {
        case MENU_MISSION:
            mCurrentLevelIndex = 0;
            mStats = PlayerStats();
            mState = STATE_MISSION_SELECT;
            break;
        case MENU_SURVIVAL:
            // Survival mode is not implemented yet; start mission 1 as a placeholder.
            mCurrentLevelIndex = 0;
            mStats = PlayerStats();
            StartLevel(0);
            break;
        case MENU_QUIT:
            mRunning = false;
            break;
        default:
            AudioSystem::PlaySound("denied", 0.5f); // Heroes / Options / Help not implemented yet
            break;
    }
}

void GameEngine::UpdateMissionSelect(float dt) {
    (void)dt;
    const InputState& input = InputManager::GetState();

    if (input.confirmPressed || input.touchPressed) {
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
    Renderer::DrawTexture(TextureManager::Get("mainmenu"), 0.0f, 0.0f);

    // Hover state: mainglow is a full-screen "over" image of which only the area around
    // the selected button is drawn (lighting its bezel), plus the button's own rim glow.
    const MenuButton& sel = kMenuButtons[mMenuSelection];
    Texture* glowTex = TextureManager::Get("mainglow");
    if (glowTex) {
        Rect area = sel.big ? Rect{ sel.cx - 85.0f, sel.cy - 85.0f, 170.0f, 170.0f }
                            : Rect{ sel.cx - 95.0f, sel.cy - 35.0f, SCREEN_WIDTH - (sel.cx - 95.0f), 70.0f };
        area.x = std::max(area.x, 0.0f);
        area.y = std::max(area.y, 0.0f);
        area.w = std::min(area.w, SCREEN_WIDTH - area.x);
        area.h = std::min(area.h, SCREEN_HEIGHT - area.y);
        Renderer::SetAdditiveBlend(true);
        Renderer::SetColor({ 1.0f, 1.0f, 1.0f, 0.75f + 0.25f * std::sin(mMenuGlowAnim) });
        Renderer::DrawTexture(glowTex, area, area);
        Renderer::SetColor(Color4f::White());
        Renderer::SetAdditiveBlend(false);
    }

    Texture* hlTex = TextureManager::Get(sel.big ? "mainbigbtn" : "mainsmallbtn");
    if (hlTex) {
        Renderer::SetAdditiveBlend(true);
        Renderer::DrawCel(hlTex, 0, 0, sel.cx, sel.cy, true);
        Renderer::SetAdditiveBlend(false);
    }
}

void GameEngine::RenderMissionSelect() {
    Renderer::DrawTexture(TextureManager::Get("map"), 0.0f, 0.0f);

    // Mission briefing panel. Provisional layout until the original war-room screen
    // (missionN.png territory overlays, mappointer, maprect) is decompiled.
    const float panelX = 40.0f, panelY = 330.0f, panelW = 560.0f, panelH = 120.0f;
    Renderer::DrawFillRect(panelX, panelY, panelW, panelH, { 0.0f, 0.0f, 0.0f, 0.75f });
    Renderer::DrawRect(panelX, panelY, panelW, panelH, { 0.85f, 0.85f, 0.85f, 0.9f });

    if (mCurrentLevelIndex < (int)mLevels.size()) {
        const LevelDef& level = mLevels[mCurrentLevelIndex];
        std::string title = "MISSION " + std::to_string(mCurrentLevelIndex + 1) + ": " + level.name;
        FontRenderer::DrawString("RubberStampLET20", title, panelX + 14.0f, panelY + 10.0f, { 1.0f, 0.85f, 0.2f, 1.0f });
        if (!level.intelList.empty()) {
            // Greedy word wrap to the panel width.
            std::istringstream words(level.intelList[0].text);
            std::string word, line;
            float lineY = panelY + 40.0f;
            const float maxW = panelW - 28.0f;
            const float lineH = FontRenderer::GetStringHeight("Normal");
            while (words >> word && lineY < panelY + panelH - 44.0f) {
                std::string candidate = line.empty() ? word : line + " " + word;
                if (!line.empty() && FontRenderer::GetStringWidth("Normal", candidate) > maxW) {
                    FontRenderer::DrawString("Normal", line, panelX + 14.0f, lineY, Color4f::White());
                    lineY += lineH;
                    line = word;
                } else {
                    line = candidate;
                }
            }
            if (!line.empty() && lineY < panelY + panelH - 44.0f) {
                FontRenderer::DrawString("Normal", line, panelX + 14.0f, lineY, Color4f::White());
            }
        }
    }
    FontRenderer::DrawString("Normal", "PRESS CROSS TO BEGIN", SCREEN_WIDTH * 0.5f, panelY + panelH - 24.0f,
                             { 0.6f, 1.0f, 0.6f, 1.0f }, 1.0f, ALIGN_CENTER);
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
    // Status bar along the top of the screen. The contents of each slot are provisional
    // until the original HUD draw code is decompiled.
    Renderer::DrawTexture(TextureManager::Get("statusbar"), 0.0f, STATUSBAR_Y);

    Texture* tankIcon = TextureManager::Get("tankicon");
    for (int i = 0; i < mStats.lives && tankIcon; ++i) {
        Renderer::DrawTexture(tankIcon, 8.0f + (float)i * 21.0f, STATUSBAR_Y + 7.0f);
    }

    Texture* nukeIcon = TextureManager::Get("nukeicon");
    for (int i = 0; i < mStats.nukes && nukeIcon; ++i) {
        Renderer::DrawCel(nukeIcon, 0, 0, 125.0f + (float)i * 16.0f, STATUSBAR_Y + 15.0f, true, 0.6f, 0.6f);
    }

    FontRenderer::DrawString("Normal", std::to_string(mStats.score), 330.0f, STATUSBAR_Y + 7.0f,
                             Color4f::White(), 1.0f, ALIGN_RIGHT);

    // Megalaser charge
    Texture* meterTex = TextureManager::Get("megameter");
    float meterX = 437.0f, meterY = STATUSBAR_Y + 6.0f;
    if (meterTex) {
        float frac = std::clamp((float)mStats.megalaserCharge / 100.0f, 0.0f, 1.0f);
        Rect src = { 0.0f, 0.0f, (float)meterTex->width * frac, (float)meterTex->height };
        Rect dst = { meterX, meterY, src.w, src.h };
        Renderer::DrawTexture(meterTex, dst, src);
    }
}

void GameEngine::RenderArmory() {
    Renderer::DrawTexture(TextureManager::Get("armory"), 0.0f, 0.0f);

    // Six weapon sockets baked into armory.jpg, three per side (measured from the art).
    // Index order matches WeaponType.
    static const float kRowY[3] = { 60.0f, 160.0f, 260.0f };
    auto socketX = [](int i) { return (i < 3) ? 108.0f : 532.0f; };
    auto levelX = [](int i) { return (i < 3) ? 40.0f : 602.0f; };

    Texture* upgradesTex = TextureManager::Get("upgrades");     // 6 x 72x72
    Texture* lvlTex = TextureManager::Get("upgradelvl");        // 4 x 20x31
    Texture* btnTex = TextureManager::Get("upgradebtns");       // 2x2 +/- glows

    for (int i = 0; i < WEAPON_COUNT; ++i) {
        float cy = kRowY[i % 3];
        if (upgradesTex) {
            Renderer::DrawCel(upgradesTex, i, 0, socketX(i), cy, true);
        }
        if (lvlTex) {
            int lvl = std::clamp(mStats.weaponLevels[i], 0, lvlTex->cols - 1);
            Renderer::DrawCel(lvlTex, lvl, 0, levelX(i), cy, true);
        }
        if (mArmorySelection == i && btnTex) {
            Renderer::SetAdditiveBlend(true);
            float bx = (i < 3) ? 161.0f : 478.0f;
            // Column 0 matches the left-hand sockets' buttons, column 1 the right-hand ones.
            Renderer::DrawCel(btnTex, (i < 3) ? 0 : 1, 0, bx, cy - 14.5f, true);
            Renderer::DrawCel(btnTex, (i < 3) ? 0 : 1, 1, bx, cy + 14.0f, true);
            Renderer::SetAdditiveBlend(false);
        }
    }

    // Central screen: selected weapon and remaining points.
    const char* weaponNames[WEAPON_COUNT] = {
        "HEAVY CANNON", "DEFENSE ORBS", "HOMING MISSILES", "LASER CANNON", "FLAK SHELLS", "THUNDERSTRIKE"
    };
    FontRenderer::DrawString("Normal", weaponNames[mArmorySelection], 316.0f, 110.0f,
                             { 0.6f, 1.0f, 0.6f, 1.0f }, 1.0f, ALIGN_CENTER);
    FontRenderer::DrawString("Normal", "POINTS: " + std::to_string(mStats.availableUpgradePoints), 316.0f, 140.0f,
                             { 0.6f, 1.0f, 0.6f, 1.0f }, 1.0f, ALIGN_CENTER);

    Texture* advanceTex = TextureManager::Get("advancebtn");
    if (advanceTex) {
        // Strip of three 135px button states (normal / over / down); draw "normal".
        float w = advanceTex->width / 3.0f;
        Rect src = { 0.0f, 0.0f, w, (float)advanceTex->height };
        Rect dst = { (SCREEN_WIDTH - w) * 0.5f, 420.0f, w, src.h };
        Renderer::DrawTexture(advanceTex, dst, src);
    }
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
