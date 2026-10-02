#pragma once

#include "Constants.h"
#include "DataModels.h"
#include "Entities.h"
#include "XmlLoader.h"
#include "WorldRenderer.h"
#include "FontRenderer.h"
#include "AudioSystem.h"
#include <vector>
#include <unordered_map>

namespace HeavyWeapon {

class GameEngine {
public:
    GameEngine();
    ~GameEngine();

    bool Init();
    void Run();
    void Shutdown();

    void Update(float dt);
    void Render();

private:
    bool mRunning = true;
    GameState mState = STATE_TITLE;

    // Game data loaded from XML
    std::unordered_map<std::string, CraftDef> mCraftDefs;
    std::vector<LevelDef> mLevels;
    std::unordered_map<std::string, BossDef> mBossDefs;
    std::vector<std::vector<AnimDef>> mLevelAnims;

    // Runtime state
    PlayerStats mStats;
    PlayerTank mPlayerTank;
    std::vector<Enemy> mEnemies;
    std::vector<Projectile> mProjectiles;
    std::vector<ExplosionInstance> mExplosions;
    std::vector<CraterInstance> mCraters;
    std::vector<PowerUpItem> mPowerUps;
    std::vector<Particle> mParticles;

    // Wave spawning state
    int mCurrentLevelIndex = 0;
    size_t mCurrentWaveIndex = 0;
    float mWaveTimer = 0.0f;
    float mLevelProgress = 0.0f;
    float mLevelLength = 1000.0f;
    float mSpawnTimer = 0.0f;
    bool mBossSpawned = false;

    // Menu & UI State
    int mMenuSelection = 0;
    int mArmorySelection = 0;
    float mMenuGlowAnim = 0.0f;
    float mDieselSoundTimer = 0.0f;

    // FX State
    float mNukeFlashAlpha = 0.0f;
    float mMushCloudTimer = 0.0f;
    float mMushCloudX = 480.0f;

    // Internal state updates
    void UpdateTitle(float dt);
    void UpdateMissionSelect(float dt);
    void UpdatePlaying(float dt);
    void UpdateArmory(float dt);
    void UpdatePaused(float dt);
    void UpdateGameOver(float dt);

    // Internal state renders
    void RenderTitle();
    void RenderMissionSelect();
    void RenderPlaying();
    void RenderHUD();
    void RenderArmory();
    void RenderPaused();
    void RenderGameOver();

    // Spawning & Combat helpers
    void StartLevel(int levelIndex);
    void SpawnNextEnemy();
    void TriggerNuke();
    void SpawnExplosion(float x, float y, float size = 1.0f, bool isNuke = false);
};

} // namespace HeavyWeapon
