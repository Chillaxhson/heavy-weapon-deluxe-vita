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
        if (opts.armoryLevel > 0) {
            for (int i = UP_ORBS; i <= UP_STATIC; ++i) mApp.up[i] = std::min(opts.armoryLevel, 3);
            mApp.up[UP_SPREAD] = std::min(opts.armoryLevel, 4);
        }
        StartLevel(mCurrentLevelIndex);
    } else if (opts.startState == "armory") {
        mUpgradePoints = 3;
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
            mApp = AppState();
            mState = STATE_MISSION_SELECT;
            break;
        case MENU_SURVIVAL:
            // Survival mode is not implemented yet; start mission 1 as a placeholder.
            mCurrentLevelIndex = 0;
            mApp = AppState();
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
    mApp.mission = levelIndex;
    mTickAccum = 0.0f;
    mLevelEndTimer = 0;

    const LevelDef* level = (levelIndex < (int)mLevels.size()) ? &mLevels[levelIndex] : nullptr;
    std::string theme = level ? level->bgTheme : "antagonistan";
    std::vector<AnimDef> anims;
    if (levelIndex < (int)mLevelAnims.size()) anims = mLevelAnims[levelIndex];
    WorldRenderer::SetTheme(theme, anims);

    std::vector<CraftDef> byId(mCraftDefs.size() + 1);
    for (const auto& kv : mCraftDefs) {
        if (kv.second.id > 0 && kv.second.id < (int)byId.size()) byId[kv.second.id] = kv.second;
    }
    mBoard = std::make_unique<Board>(mApp, level, byId, false);

    AudioSystem::PlayMusic("Music/AtomicTank.mo3");
    AudioSystem::PlaySoundId(SND_V_GETREADY);
    mState = STATE_PLAYING;
}

// Feeds input to the board the way the original's mouse handlers do: the cursor is the
// point the tank drives toward and aims at. Gamepads aim with the right stick and drive
// with the left stick instead.
void GameEngine::ApplyBoardInput() {
    const InputState& input = InputManager::GetState();
    Board& board = *mBoard;

    bool stickAim = std::abs(input.aimAxisX) > 0.0f || std::abs(input.aimAxisY) > 0.0f;
    if (input.touchDown || (input.pointerAim && !stickAim)) {
        board.SetTarget((int)input.touchX, (int)input.touchY);
        board.SetDriveOverride(false, 0.0f);
    } else {
        // Point the virtual cursor 200px out along the right stick (straight up when idle),
        // so the original aiming code applies unchanged.
        float ax = input.aimAxisX, ay = input.aimAxisY;
        if (!stickAim) { ax = 0.0f; ay = -1.0f; }
        if (ay > 0.0f) ay = 0.0f;
        float len = std::sqrt(ax * ax + ay * ay);
        if (len < 0.001f) { ax = 0.0f; ay = -1.0f; len = 1.0f; }
        int tx = (int)(board.TankX() + 320.0 + ax / len * 200.0);
        int ty = (int)(board.TankY() - 12 + ay / len * 200.0);
        board.SetTarget(tx, ty);
        board.SetDriveOverride(true, input.moveAxisX);
    }
    board.SetFiring(input.fireCannon || input.touchDown || mOpts.autoFire);
    if (input.fireNukePressed) board.FireNuke();
}

void GameEngine::UpdatePlaying(float dt) {
    const InputState& input = InputManager::GetState();
    if (input.pausePressed) {
        mState = STATE_PAUSED;
        return;
    }

    // The original logic runs at a fixed 100 Hz; render frames run at the display rate.
    mTickAccum += dt * POPCAP_TICKS_PER_SEC;
    if (mTickAccum > 10.0f) mTickAccum = 10.0f;
    while (mTickAccum >= 1.0f) {
        mTickAccum -= 1.0f;
        ApplyBoardInput();
        mBoard->Update();
        AudioSystem::Tick();
    }

    if (mBoard->IsGameOver()) {
        AudioSystem::PlaySoundId(SND_V_GAMEOVER);
        mState = STATE_GAMEOVER;
        return;
    }

    // Placeholder level end until the boss and level-complete screens are ported: once
    // the level length is reached, move on to the armory after a short pause.
    if (mBoard->Progress() >= mBoard->Length()) {
        if (++mLevelEndTimer == 1) AudioSystem::PlaySoundId(SND_V_LEVELCOMPLETE);
        if (mLevelEndTimer > 180) {
            mUpgradePoints++;
            mState = STATE_ARMORY;
        }
    }
}

void GameEngine::UpdateArmory(float dt) {
    (void)dt;
    const InputState& input = InputManager::GetState();
    constexpr int kWeapons = UP_STATIC - UP_ORBS + 1;

    if (input.upPressed) {
        mArmorySelection = (mArmorySelection - 1 + kWeapons) % kWeapons;
        AudioSystem::PlaySoundId(SND_MAPOVER);
    } else if (input.downPressed) {
        mArmorySelection = (mArmorySelection + 1) % kWeapons;
        AudioSystem::PlaySoundId(SND_MAPOVER);
    }

    int& level = mApp.up[UP_ORBS + mArmorySelection];
    if (input.rightPressed || input.altFirePressed) {
        if (mUpgradePoints > 0 && level < 3) {
            ++level;
            --mUpgradePoints;
            AudioSystem::PlaySoundId(SND_UPGRADE);
        } else {
            AudioSystem::PlaySoundId(SND_DENIED);
        }
    } else if (input.leftPressed) {
        if (level > 0) {
            --level;
            ++mUpgradePoints;
            AudioSystem::PlaySoundId(SND_BUTTONUP);
        }
    }

    if (input.confirmPressed) {
        AudioSystem::PlaySoundId(SND_BUTTONDOWN);
        mCurrentLevelIndex++;
        mState = (mCurrentLevelIndex >= NUM_CAMPAIGN_MISSIONS) ? STATE_TITLE : STATE_MISSION_SELECT;
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
        mApp = AppState();
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
    if (mBoard) mBoard->Draw();
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
            int lvl = std::clamp(mApp.up[UP_ORBS + i], 0, lvlTex->cols - 1);
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
    // Names from the executable's weapon table, in armory slot order.
    const char* weaponNames[WEAPON_COUNT] = {
        "DEFENSE ORBS", "HOMING MISSILE", "LASER", "ROCKETS", "FLAK CANNON", "THUNDERSTRIKE"
    };
    FontRenderer::DrawString("Normal", weaponNames[mArmorySelection], 316.0f, 110.0f,
                             { 0.6f, 1.0f, 0.6f, 1.0f }, 1.0f, ALIGN_CENTER);
    FontRenderer::DrawString("Normal", "POINTS: " + std::to_string(mUpgradePoints), 316.0f, 140.0f,
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

    std::string scoreStr = "FINAL SCORE: " + std::to_string(mApp.score);
    FontRenderer::DrawString("Normal", scoreStr, SCREEN_WIDTH * 0.5f, 275.0f, { 1.0f, 1.0f, 1.0f, 1.0f }, 1.2f, ALIGN_CENTER);

    FontRenderer::DrawString("Computer", "PRESS CROSS TO RETURN TO HEADQUARTERS", SCREEN_WIDTH * 0.5f, 370.0f, { 1.0f, 0.9f, 0.2f, 1.0f }, 1.0f, ALIGN_CENTER);
}

} // namespace HeavyWeapon
