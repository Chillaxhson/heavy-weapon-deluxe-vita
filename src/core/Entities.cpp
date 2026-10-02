#include "Entities.h"
#include "InputManager.h"
#include "AudioSystem.h"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace HeavyWeapon {

// -------------------------------------------------------------
// Player Tank implementation
// -------------------------------------------------------------

void PlayerTank::Init() {
    x = 480.0f;
    y = TANK_DEFAULT_Y;
    vx = 0.0f;
    turretAngle = 90.0f;
    turretAngleCel = 10;
    recoil = 0.0f;
    fireCooldown = 0.0f;
    megalaserTimer = 0.0f;
    defenseOrbAngle = 0.0f;
    treadAnim = 0.0f;
    flameAnim = 0.0f;
    muzzleFlashTimer = 0.0f;
    isInvulnerable = true;
    invulnerableTimer = 2.5f;
    casings.clear();
}

void PlayerTank::TakeDamage() {
    if (isInvulnerable) return;
    isInvulnerable = true;
    invulnerableTimer = 2.5f;
    Renderer::AddScreenShake(14.0f, 0.45f);
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

    // Tread & exhaust animations
    if (std::abs(vx) > 10.0f) {
        float dir = (vx > 0.0f) ? 1.0f : -1.0f;
        treadAnim += dir * std::abs(vx) * dt * 0.06f;
        flameAnim += 35.0f * dt;
        if (flameAnim >= 20.0f) flameAnim = std::fmod(flameAnim, 20.0f);
    } else {
        flameAnim = 0.0f;
    }

    // Aiming calculation
    // Priority 1: Touch screen direct aim
    if (input.touchDown) {
        float dx = input.touchX - x;
        float dy = (y - 12.0f) - input.touchY;
        if (dy > -10.0f) { // Aiming generally upward
            turretAngle = std::atan2(dy, dx) * 180.0f / 3.14159265f;
            turretAngle = std::clamp(turretAngle, 0.0f, 180.0f);
        }
    }
    // Priority 2: Right analog stick
    else if (std::abs(input.aimAxisX) > 0.15f || std::abs(input.aimAxisY) > 0.15f) {
        float aimAng = std::atan2(-input.aimAxisY, input.aimAxisX) * 180.0f / 3.14159265f;
        turretAngle = std::clamp(aimAng, 0.0f, 180.0f);
    }

    // Map angle [0 deg (right) .. 90 deg (up) .. 180 deg (left)] to column [20 .. 10 .. 0]
    turretAngleCel = std::clamp((int)std::round((180.0f - turretAngle) / 9.0f), 0, 20);

    // Recoil recovery
    if (recoil > 0.0f) {
        recoil -= 35.0f * dt;
        if (recoil < 0.0f) recoil = 0.0f;
    }

    // Muzzle flash timer
    if (muzzleFlashTimer > 0.0f) {
        muzzleFlashTimer -= dt;
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
        float aimRad = (180.0f - (float)turretAngleCel * 9.0f) * 3.14159265f / 180.0f;
        Projectile laser;
        laser.type = PROJ_PLAYER_LASER;
        laser.x = x + std::cos(aimRad) * 40.0f;
        laser.y = (y - 12.0f) - std::sin(aimRad) * 40.0f;
        laser.vx = std::cos(aimRad) * 1800.0f;
        laser.vy = -std::sin(aimRad) * 1800.0f;
        laser.angle = turretAngle;
        laser.damage = 18;
        laser.life = 0.05f;
        outPlayerProj.push_back(laser);
    }

    // Primary weapon firing
    if (input.fireCannon && fireCooldown <= 0.0f) {
        int cannonLvl = std::clamp(stats.weaponLevels[WEAPON_CANNON], 1, 5);
        fireCooldown = 0.11f; // 9 rounds/sec
        recoil = 7.0f;
        muzzleFlashTimer = 0.08f;

        // Play authentic PopCap randomized cannon SFX
        int sfxNum = 1 + rand() % 4;
        std::string sfxName = "tankfire" + std::to_string(sfxNum);
        AudioSystem::PlaySound(sfxName);

        float aimRad = (180.0f - (float)turretAngleCel * 9.0f) * 3.14159265f / 180.0f;
        float barrelLen = 36.0f;
        float muzzleX = x + std::cos(aimRad) * barrelLen;
        float muzzleY = (y - 12.0f) - std::sin(aimRad) * barrelLen;
        float bulletSpeed = 980.0f;

        // Spread angles for upgraded cannons
        if (cannonLvl == 1) {
            Projectile p;
            p.type = PROJ_PLAYER_CANNON;
            p.x = muzzleX;
            p.y = muzzleY;
            p.vx = std::cos(aimRad) * bulletSpeed;
            p.vy = -std::sin(aimRad) * bulletSpeed;
            p.angleCel = turretAngleCel;
            p.level = 0;
            p.damage = 1;
            outPlayerProj.push_back(p);
        } else if (cannonLvl == 2) {
            for (float spread : { -2.5f, 2.5f }) {
                float r = (180.0f - (float)turretAngleCel * 9.0f + spread) * 3.14159265f / 180.0f;
                Projectile p;
                p.type = PROJ_PLAYER_CANNON;
                p.x = muzzleX;
                p.y = muzzleY;
                p.vx = std::cos(r) * bulletSpeed;
                p.vy = -std::sin(r) * bulletSpeed;
                p.angleCel = turretAngleCel;
                p.level = 1;
                p.damage = 1;
                outPlayerProj.push_back(p);
            }
        } else if (cannonLvl == 3) {
            for (float spread : { -4.5f, 0.0f, 4.5f }) {
                float r = (180.0f - (float)turretAngleCel * 9.0f + spread) * 3.14159265f / 180.0f;
                Projectile p;
                p.type = PROJ_PLAYER_CANNON;
                p.x = muzzleX;
                p.y = muzzleY;
                p.vx = std::cos(r) * bulletSpeed;
                p.vy = -std::sin(r) * bulletSpeed;
                p.angleCel = turretAngleCel;
                p.level = 2;
                p.damage = 2;
                outPlayerProj.push_back(p);
            }
        } else { // Level 4 and 5
            for (float spread : { -6.0f, -2.0f, 2.0f, 6.0f }) {
                float r = (180.0f - (float)turretAngleCel * 9.0f + spread) * 3.14159265f / 180.0f;
                Projectile p;
                p.type = PROJ_PLAYER_CANNON;
                p.x = muzzleX;
                p.y = muzzleY;
                p.vx = std::cos(r) * bulletSpeed;
                p.vy = -std::sin(r) * bulletSpeed;
                p.angleCel = turretAngleCel;
                p.level = cannonLvl - 1;
                p.damage = (cannonLvl >= 5) ? 3 : 2;
                outPlayerProj.push_back(p);
            }
        }

        // Eject authentic brass shell casing
        EjectedCasing c;
        c.x = x;
        c.y = y - 12.0f;
        c.vx = (vx * 0.2f) - std::cos(aimRad) * 110.0f + (float)(rand() % 40 - 20);
        c.vy = -180.0f - (float)(rand() % 80);
        c.rot = (float)(rand() % 360);
        c.vrot = (float)(rand() % 500 - 250);
        c.life = 1.0f;
        casings.push_back(c);

        // Secondary: Homing missiles
        int missileLvl = stats.weaponLevels[WEAPON_HOMING_MISSILES];
        if (missileLvl > 0 && (rand() % 3 == 0)) {
            Projectile m;
            m.type = PROJ_PLAYER_MISSILE;
            m.x = x;
            m.y = y - 25.0f;
            m.vx = (float)(rand() % 140 - 70);
            m.vy = -520.0f;
            m.damage = 2 * missileLvl;
            outPlayerProj.push_back(m);
            AudioSystem::PlaySound("missile");
        }

        // Secondary: Flak cannon
        int flakLvl = stats.weaponLevels[WEAPON_FLAK];
        if (flakLvl > 0 && (rand() % 4 == 0)) {
            Projectile f;
            f.type = PROJ_PLAYER_FLAK;
            f.x = muzzleX;
            f.y = muzzleY;
            f.vx = std::cos(aimRad) * 700.0f;
            f.vy = -std::sin(aimRad) * 700.0f;
            f.damage = 4 * flakLvl;
            outPlayerProj.push_back(f);
            AudioSystem::PlaySound("flak");
        }
    }

    // Update Casings
    for (auto& c : casings) {
        if (!c.active) continue;
        c.x += c.vx * dt;
        c.y += c.vy * dt;
        c.vy += 750.0f * dt; // Gravity
        c.rot += c.vrot * dt;
        c.life -= dt;

        // Ground collision bounce
        if (c.y >= GROUND_Y - 2.0f) {
            c.y = GROUND_Y - 2.0f;
            c.vy = -c.vy * 0.35f;
            c.vx *= 0.5f;
            c.vrot *= 0.4f;
        }

        if (c.life <= 0.0f) {
            c.active = false;
        }
    }
    casings.erase(std::remove_if(casings.begin(), casings.end(), [](const EjectedCasing& c){ return !c.active; }), casings.end());
}

Rect PlayerTank::GetHitbox() const {
    return { x - 35.0f, y - 25.0f, 70.0f, 35.0f };
}

void PlayerTank::Render(const PlayerStats& stats) {
    // Layer 1: Ground shadow (tankshadow.png, 80x20) centered at (x, y + 22)
    Texture* shadowTex = TextureManager::Get("tankshadow");
    if (shadowTex) {
        Renderer::DrawCel(shadowTex, 0, 0, x, y + 22.0f, true);
    }

    // Layer 2: Animated exhaust flame (tankflame.png, 40x40, 20 frames)
    if (flameAnim > 0.0f) {
        Texture* flameTex = TextureManager::Get("tankflame");
        if (flameTex) {
            int flameFrame = ((int)flameAnim) % 20;
            float flameX = (vx >= 0.0f) ? (x - 38.0f) : (x + 38.0f);
            float flameScaleX = (vx >= 0.0f) ? -1.0f : 1.0f;
            Renderer::SetAdditiveBlend(true);
            Renderer::DrawCel(flameTex, flameFrame, 0, flameX, y + 6.0f, true, flameScaleX, 1.0f);
            Renderer::SetAdditiveBlend(false);
        }
    }

    // Layer 3: Animated tank chassis & treads (tank.png, 80x55, 10 frames)
    Texture* tankTex = TextureManager::Get("tank");
    if (tankTex) {
        int tankFrame = ((int)std::abs(treadAnim)) % 10;
        if (isInvulnerable && (int)(invulnerableTimer * 12.0f) % 2 == 0) {
            Renderer::SetColor({ 0.6f, 0.85f, 1.0f, 0.7f });
        }
        Renderer::DrawCel(tankTex, tankFrame, 0, x, y, true);
        Renderer::SetColor(Color4f::White());
    }

    // Layer 4: Turret gun (gun.png, 80x50, 21 angles, 5 rows)
    // Drawn at (x - 40, y - 52) top-left
    Texture* gunTex = TextureManager::Get("gun");
    if (gunTex) {
        int cannonLvl = std::clamp(stats.weaponLevels[WEAPON_CANNON], 1, 5);
        float aimRad = (180.0f - (float)turretAngleCel * 9.0f) * 3.14159265f / 180.0f;
        float rx = -std::cos(aimRad) * recoil;
        float ry = std::sin(aimRad) * recoil;
        Renderer::DrawCel(gunTex, turretAngleCel, cannonLvl - 1, x - 40.0f + rx, y - 52.0f + ry, false);
    }

    // Layer 5: Secondary weapon mounts
    // Lasers (lasers.png, 80x40, 21 angles, 3 rows) at (x - 40, y - 42)
    if (stats.weaponLevels[WEAPON_LASER] > 0) {
        Texture* laserTex = TextureManager::Get("lasers");
        if (laserTex) {
            int lvl = std::clamp(stats.weaponLevels[WEAPON_LASER], 1, 3);
            Renderer::DrawCel(laserTex, turretAngleCel, lvl - 1, x - 40.0f, y - 42.0f, false);
        }
    }

    // Flak gun (flakguns.jpg, 80x60, 21 angles, 3 rows) at (x - 40, y - 52)
    if (stats.weaponLevels[WEAPON_FLAK] > 0) {
        Texture* flakTex = TextureManager::Get("flakguns");
        if (flakTex) {
            int lvl = std::clamp(stats.weaponLevels[WEAPON_FLAK], 1, 3);
            Renderer::DrawCel(flakTex, turretAngleCel, lvl - 1, x - 40.0f, y - 52.0f, false);
        }
    }

    // Rocket pod (rocketpod.png, 24x24, 21 angles) at (x - 12, y - 48)
    if (stats.weaponLevels[WEAPON_HOMING_MISSILES] > 0) {
        Texture* podTex = TextureManager::Get("rocketpod");
        if (podTex) {
            Renderer::DrawCel(podTex, turretAngleCel, 0, x - 12.0f, y - 48.0f, false);
        }
    }

    // Defense Orbs (orb.png, 2 frames) orbiting in ellipse (rx=50, ry=40)
    int orbLvl = stats.weaponLevels[WEAPON_DEFENSE_ORBS];
    if (orbLvl > 0) {
        Texture* orbTex = TextureManager::Get("orb");
        int orbCount = std::min(orbLvl, 3);
        float step = 360.0f / (float)orbCount;
        for (int i = 0; i < orbCount; ++i) {
            float a = (defenseOrbAngle + step * (float)i) * 3.14159265f / 180.0f;
            float ox = x + std::cos(a) * 50.0f;
            float oy = (y - 12.0f) + std::sin(a) * 36.0f;
            if (orbTex) {
                Renderer::DrawCel(orbTex, 0, 0, ox, oy, true);
            } else {
                Renderer::DrawFillRect(ox - 5.0f, oy - 5.0f, 10.0f, 10.0f, { 0.2f, 0.9f, 1.0f, 0.9f });
            }
        }
    }

    // Layer 6: Muzzle flash (muzzleflash.png, 50x50, 5 frames)
    if (muzzleFlashTimer > 0.0f) {
        Texture* flashTex = TextureManager::Get("muzzleflash");
        if (flashTex) {
            float aimRad = (180.0f - (float)turretAngleCel * 9.0f) * 3.14159265f / 180.0f;
            float barrelLen = 38.0f;
            float muzzleX = x + std::cos(aimRad) * barrelLen;
            float muzzleY = (y - 12.0f) - std::sin(aimRad) * barrelLen;
            int flashFrame = (int)((1.0f - (muzzleFlashTimer / 0.08f)) * 4.9f);
            flashFrame = std::clamp(flashFrame, 0, 4);
            Renderer::SetAdditiveBlend(true);
            Renderer::DrawCel(flashTex, flashFrame, 0, muzzleX, muzzleY, true);
            Renderer::SetAdditiveBlend(false);
        }
    }

    // Layer 7: Ejected brass shell casings
    Texture* casingTex = TextureManager::Get("casing");
    if (casingTex) {
        for (const auto& c : casings) {
            if (c.active) {
                Renderer::DrawCel(casingTex, 0, 0, c.x, c.y, true, 1.0f, 1.0f, c.rot);
            }
        }
    }

    // Layer 8: Spawn shield (shield.jpg / shieldzap.png)
    if (isInvulnerable) {
        Texture* shieldTex = TextureManager::Get("shield");
        if (shieldTex) {
            int shieldFrame = ((int)(invulnerableTimer * 20.0f)) % 10;
            Renderer::SetAdditiveBlend(true);
            Renderer::SetColor({ 0.5f, 0.8f, 1.0f, 0.75f });
            Renderer::DrawCel(shieldTex, shieldFrame, 0, x, y - 8.0f, true);
            Renderer::SetColor(Color4f::White());
            Renderer::SetAdditiveBlend(false);
        }
    }

    // Megalaser Beam effect
    if (megalaserTimer > 0.0f) {
        Renderer::SetAdditiveBlend(true);
        float aimRad = (180.0f - (float)turretAngleCel * 9.0f) * 3.14159265f / 180.0f;
        float x2 = x + std::cos(aimRad) * 1200.0f;
        float y2 = (y - 12.0f) - std::sin(aimRad) * 1200.0f;
        Renderer::DrawLine(x, y - 12.0f, x2, y2, { 0.3f, 0.7f, 1.0f, 0.9f }, 16.0f);
        Renderer::DrawLine(x, y - 12.0f, x2, y2, { 1.0f, 1.0f, 1.0f, 1.0f }, 6.0f);
        Renderer::SetAdditiveBlend(false);
    }
}

// -------------------------------------------------------------
// Projectile rendering
// -------------------------------------------------------------

void Projectile::Render() const {
    if (!active) return;

    if (type == PROJ_PLAYER_CANNON) {
        Texture* tex = TextureManager::Get("bullets");
        if (tex) {
            Renderer::DrawCel(tex, angleCel, level, x, y, true);
        } else {
            Renderer::DrawFillRect(x - 3.0f, y - 3.0f, 6.0f, 6.0f, { 1.0f, 0.9f, 0.3f, 1.0f });
        }
    } else if (type == PROJ_PLAYER_MISSILE || type == PROJ_ENEMY_MISSILE) {
        Texture* tex = TextureManager::Get("missile");
        if (tex) {
            int frame = ((int)animFrame) % 20;
            Renderer::DrawCel(tex, frame, 0, x, y, true);
        } else {
            Renderer::DrawFillRect(x - 4.0f, y - 4.0f, 8.0f, 8.0f, { 1.0f, 0.4f, 0.2f, 1.0f });
        }
    } else if (type == PROJ_ENEMY_BOMB) {
        Texture* tex = TextureManager::Get("dumbbomb");
        if (tex) {
            int frame = ((int)animFrame) % 10;
            Renderer::DrawCel(tex, frame, 0, x, y, true);
        } else {
            Renderer::DrawFillRect(x - 5.0f, y - 5.0f, 10.0f, 10.0f, { 0.9f, 0.9f, 0.9f, 1.0f });
        }
    } else if (type == PROJ_ENEMY_ARMORED_BOMB) {
        Texture* tex = TextureManager::Get("ironbomb");
        if (tex) {
            int frame = ((int)animFrame) % 10;
            Renderer::DrawCel(tex, frame, 0, x, y, true);
        } else {
            Renderer::DrawFillRect(x - 5.0f, y - 5.0f, 10.0f, 10.0f, { 0.4f, 0.4f, 0.4f, 1.0f });
        }
    } else if (type == PROJ_ENEMY_FRAG_BOMB) {
        Texture* tex = TextureManager::Get("fragbomb");
        if (tex) {
            int frame = ((int)animFrame) % 10;
            Renderer::DrawCel(tex, frame, 0, x, y, true);
        } else {
            Renderer::DrawFillRect(x - 5.0f, y - 5.0f, 10.0f, 10.0f, { 1.0f, 0.8f, 0.2f, 1.0f });
        }
    } else if (type == PROJ_ENEMY_FATBOY) {
        Texture* tex = TextureManager::Get("fatboy");
        if (tex) {
            int frame = ((int)animFrame) % 10;
            Renderer::DrawCel(tex, frame, 0, x, y, true);
        } else {
            Renderer::DrawFillRect(x - 10.0f, y - 10.0f, 20.0f, 20.0f, { 0.8f, 0.2f, 0.2f, 1.0f });
        }
    } else if (type == PROJ_ENEMY_LGB) {
        Texture* tex = TextureManager::Get("lgb");
        if (tex) {
            int frame = ((int)animFrame) % 21;
            Renderer::DrawCel(tex, frame, 0, x, y, true);
        } else {
            Renderer::DrawFillRect(x - 5.0f, y - 5.0f, 10.0f, 10.0f, { 1.0f, 0.2f, 0.2f, 1.0f });
        }
    } else {
        // Generic bullet / energy cannon
        Texture* tex = TextureManager::Get("bullets");
        if (tex) {
            Renderer::DrawCel(tex, 10, 0, x, y, true, 1.1f, 1.1f);
        } else {
            Renderer::DrawFillRect(x - 3.0f, y - 3.0f, 6.0f, 6.0f, { 1.0f, 0.3f, 0.3f, 1.0f });
        }
    }
}

// -------------------------------------------------------------
// Explosion instance
// -------------------------------------------------------------

void ExplosionInstance::Update(float dt) {
    animFrame += 30.0f * dt;
    if (animFrame >= 20.0f) {
        active = false;
    }
}

void ExplosionInstance::Render() const {
    if (!active) return;
    Texture* expTex = TextureManager::Get("explosion");
    if (expTex) {
        int frame = std::clamp((int)animFrame, 0, 19);
        Renderer::DrawCel(expTex, frame, 0, x, y, true, scale, scale);
    }
}

// -------------------------------------------------------------
// PowerUp Item
// -------------------------------------------------------------

void PowerUpItem::Render() const {
    if (!active) return;
    if (isNuke) {
        Texture* nukeTex = TextureManager::Get("nukeicon");
        if (nukeTex) {
            Renderer::DrawCel(nukeTex, 0, 0, x, y, true, 1.3f, 1.3f);
        }
    } else {
        Texture* pupTex = TextureManager::Get("powerups");
        if (pupTex) {
            Renderer::DrawCel(pupTex, cel % 12, 0, x, y, true);
        }
    }
}

// -------------------------------------------------------------
// Enemy implementation
// -------------------------------------------------------------

void Enemy::TakeDamage(int dmg) {
    hp -= dmg;
    hitFlashTimer = 0.08f;
}

void Enemy::Update(float dt, float playerX, float playerY, std::vector<Projectile>& outEnemyProj) {
    x += vx * dt;
    y += vy * dt;

    animFrame += 15.0f * dt;
    rotorAnim += 35.0f * dt;

    if (hitFlashTimer > 0.0f) {
        hitFlashTimer -= dt;
    }

    // Firing behavior
    fireTimer -= dt;
    if (fireTimer <= 0.0f) {
        if (def.name == "PROPFIGHTER") {
            fireTimer = 999.0f; // Props don't fire
        } else if (def.name == "SMALLJET") {
            if (std::abs(x - playerX) < 160.0f) {
                Projectile b;
                b.type = PROJ_ENEMY_BOMB;
                b.x = x;
                b.y = y + 10.0f;
                b.vx = vx * 0.4f;
                b.vy = 90.0f;
                b.damage = 1;
                outEnemyProj.push_back(b);
                AudioSystem::PlaySound("bombfall", 0.6f);
                fireTimer = 2.5f;
            }
        } else if (def.name == "BOMBER" || def.name == "BIGBOMBER") {
            if (std::abs(x - playerX) < 220.0f) {
                Projectile b;
                b.type = PROJ_ENEMY_BOMB;
                b.x = x;
                b.y = y + 15.0f;
                b.vx = vx * 0.35f;
                b.vy = 95.0f;
                b.damage = 1;
                outEnemyProj.push_back(b);
                AudioSystem::PlaySound("bombfall", 0.6f);
                fireTimer = 1.6f;
            }
        } else if (def.name == "DELTABOMBER") {
            if (std::abs(x - playerX) < 200.0f) {
                Projectile b;
                b.type = PROJ_ENEMY_FRAG_BOMB;
                b.x = x;
                b.y = y + 15.0f;
                b.vx = vx * 0.35f;
                b.vy = 95.0f;
                b.damage = 1;
                outEnemyProj.push_back(b);
                AudioSystem::PlaySound("bombfall", 0.6f);
                fireTimer = 2.0f;
            }
        } else if (def.name == "DELTAJET" || def.name == "SUPERBOMBER") {
            if (std::abs(x - playerX) < 200.0f) {
                Projectile b;
                b.type = PROJ_ENEMY_ARMORED_BOMB;
                b.x = x;
                b.y = y + 15.0f;
                b.vx = vx * 0.35f;
                b.vy = 100.0f;
                b.damage = 1;
                outEnemyProj.push_back(b);
                AudioSystem::PlaySound("bombfall", 0.6f);
                fireTimer = 2.0f;
            }
        } else if (def.name == "FATBOMBER") {
            if (std::abs(x - playerX) < 180.0f) {
                Projectile b;
                b.type = PROJ_ENEMY_FATBOY;
                b.x = x;
                b.y = y + 25.0f;
                b.vx = vx * 0.2f;
                b.vy = 70.0f;
                b.damage = 2;
                outEnemyProj.push_back(b);
                AudioSystem::PlaySound("bombfall", 0.8f);
                fireTimer = 3.0f;
            }
        } else if (def.name == "JETFIGHTER") {
            if (std::abs(x - playerX) < 250.0f) {
                Projectile b;
                b.type = PROJ_ENEMY_LGB;
                b.x = x;
                b.y = y + 12.0f;
                b.vx = vx * 0.5f;
                b.vy = 110.0f;
                b.damage = 1;
                outEnemyProj.push_back(b);
                AudioSystem::PlaySound("bombfall", 0.6f);
                fireTimer = 2.2f;
            }
        } else if (def.name == "SMALLCOPTER" || def.name == "MEDCOPTER" || def.name == "BIGCOPTER") {
            float dx = playerX - x;
            float dy = playerY - y;
            float len = std::sqrt(dx * dx + dy * dy);
            if (len > 1.0f) {
                Projectile b;
                b.type = (def.name == "BIGCOPTER") ? PROJ_ENEMY_MISSILE : PROJ_ENEMY_BULLET;
                b.x = x;
                b.y = y + 10.0f;
                b.vx = (dx / len) * 340.0f;
                b.vy = (dy / len) * 340.0f;
                b.damage = 1;
                outEnemyProj.push_back(b);
                AudioSystem::PlaySound("enemyfire", 0.6f);
            }
            fireTimer = (def.name == "SMALLCOPTER") ? 1.8f : (def.name == "MEDCOPTER") ? 1.5f : 1.2f;
        } else if (def.name == "BIGMISSILE") {
            vy += 280.0f * dt; // Rapid missile descent
        } else {
            fireTimer = 3.0f;
        }
    }

    // Off-screen despawn
    if (x < -180.0f || x > SCREEN_WIDTH + 250.0f || y > GROUND_Y + 60.0f) {
        active = false;
    }
}

Rect Enemy::GetHitbox() const {
    float w = 50.0f;
    float h = 30.0f;
    if (def.name == "BLIMP") { w = 220.0f; h = 75.0f; }
    else if (def.name == "BIGBOMBER" || def.name == "SUPERBOMBER") { w = 110.0f; h = 45.0f; }
    else if (def.name == "FATBOMBER") { w = 140.0f; h = 60.0f; }
    else if (def.name == "BIGCOPTER") { w = 85.0f; h = 50.0f; }
    return { x - w * 0.5f, y - h * 0.5f, w, h };
}

void Enemy::Render() const {
    std::string lowerName = def.name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

    // Ground vehicle shadows
    if (def.name == "TRUCK" || def.name == "ENEMYTANK") {
        Texture* shadowTex = TextureManager::Get("enemytankshadow");
        if (shadowTex) Renderer::DrawCel(shadowTex, 0, 0, x, GROUND_Y - 5.0f, true);
    } else if (def.name == "DOZER") {
        Texture* shadowTex = TextureManager::Get("dozershadow");
        if (shadowTex) Renderer::DrawCel(shadowTex, 0, 0, x, GROUND_Y - 5.0f, true);
    }

    // Flash white on hit
    if (hitFlashTimer > 0.0f) {
        Renderer::SetColor({ 1.0f, 1.0f, 1.0f, 0.75f });
    }

    Texture* tex = TextureManager::Get(lowerName);
    if (tex) {
        int col = 0;
        if (tex->cols > 1) {
            col = ((int)animFrame) % tex->cols;
        }
        Renderer::DrawCel(tex, col, 0, x, y, true);

        // Multi-part helicopter rotor rendering
        if (def.name == "SMALLCOPTER" || def.name == "BIGCOPTER") {
            Texture* rotorTex = TextureManager::Get("copterblades");
            if (rotorTex) {
                int rFrame = ((int)rotorAnim) % 10;
                Renderer::DrawCel(rotorTex, rFrame, 0, x, y - 22.0f, true);
            }
        } else if (def.name == "MEDCOPTER") {
            Texture* rotorTex = TextureManager::Get("medrotor");
            if (rotorTex) {
                int rFrame = ((int)rotorAnim) % 6;
                Renderer::DrawCel(rotorTex, rFrame, 0, x, y - 20.0f, true);
            }
        } else if (def.name == "PUPCOPTER") {
            Texture* rotorTex = TextureManager::Get("puprotor");
            if (rotorTex) {
                int rFrame = ((int)rotorAnim) % 7;
                Renderer::DrawCel(rotorTex, rFrame, 0, x, y - 18.0f, true);
            }
        } else if (def.name == "BLIMP") {
            Texture* propTex = TextureManager::Get("blimpprop");
            if (propTex) {
                int pFrame = ((int)rotorAnim) % 5;
                Renderer::DrawCel(propTex, pFrame, 0, x + 130.0f, y + 10.0f, true);
            }
        }
    } else {
        // Fallback
        Rect hb = GetHitbox();
        Renderer::DrawFillRect(hb.x, hb.y, hb.w, hb.h, { 0.8f, 0.3f, 0.3f, 1.0f });
    }

    Renderer::SetColor(Color4f::White());

    // Boss or Heavy Enemy Health Bar
    if (maxHp > 30 && hp < maxHp) {
        float barW = 50.0f;
        float fillW = barW * ((float)hp / (float)maxHp);
        Renderer::DrawFillRect(x - barW * 0.5f, y - 32.0f, barW, 4.0f, { 0.2f, 0.2f, 0.2f, 0.8f });
        Renderer::DrawFillRect(x - barW * 0.5f, y - 32.0f, fillW, 4.0f, { 0.2f, 1.0f, 0.2f, 0.9f });
    }
}

} // namespace HeavyWeapon
