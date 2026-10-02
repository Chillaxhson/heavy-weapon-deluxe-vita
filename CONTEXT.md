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
     * `data/craft.xml`: 21 enemy aircraft, bombers, helicopters, trucks, missiles, and stats (armor, points, arms).
     * `data/bosses.xml`: Full multi-part boss parameters (Twinblade, Battleship, War Blimp, etc.).
     * `data/waves.xml`: Exact wave progression tables for all 19 missions.
     * `data/survival0.xml`–`survival9.xml`: Survival mode wave definitions.
   * `Images/`:
     * The game is **640×480**. Sky images, menus, the map and the armory are full 640×480 frames; the ground strip is 640×60. The 300px tall `*_bg` / `*_bg2` planes are 960 wide only so they can wrap while scrolling (this has nothing to do with the Vita's 960px width).
     * Dual-file alpha masking: colour in `name.ext` and alpha in `name_.ext` (or `_name.ext`), with any mix of `.png` / `.jpg` / `.gif` (e.g. `Anims/Frigistan/Yetti.gif` + `Yetti_.gif`).
     * `Anims/Anims.xml`: 4-layer parallax plane specifications (Sky, Far BG, Mid BG, Ground) with frame counts, offsets, and speeds.
   * `Sounds/`: High-quality OGG Vorbis sound effects with exact default volumes matching PopCap's audio table.
   * `Music/`: `AtomicTank.mo3` (MO3 module) and `LoveTheme.ogg`.

2. **Reverse Engineering PopCap's Engine**:
   * Extracted inner game executable (`inner_game.exe`, 1.5 MB) from `Heavy Weapon Deluxe.exe` at offset `0x591a2` (below the DRM wrapper).
   * The extracted binary lives at `re/inner_game.exe` (git-ignored, image base `0x400000`; `.text` at file offset `0x1000`, `.rdata` at `0x127000`). File offsets equal RVAs.
  * Disassembled core classes using Capstone:
     * **Image Resource Table** (`0x127820`): 52-byte structs storing image names, column counts, and row counts (e.g. `gun` cols=21, rows=5; `bullets` cols=21, rows=5; `tank` cols=10, rows=1; `explosion` cols=20, rows=1).
     * **Sound Table** (`0x129808`): 24-byte structs with sound names and IEEE-754 double precision default volume levels (e.g. `bigexplode` 1.0, `tankfire3` 0.8, `diesel` 0.4).
     * **Tank Rendering Pipeline** (`0x401b80`): Multi-layer compositing drawing `tankshadow` at $(x, y+22)$, `tank` at $(x, y)$, `gun` at $(x-40, y-52)$ using 21 discrete angle columns computed as $(180^\circ - \theta)/9.0^\circ$, secondary weapon attachments (`lasers.png`, `flakguns.jpg`, `rocketpod.png`), and `orb.png` orbiting at elliptical radii $(r_x=50, r_y=40)$.

* **Logic rate**: `Anims.xml` documents speeds "per program cycle, 1 = 100fps", so PopCap's logic ticks at 100 Hz.

---

## 2. Rendering, Layout & Desktop Build (current)

* **Logical resolution**: Everything renders at 640×480 (`SCREEN_WIDTH/HEIGHT`) into an offscreen framebuffer, which `Renderer::EndFrame` scales to the display (`DISPLAY_WIDTH/HEIGHT` = 960×544 on Vita): 4:3 pillarbox (725×544) by default, or `SCALE_STRETCH`. Touch and mouse input are mapped back to 640×480 via `Renderer::DisplayToLogical` / `WindowToLogical`.
* **Playfield layout** (`Constants.h`, provisional until confirmed in the decompiled draw code): status bar at the top (y=0), ground strip at y=420–480, background planes bottom-aligned to the screen (y=180). That placement makes every theme's plane art end just below the ground's grass edge and puts the `Anims.xml` props (absolute y) on the terrain.
* **Menus** draw the baked-in art at 0,0. Main-menu hover = the part of `mainglow` around the selected button plus `mainbigbtn` / `mainsmallbtn` (additive rim glows). Button centres and armory socket positions were measured from the art.
* **Desktop build**: `tools/desktop/hw.sh` (Podman). `build` / `run` / `shot OUT.png --state play --level N --frames N` give a playable Linux window and headless screenshots, so changes can be checked without a Vita. `vpk` builds the Vita package.
* **Known gaps**:
  * Gameplay numbers in `Entities.cpp` / `GameEngine.cpp` (speeds, fire rates, spreads, enemy behaviour, spawning, hitboxes) are guesses and do not come from the executable. Next step: decompile them.
  * HUD slot contents, war-room overlays (`missionN.png`, `maprect`, `mappointer`) and plane scroll speeds are placeholders.
  * `milemarker.png` is the in-world "ENEMY HQ" sign, not a HUD slider.
  * `AtomicTank.mo3` does not play on either platform: libxmp cannot decode MO3. Needs libopenmpt or a one-time conversion to OGG.
  * `copterblades.png` is unused by the original; helicopters use `rotors` (7 frames).

---

## 3. Architecture & Subsystems Implemented

* **Platform / Virtual File System (`Vfs`)**:
  * Case-insensitive POSIX file resolver (every path component, since PopCap data uses arbitrary casing such as `frigistan\yetti`).
  * Checks `ux0:data/heavyweapon/`, `app0:`, `./Heavy Weapon Deluxe/`, and `./`.
* **Renderer & Texture Pipeline (`Renderer`, `TextureManager`)**:
  * Hardware-accelerated 2D quad/sprite renderer via **VitaGL** and **SDL2**.
  * Dual-file PopCap texture loader merging `.jpg` color channels with `_.png` alpha masks into 32-bit RGBA GL textures.
  * Added `DrawCel()` utility for column/row grid sprite sheets with scaling, rotation, and additive blending.
  * Additive blending for laser beams, explosions, and shield effects.
  * Screen shake physics for detonations and damage.
* **Typography (`FontRenderer`)**:
  * Parser for PopCap `ImageFont` metrics (`.txt`) and glyph sheets (`.png`).
  * Supports alignment (Left, Center, Right), scaling, and kerning.
* **Audio Engine (`AudioSystem`)**:
  * Powered by `SDL2_mixer` for low-latency sound effects across 32 channels.
  * PopCap volume balance table applied automatically to all sounds.
  * Looping `diesel.ogg` engine sound channel while the tank travels.
  * PopCap announcer voice lines (`v_atomictank`, `v_getready`, `v_levelcomplete`, `v_gameover`, `v_megalaser`).
* **Controls & Input (`InputManager`)**:
  * Modern twin-stick controls: Left stick drives tank, Right stick aims with auto-fire threshold.
  * D-pad button edge triggers for menu and Armory navigation.
  * Hardware buttons: Cross/R1 (Cannon), Triangle/L1 (Nuke), Square (Megalaser), Start (Pause).
  * Direct touchscreen support for aiming and menu selections.
* **World Simulation & Parallax (`WorldRenderer`)**:
  * 4-plane horizontal parallax scrolling with wrapping and ambient animations from `Anims.xml`.
* **Entities & Game Logic (`PlayerTank`, `Enemy`, `Projectile`, `GameEngine`)**:
  * Authentic multi-layer Atomic Tank: rocking chassis, 21-angle turret gun, recoil, muzzle blast (`muzzleflash.png`), exhaust flame (`tankflame.png`), and brass shell casings (`casing.png`) with physics.
  * Authentic projectiles: `bullets.png` (21 angles $\times$ 5 rows), `dumbbomb.png`, `fragbomb.png`, `ironbomb.png`, `fatboy.png`, `lgb.png`, `missile.png`.
  * Authentic explosions: 20-frame high-resolution explosion sheet (`explosion.jpg` + `explosion_.png`), ground craters (`crater.png`), and nuke mushroom clouds (`mushsmoke.png`, `mushfire.jpg`).
  * All 21 craft types from `craft.xml` with animated helicopter rotors (`rotors.png`, `medrotor.jpg`, `puprotor.png`), vehicle shadows, and damage hit-flashes.
  * In-game metallic status bar (`statusbar.png`) along the top of the screen.
  * War Room tactical map (`map.jpg`, `*_map.png`, `mappointer.png`) and Armory station (`armory.jpg`, `upgrades.png`, `upgradelvl.jpg`, `upgradebtns.png`, `advancebtn.jpg`).

---

## 4. Milestones Achieved

- [x] Initialized Git repository tracking `git@github.com:Chillaxhson/heavy-weapon-deluxe-vita.git`.
- [x] Implemented BYOG (Bring-Your-Own-Game) `.gitignore` protecting commercial assets.
- [x] Generated PS Vita LiveArea assets (`icon0.png`, `bg.png`, `startup.png`, `template.xml`).
- [x] Created complete C++17 engine source code across all subsystems.
- [x] Configured VitaSDK CMake build pipeline with all dependencies (`SDL2`, `SDL2_image`, `SDL2_mixer`, `vitaGL`, `vitashark`, `mathneon`, `libpng`, `libjpeg`, `libwebp`, `tinyxml2`).
- [x] Discovered and resolved PopCap's XML designer typo in `Anims.xml` (`nuke="yes"/ rare="yes">`) via automatic in-memory XML preparation.
- [x] Added automated unit test suite (`tests/test_xml_loader.cpp`) verifying 21 craft types, 19 missions, 84 waves, 10 bosses, and 60 ambient animations with 100% pass rate.
- [x] Resolved VitaShell error `0x8010113D` by converting all LiveArea PNGs (`icon0.png`, `bg.png`, `startup.png`) to 8-bit indexed colormap format as strictly required by Sony SceAppMgr.
- [x] Reverse-engineered PopCap's executable to uncover exact sprite tables, frame layouts, sound volumes, and tank/projectile rendering logic.
- [x] Overhauled engine to 100% authentic PopCap visuals (multi-layer tank, 21-angle turret gun, 20-frame explosions, helicopter rotors, metallic status bar, War Room, and Armory).
- [x] Successfully recompiled and packaged `HeavyWeaponDeluxe.vpk` (2.9 MB).
- [x] Synchronized all updates with GitHub repository.
- [x] Switched to native 640×480 rendering with display scaling; fixed the background plane layout, case-insensitive asset lookup (background animations now load), cached failed texture loads, PopCap font punctuation, and 100 Hz animation timing.
- [x] Added a Linux desktop build with headless screenshots (`tools/desktop/hw.sh`).
- [x] Replaced the hand-written sprite table with all 157 records from the executable.
