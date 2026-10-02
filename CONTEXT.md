# Project Context: Heavy Weapon Deluxe - PS Vita Port

* **Repository**: `git@github.com:Chillaxhson/heavy-weapon-deluxe-vita.git`
* **Target Hardware**: Sony PlayStation Vita (ARM Cortex-A9, SGX543MP4+, 960×544, 60 FPS)
* **Build Artifact**: `build/HeavyWeaponDeluxe.vpk` (Title ID: `HWDL00001`)

---

## 1. Technical Analysis & Discoveries

1. **Asset Structure**:
   * Inspecting the Steam release in `Heavy Weapon Deluxe/` revealed that all assets are loose/unpacked rather than packed into an encrypted `.pak` file.
   * `data/`: All game configuration files are human-readable XML:
     * `data/levels.xml`: 19 missions with population stats, export descriptions, and mission lengths.
     * `data/craft.xml`: 20+ enemy aircraft, bombers, helicopters, trucks, missiles, and stats (armor, points, arms).
     * `data/bosses.xml`: Full multi-part boss parameters (Twinblade, Battleship, War Blimp, etc.).
     * `data/waves.xml`: Exact wave progression tables for all 19 missions.
     * `data/survival0.xml`–`survival9.xml`: Survival mode wave definitions.
   * `Images/`:
     * Background textures (`*_bg.jpg`, `*_bg2.png`, etc.) are natively **960 pixels wide**, precisely matching the PS Vita screen resolution (960×544).
     * Dual-file alpha masking is used: color channels in `.jpg` and 8-bit alpha channels in `_.png`.
     * `Anims/Anims.xml`: 4-layer parallax plane specifications (Sky, Far BG, Mid BG, Ground) with frame counts, offsets, and speeds.
   * `Sounds/`: High-quality OGG Vorbis sound effects.
   * `Music/`: `AtomicTank.mo3` (MO3 module) and `LoveTheme.ogg`.

2. **Binary Analysis**:
   * `Heavy Weapon Deluxe.exe` is an unpacked Win32 binary with ~256 KB `.text` section compiled with MSVC against PopCap's SexyAppFramework v1.3.

---

## 2. Architecture & Subsystems Implemented

* **Platform / Virtual File System (`Vfs`)**:
  * Case-insensitive POSIX file resolver.
  * Checks `ux0:data/heavyweapon/`, `app0:`, `./Heavy Weapon Deluxe/`, and `./`.
* **Renderer & Texture Pipeline (`Renderer`, `TextureManager`)**:
  * Hardware-accelerated 2D quad/sprite renderer via **VitaGL** and **SDL2**.
  * Dual-file PopCap texture loader merging `.jpg` color channels with `_.png` alpha masks into 32-bit RGBA GL textures.
  * Additive blending for laser beams, explosions, and shield effects.
  * Screen shake physics for detonations and damage.
* **Typography (`FontRenderer`)**:
  * Parser for PopCap `ImageFont` metrics (`.txt`) and glyph sheets (`.png`).
  * Supports alignment (Left, Center, Right), scaling, and kerning.
* **Audio Engine (`AudioSystem`)**:
  * Powered by `SDL2_mixer` for low-latency sound effects across 32 channels.
  * Background music streaming with volume controls.
* **Controls & Input (`InputManager`)**:
  * Modern twin-stick controls: Left stick drives tank, Right stick aims 360°/180° with auto-fire threshold.
  * Hardware buttons: Cross/R1 (Cannon), Triangle/L1 (Nuke), Square (Megalaser), Start (Pause).
  * Direct touchscreen support for navigating menus and selecting Armory upgrades.
* **World Simulation & Parallax (`WorldRenderer`)**:
  * 4-plane horizontal parallax scrolling with wrapping and ambient animations from `Anims.xml`.
* **Entities & Game Logic (`PlayerTank`, `Enemy`, `Projectile`, `GameEngine`)**:
  * Full player physics, turret recoil, weapon upgrades (Level 1–3 Cannon, Laser, Flak, Homing Missiles, Defense Pods, Megalaser).
  * Enemy flight patterns, bomb dropping, and multi-projectile collision detection.
  * Full game state machine: Title -> Mission Select Briefing -> Active Gameplay -> Armory -> Victory / Game Over.

---

## 3. Milestones Achieved

- [x] Initialized Git repository tracking `git@github.com:Chillaxhson/heavy-weapon-deluxe-vita.git`.
- [x] Implemented BYOG (Bring-Your-Own-Game) `.gitignore` protecting commercial assets.
- [x] Generated PS Vita LiveArea assets (`icon0.png`, `bg.png`, `startup.png`, `template.xml`).
- [x] Created complete C++17 engine source code across all subsystems.
- [x] Configured VitaSDK CMake build pipeline with all dependencies (`SDL2`, `SDL2_image`, `SDL2_mixer`, `vitaGL`, `vitashark`, `mathneon`, `libpng`, `libjpeg`, `libwebp`, `tinyxml2`).
- [x] Successfully compiled and packaged `HeavyWeaponDeluxe.vpk` (3.2 MB) in containerized VitaSDK.
- [x] Created `README.md` and `CONTEXT.md` documentation.
