#pragma once

#include "Constants.h"
#include "DataModels.h"
#include "Renderer.h"
#include <vector>
#include <string>

namespace HeavyWeapon {

enum ProjectileType {
    PROJ_PLAYER_CANNON = 0,
    PROJ_PLAYER_MISSILE,
    PROJ_PLAYER_FLAK,
    PROJ_PLAYER_LASER,
    PROJ_ENEMY_BULLET,
    PROJ_ENEMY_BOMB,
    PROJ_ENEMY_ARMORED_BOMB,
    PROJ_ENEMY_MISSILE,
    PROJ_ENEMY_ATOMIC_BOMB,
    PROJ_NUKE_BLAST
};

struct Projectile {
    ProjectileType type = PROJ_PLAYER_CANNON;
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float angle = 0.0f;
    int damage = 1;
    float life = 3.0f;
    bool active = true;
};

struct Particle {
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float life = 1.0f;
    float maxLife = 1.0f;
    float size = 8.0f;
    Color4f color = Color4f::White();
    bool additive = false;
    bool active = true;
};

struct PowerUpItem {
    float x = 0.0f;
    float y = 0.0f;
    float vy = 60.0f; // falling speed
    int weaponType = 0; // or nuke
    bool isNuke = false;
    bool active = true;
};

class Enemy {
public:
    CraftDef def;
    float x = 0.0f;
    float y = 0.0f;
    float vx = -100.0f;
    float vy = 0.0f;
    int hp = 1;
    int maxHp = 1;
    float fireTimer = 1.0f;
    float animFrame = 0.0f;
    bool active = true;
    bool isBoss = false;

    void Update(float dt, float playerX, float playerY, std::vector<Projectile>& outEnemyProj);
    void Render();
    Rect GetHitbox() const;
};

class PlayerTank {
public:
    float x = 480.0f;
    float y = TANK_DEFAULT_Y;
    float vx = 0.0f;
    float turretAngle = -90.0f;
    float recoil = 0.0f;
    float fireCooldown = 0.0f;
    float megalaserTimer = 0.0f;
    float defenseOrbAngle = 0.0f;
    bool isInvulnerable = false;
    float invulnerableTimer = 0.0f;

    void Init();
    void Update(float dt, const PlayerStats& stats, std::vector<Projectile>& outPlayerProj);
    void Render(const PlayerStats& stats);
    Rect GetHitbox() const;
    void TakeDamage();
};

} // namespace HeavyWeapon
