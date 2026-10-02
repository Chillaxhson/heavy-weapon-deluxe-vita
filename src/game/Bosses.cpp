// Bosses. Each boss is a craft that arrives as the level runs out (Board::Update spawns
// it at progress == length-1 by mission % 9) plus craft "parts" riding on it. Stats come
// from bosses.xml, Level = mission / 9 + 1 (0x43f910).

#include "game/Bosses.h"
#include "game/Board.h"
#include "game/Enemies.h"
#include "game/Gfx.h"
#include "AudioSystem.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace HeavyWeapon {

static int Rand() { return std::rand() & 0x7fffffff; }

// Sub-folder boss images are not in the game's image table, so their grids come from the
// boss constructors (0x42d500 calls).
static Texture* Grid(const char* path, int cols, int rows = 1) {
    return TextureManager::LoadGrid(path, cols, rows);
}

static const BossLevelDef* Stats(Board& b, const char* type) {
    const BossDef* def = b.BossStats(type);
    if (!def || def->levels.empty()) return nullptr;
    size_t level = std::min((size_t)(b.App().mission / 9), def->levels.size() - 1);
    return &def->levels[level];
}

namespace {

// ---------------------------------------------------------------------------------
// Helicopter "Twinblade" (0x440300): hovers over the middle with two missile launchers,
// plus two sweeping energy-cannon turrets from its second encounter on.
// ---------------------------------------------------------------------------------

struct BossCopter;

// A part that rides on its boss at a fixed offset and becomes hittable with it.
struct BossPart : Craft {
    Craft& boss;
    double ox, oy;
    int fireDelay, timer;
    BossPart(Board& bd, Craft& owner, Texture* image, double hp_, double ox_, double oy_, int fire)
        : Craft(bd, 0, "pupcopter"), boss(owner), ox(ox_), oy(oy_), fireDelay(fire), timer(fire) {
        img = image;
        hp = maxHp = hp_;
        points = 0;
        hittable = false;
        b.CountCraft(-1, 0, 0);     // parts do not count toward the kill percentage
    }
    bool Follow() {
        if (b.Progress() < b.Length()) return false;
        Craft::Update();
        x = (int)boss.x + ox;
        y = (int)boss.y + oy;
        vx = boss.vx;
        vy = boss.vy;
        hittable = boss.hittable;
        return true;
    }
    void UpdateF() override {}
    void Die() override {
        Craft::Die();
        b.CountCraft(0, -1, 0);
    }
    void Draw() override {
        if (b.Progress() >= b.Length()) Craft::Draw();
    }
};

// Missile launcher (0x440780).
struct CopterLauncher : BossPart {
    double speed;
    CopterLauncher(Board& bd, Craft& owner, const BossLauncherDef& d, double ox_)
        : BossPart(bd, owner, Grid("Images/hugecopter/launcher", 1), d.armor, ox_, 45.0, d.fireInterval), speed(d.speed) {}
    void Update() override {
        if (!Follow() || !boss.hittable) return;
        timer = b.TankDead() ? fireDelay : timer - 1;
        if (x > -300.0 && x < 300.0 && timer < 1 && boss.y > 50.0 && b.NukeFlash() == 0.0 && !b.BossDying()) {
            FireMissile(b, (int)x, (int)y, vx, vy, speed);
            timer = fireDelay;
        }
    }
};

// Sweeping energy cannon (0x440a20).
struct CopterTurret : BossPart {
    double sweep;
    CopterTurret(Board& bd, Craft& owner, const BossTurretDef& d, double ox_, double sweep_)
        : BossPart(bd, owner, Grid("Images/hugecopter/turret", 21), d.armor, ox_, 33.0, d.fireInterval), sweep(sweep_) {
        frame = 10.0;
    }
    void Update() override {
        if (!Follow()) return;
        frame += sweep;
        int cols = img ? img->cols : 21;
        if (frame >= cols) {
            sweep = -0.1;
            frame = cols - 1;
        } else if (frame < 0.0) {
            sweep = 0.1;
            frame = 0.0;
        }
        timer = b.TankDead() ? fireDelay : timer - 1;
        if (timer < 1 && boss.y > 50.0 && !b.TankDead() && b.NukeFlash() == 0.0 && !b.BossDying()) {
            double a = frame / (cols * 0.6369426751592356) + 0.7854;
            b.FireEnemyShot((int)(x - std::cos(a) * 30.0), (int)(std::sin(a) * 30.0 + y), a, 0.0);
            timer = fireDelay;
        }
    }
};

struct BossCopter : Craft {
    Texture* blades;
    double bladeFrame = 0.0;
    BossCopter(Board& bd) : Craft(bd, 0, "pupcopter") {
        img = Grid("Images/hugecopter/hugecopter", 1);
        blades = Grid("Images/hugecopter/copterblades", 5);
        const BossLevelDef* st = Stats(b, "Helicopter");
        hp = maxHp = st ? st->armor : 300;
        points = st ? st->score : 10000;
        boss = true;
        persistent = true;
        hittable = false;
        x = 0.0;
        y = -(double)H();
        if (st) {
            BossLauncherDef ld = st->launchers.empty() ? BossLauncherDef() : st->launchers[0];
            b.AddPart(std::make_unique<CopterLauncher>(b, *this, ld, -75.0));
            b.AddPart(std::make_unique<CopterLauncher>(b, *this, ld, 75.0));
            if (b.App().mission >= 1 && !st->turrets.empty()) {
                b.AddPart(std::make_unique<CopterTurret>(b, *this, st->turrets[0], -122.0, -0.1));
                b.AddPart(std::make_unique<CopterTurret>(b, *this, st->turrets[0], 122.0, 0.1));
            }
        }
    }
    ~BossCopter() override { AudioSystem::SetLoop(SND_BOSSCOPTER, false); }
    void UpdateF() override {}
    void Update() override {
        Craft::Update();
        bladeFrame += 0.4;
        if (bladeFrame >= 5.0) bladeFrame -= 5.0;
        x += vx;
        y += vy;
        // Descend to the hover band, then drift around the middle.
        if (y >= 140.0) {
            hittable = true;
            vy += (y <= 200.0) ? (double)(Rand() % 20) * 0.001 - 0.01 : -0.01;
        } else {
            vy += 0.01;
        }
        if (x < -120.0) vx += 0.01;
        else if (x > 120.0) vx -= 0.01;
        else vx += (double)(Rand() % 20) * 0.001 - 0.01;
        vx = std::clamp(vx, -0.5, 0.5);
        vy = std::clamp(vy, -0.5, 0.5);
        AudioSystem::SetLoop(SND_BOSSCOPTER, true, b.Pan(x * 2.0));
    }
    void Draw() override {
        if (b.Progress() < b.Length()) return;
        if (flash != 0) {
            Gfx::SetColorizeImages(true);
            Gfx::SetColor(255, 255 - flash, 255 - flash);
        }
        Gfx::DrawSprite(img, (int)x, (int)y, true, 0, 0, mirror);
        Gfx::DrawSprite(blades, (int)(x - 122.0), (int)(y - 68.0), true, (int)bladeFrame);
        Gfx::DrawSprite(blades, (int)(x + 122.0), (int)(y - 68.0), true, (int)bladeFrame);
        Gfx::SetColorizeImages(false);
    }
    // Slot 4 (0x43f800): the score, then the death sequence takes over.
    void Die() override {
        if (b.BossDying()) return;
        b.CountCraft(0, 1, 0);
        b.AddScore(points, (int)x, (int)y);
        b.StartBossDeath();
    }
};

} // namespace

std::unique_ptr<Craft> CreateBoss(Board& b, int mission) {
    int kind = (mission == 18) ? 9 : mission % 9;
    switch (kind) {
        case 0: return std::make_unique<BossCopter>(b);
        default: return nullptr;   // not ported yet: the post-boss sequence runs straight away
    }
}

} // namespace HeavyWeapon
