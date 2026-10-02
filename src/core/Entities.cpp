#include "Entities.h"
#include "InputManager.h"
#include "AudioSystem.h"
#include <cmath>
#include <algorithm>

namespace HeavyWeapon {

void PlayerTank::Init() {
    x = 480.0f;
    y = TANK_DEFAULT_Y;
    vx = 0.0f;
    turretAngle = -90.0f;
    recoil = 0.0f;
    fireCooldown = 0.0f;
    megalaserTimer = 0.0f;
    defenseOrbAngle = 0.0f;
    isInvulnerable = true;
    invulnerableTimer = 2.0f; // 2 seconds spawn shield
}

void PlayerTank::TakeDamage() {
    if (isInvulnerable) return;
    isInvulnerable = true;
    invulnerableTimer = 2.5f;
    Renderer::AddScreenShake(12.0f, 0.4f);
    AudioSystem::PlaySound("tankexplode");
}

void PlayerTank::Update(float dt, const PlayerStats& stats, std::vector<Projectile>& outPlayerProj) {
    const InputState& input = InputManager::GetState();

    // Invulnerability timer
    if (invulnerableTimer > 0.0f) {
        invulnerableTimer -= dt;
        if (invulnerableTimer <= 0.0f) {
            isInvulnerable = false;
        }
    }

    // Horizontal movement
    vx = input.moveAxisX * TANK_SPEED;
    x += vx * dt;
    x = std::clamp(x, TANK_MIN_X, TANK_MAX_X);

    // Aiming
    turretAngle = input.aimAngleDegrees;

    // Recoil recovery
    if (recoil > 0.0f) {
        recoil -= 40.0f * dt;
        if (recoil < 0.0f) recoil = 0.0f;
    }

    // Cooldown countdown
    if (fireCooldown > 0.0f) {
        fireCooldown -= dt;
    }

    // Defense orb rotation
    defenseOrbAngle += 180.0f * dt;
    if (defenseOrbAngle >= 360.0f) defenseOrbAngle -= 360.0f;

    // Megalaser update
    if (megalaserTimer > 0.0f) {
        megalaserTimer -= dt;
        // Continuous megalaser projectile
        Projectile laser;
        laser.type = PROJ_PLAYER_LASER;
        laser.x = x;
        laser.y = y - 25.0f;
        laser.angle = turretAngle;
        laser.damage = 15;
        laser.life = 0.05f;
        outPlayerProj.push_back(laser);
    }

    // Primary weapon firing
    if (input.fireCannon && fireCooldown <= 0.0f) {
        int cannonLvl = stats.weaponLevels[WEAPON_CANNON];
        fireCooldown = 0.11f; // 9 rounds per second

        float rad = turretAngle * 3.14159265f / 180.0f;
        float barrelLength = 40.0f;
        float muzzleX = x + std::cos(rad) * barrelLength;
        float muzzleY = (y - 18.0f) + std::sin(rad) * barrelLength;

        recoil = 6.0f;
        AudioSystem::PlaySound("tankfire");

        float bulletSpeed = 950.0f;

        if (cannonLvl <= 1) {
            Projectile p;
            p.type = PROJ_PLAYER_CANNON;
            p.x = muzzleX;
            p.y = muzzleY;
            p.vx = std::cos(rad) * bulletSpeed;
            p.vy = std::sin(rad) * bulletSpeed;
            p.angle = turretAngle;
            p.damage = 1;
            outPlayerProj.push_back(p);
        } else if (cannonLvl == 2) {
            for (float spread : { -3.0f, 3.0f }) {
                float r = (turretAngle + spread) * 3.14159265f / 180.0f;
                Projectile p;
                p.type = PROJ_PLAYER_CANNON;
                p.x = muzzleX;
                p.y = muzzleY;
                p.vx = std::cos(r) * bulletSpeed;
                p.vy = std::sin(r) * bulletSpeed;
                p.angle = turretAngle + spread;
                p.damage = 1;
                outPlayerProj.push_back(p);
            }
        } else { // Level 3+
            for (float spread : { -6.0f, 0.0f, 6.0f }) {
                float r = (turretAngle + spread) * 3.14159265f / 180.0f;
                Projectile p;
                p.type = PROJ_PLAYER_CANNON;
                p.x = muzzleX;
                p.y = muzzleY;
                p.vx = std::cos(r) * bulletSpeed;
                p.vy = std::sin(r) * bulletSpeed;
                p.angle = turretAngle + spread;
                p.damage = 2;
                outPlayerProj.push_back(p);
            }
        }

        // Secondary: Homing missiles
        int missileLvl = stats.weaponLevels[WEAPON_HOMING_MISSILES];
        if (missileLvl > 0 && (rand() % 4 == 0)) {
            Projectile m;
            m.type = PROJ_PLAYER_MISSILE;
            m.x = x;
            m.y = y - 25.0f;
            m.vx = (float)(rand() % 100 - 50);
            m.vy = -500.0f;
            m.damage = 2 * missileLvl;
            outPlayerProj.push_back(m);
            AudioSystem::PlaySound("missile");
        }

        // Secondary: Flak cannon
        int flakLvl = stats.weaponLevels[WEAPON_FLAK];
        if (flakLvl > 0 && (rand() % 5 == 0)) {
            Projectile f;
            f.type = PROJ_PLAYER_FLAK;
            f.x = muzzleX;
            f.y = muzzleY;
            f.vx = std::cos(rad) * 650.0f;
            f.vy = std::sin(rad) * 650.0f;
            f.damage = 3 * flakLvl;
            outPlayerProj.push_back(f);
            AudioSystem::PlaySound("flak");
        }
    }
}

Rect PlayerTank::GetHitbox() const {
    return { x - 35.0f, y - 25.0f, 70.0f, 35.0f };
}

void PlayerTank::Render(const PlayerStats& stats) {
    // Tank shadow
    Texture* shadowTex = TextureManager::Get("Images/tankshadow.png");
    if (shadowTex) {
        Renderer::DrawTexture(shadowTex, x - 50.0f, y + 10.0f);
    }

    // Atomic Tank chassis & turret
    Texture* tankTex = TextureManager::Get("Images/atomictank.png");
    if (tankTex) {
        // chassis sprite: 0, 0, 102, 50
        Rect chassisSrc = { 0.0f, 0.0f, 102.0f, 50.0f };
        Rect chassisDst = { x - 51.0f, y - 25.0f, 102.0f, 50.0f };

        // Turret gun sprite: 0, 52, 60, 24
        Rect turretSrc = { 0.0f, 52.0f, 60.0f, 24.0f };
        float rad = turretAngle * 3.14159265f / 180.0f;
        float rx = std::cos(rad) * (-recoil);
        float ry = std::sin(rad) * (-recoil);

        Rect turretDst = { x - 10.0f + rx, y - 24.0f + ry, 60.0f, 24.0f };
        Renderer::DrawTextureRotated(tankTex, turretDst, turretSrc, turretAngle + 90.0f, 10.0f, 12.0f);

        // Draw chassis on top of turret mount
        if (isInvulnerable && (int)(invulnerableTimer * 10.0f) % 2 == 0) {
            Renderer::SetColor({ 0.5f, 0.8f, 1.0f, 0.6f });
        }
        Renderer::DrawTexture(tankTex, chassisDst, chassisSrc);
        Renderer::SetColor(Color4f::White());
    }

    // Defense Orbs
    int orbLvl = stats.weaponLevels[WEAPON_DEFENSE_ORBS];
    if (orbLvl > 0) {
        int orbCount = std::min(orbLvl, 3);
        float step = 360.0f / (float)orbCount;
        for (int i = 0; i < orbCount; ++i) {
            float a = (defenseOrbAngle + step * (float)i) * 3.14159265f / 180.0f;
            float ox = x + std::cos(a) * 55.0f;
            float oy = (y - 15.0f) + std::sin(a) * 30.0f; // Elliptical orbit
            Renderer::DrawFillRect(ox - 5.0f, oy - 5.0f, 10.0f, 10.0f, { 0.2f, 0.9f, 1.0f, 0.9f });
        }
    }

    // Megalaser Beam
    if (megalaserTimer > 0.0f) {
        Renderer::SetAdditiveBlend(true);
        float rad = turretAngle * 3.14159265f / 180.0f;
        float x2 = x + std::cos(rad) * 1200.0f;
        float y2 = (y - 20.0f) + std::sin(rad) * 1200.0f;
        Renderer::DrawLine(x, y - 20.0f, x2, y2, { 0.3f, 0.7f, 1.0f, 0.9f }, 16.0f);
        Renderer::DrawLine(x, y - 20.0f, x2, y2, { 1.0f, 1.0f, 1.0f, 1.0f }, 6.0f);
        Renderer::SetAdditiveBlend(false);
    }
}

// -------------------------------------------------------------
// Enemy implementation
// -------------------------------------------------------------

void Enemy::Update(float dt, float playerX, float playerY, std::vector<Projectile>& outEnemyProj) {
    x += vx * dt;
    y += vy * dt;

    animFrame += 10.0f * dt;

    // Firing behavior
    fireTimer -= dt;
    if (fireTimer <= 0.0f) {
        if (def.name == "PROPFIGHTER") {
            fireTimer = 999.0f; // props don't fire
        } else if (def.name == "SMALLJET" || def.name == "BOMBER" || def.name == "DELTABOMBER") {
            // Drop bomb when near player horizontally
            if (std::abs(x - playerX) < 180.0f) {
                Projectile b;
                b.type = (def.name == "DELTABOMBER") ? PROJ_ENEMY_ARMORED_BOMB : PROJ_ENEMY_BOMB;
                b.x = x;
                b.y = y + 15.0f;
                b.vx = vx * 0.4f;
                b.vy = 80.0f;
                b.damage = 1;
                outEnemyProj.push_back(b);
                fireTimer = 2.5f;
            }
        } else if (def.name == "SMALLCOPTER" || def.name == "MEDCOPTER" || def.name == "BIGCOPTER") {
            // Aimed energy cannon at player
            float dx = playerX - x;
            float dy = playerY - y;
            float len = std::sqrt(dx * dx + dy * dy);
            if (len > 1.0f) {
                Projectile b;
                b.type = PROJ_ENEMY_BULLET;
                b.x = x;
                b.y = y + 10.0f;
                b.vx = (dx / len) * 320.0f;
                b.vy = (dy / len) * 320.0f;
                b.damage = 1;
                outEnemyProj.push_back(b);
            }
            fireTimer = 1.8f;
        } else if (def.name == "BIGMISSILE") {
            // Rapid missile descent
            vy += 250.0f * dt;
        } else {
            fireTimer = 3.0f;
        }
    }

    // Despawn if offscreen far left/right or below ground
    if (x < -150.0f || x > SCREEN_WIDTH + 200.0f || y > GROUND_Y + 50.0f) {
        active = false;
    }
}

Rect Enemy::GetHitbox() const {
    float w = 50.0f;
    float h = 30.0f;
    if (def.name == "BLIMP") { w = 180.0f; h = 70.0f; }
    else if (def.name == "BIGBOMBER" || def.name == "SUPERBOMBER") { w = 90.0f; h = 40.0f; }
    return { x - w * 0.5f, y - h * 0.5f, w, h };
}

void Enemy::Render() {
    std::string lowerName = def.name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    std::string texPath = "Images/" + lowerName + ".png";
    Texture* tex = TextureManager::Get(texPath);

    if (tex) {
        Rect dst = { x - (float)tex->width * 0.5f, y - (float)tex->height * 0.5f, (float)tex->width, (float)tex->height };
        Renderer::DrawTexture(tex, dst.x, dst.y);
    } else {
        // Fallback representation
        Rect hb = GetHitbox();
        Renderer::DrawFillRect(hb.x, hb.y, hb.w, hb.h, { 0.8f, 0.3f, 0.3f, 1.0f });
    }

    // HP bar for heavy enemies
    if (maxHp > 10 && hp < maxHp) {
        float barW = 40.0f;
        float fillW = barW * ((float)hp / (float)maxHp);
        Renderer::DrawFillRect(x - barW * 0.5f, y - 25.0f, barW, 4.0f, { 0.2f, 0.2f, 0.2f, 0.8f });
        Renderer::DrawFillRect(x - barW * 0.5f, y - 25.0f, fillW, 4.0f, { 0.2f, 1.0f, 0.2f, 0.9f });
    }
}

} // namespace HeavyWeapon
