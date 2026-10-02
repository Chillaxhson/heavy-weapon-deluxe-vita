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
    PROJ_ENEMY_FRAG_BOMB,
    PROJ_ENEMY_FATBOY,
    PROJ_ENEMY_LGB,
    PROJ_ENEMY_MISSILE,
    PROJ_ENEMY_ENERGY_CANNON,
    PROJ_NUKE_BLAST
};

struct Projectile {
    ProjectileType type = PROJ_PLAYER_CANNON;
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float angle = 0.0f;
    int angleCel = 10; // 0 to 20
    int level = 0;    // 0 to 4
    float animFrame = 0.0f;
    int damage = 1;
    float life = 3.0f;
    bool active = true;

    void Render() const;
};

struct EjectedCasing {
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float rot = 0.0f;
    float vrot = 0.0f;
    float life = 1.2f;
    bool active = true;
};

struct ExplosionInstance {
    float x = 0.0f;
    float y = 0.0f;
    float animFrame = 0.0f; // 0.0 to 20.0
    float scale = 1.0f;
    bool isNuke = false;
    bool active = true;

    void Update(float dt);
    void Render() const;
};

struct CraterInstance {
    float x = 0.0f;
    float y = 0.0f;
    int frame = 0; // 0 to 4
    float alpha = 1.0f;
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
    int cel = 0;
    bool isNuke = false;
    bool active = true;

    void Render() const;
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
    float rotorAnim = 0.0f;
    float hitFlashTimer = 0.0f;
    bool active = true;
    bool isBoss = false;

    void Update(float dt, float playerX, float playerY, std::vector<Projectile>& outEnemyProj);
    void Render() const;
    void TakeDamage(int dmg);
    Rect GetHitbox() const;
};

class PlayerTank {
public:
    float x = 480.0f;
    float y = TANK_DEFAULT_Y;
    float vx = 0.0f;
    float turretAngle = 90.0f; // 90 is straight UP, 180 is left, 0 is right
    int turretAngleCel = 10;   // 0 (left) to 20 (right), 10 (up)
    float recoil = 0.0f;
    float fireCooldown = 0.0f;
    float megalaserTimer = 0.0f;
    float defenseOrbAngle = 0.0f;
    float treadAnim = 0.0f;
    float flameAnim = 0.0f;
    float muzzleFlashTimer = 0.0f;
    bool isInvulnerable = false;
    float invulnerableTimer = 0.0f;

    std::vector<EjectedCasing> casings;

    void Init();
    void Update(float dt, const PlayerStats& stats, std::vector<Projectile>& outPlayerProj);
    void Render(const PlayerStats& stats);
    Rect GetHitbox() const;
    void TakeDamage();
};

} // namespace HeavyWeapon
