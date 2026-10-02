#pragma once

#include <cstdint>
#include <string>

namespace HeavyWeapon {

// Display metrics
constexpr int SCREEN_WIDTH = 960;
constexpr int SCREEN_HEIGHT = 544;
constexpr float TARGET_FPS = 60.0f;
constexpr float FIXED_DT = 1.0f / TARGET_FPS;

// Original PopCap internal coordinate space
constexpr int ORIGINAL_WIDTH = 640;
constexpr int ORIGINAL_HEIGHT = 480;

// Ground plane positioning
constexpr float GROUND_Y = 476.0f;
constexpr float TANK_DEFAULT_Y = 460.0f;

// Tank limits
constexpr float TANK_MIN_X = 40.0f;
constexpr float TANK_MAX_X = 920.0f;
constexpr float TANK_SPEED = 240.0f; // pixels per second

// Gameplay constants
constexpr int MAX_LIVES = 5;
constexpr int INITIAL_LIVES = 3;
constexpr int INITIAL_NUKES = 1;
constexpr int MAX_NUKES = 3;
constexpr int NUM_CAMPAIGN_MISSIONS = 19;

// Weapon upgrade indices
enum WeaponType {
    WEAPON_CANNON = 0,
    WEAPON_DEFENSE_ORBS,
    WEAPON_HOMING_MISSILES,
    WEAPON_LASER,
    WEAPON_FLAK,
    WEAPON_THUNDERSTRIKE,
    WEAPON_COUNT
};

// Game state machine
enum GameState {
    STATE_BOOT = 0,
    STATE_TITLE,
    STATE_MISSION_SELECT,
    STATE_PLAYING,
    STATE_ARMORY,
    STATE_PAUSED,
    STATE_GAMEOVER,
    STATE_VICTORY
};

} // namespace HeavyWeapon
