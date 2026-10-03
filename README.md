# Heavy Weapon Deluxe - PS Vita Port

A native **PlayStation Vita** source port of PopCap Games' classic arcade side-scrolling shooter, **Heavy Weapon Deluxe**.

![PS Vita](https://img.shields.io/badge/Platform-PS%20Vita-blue.svg)
![Target Resolution](https://img.shields.io/badge/Resolution-640x480%20scaled%20to%20960x544-green.svg)
![Controls](https://img.shields.io/badge/Controls-Twin--Stick-orange.svg)

---

## Features

* **Original 640×480 presentation**: The game renders at its native 640×480 (every asset is authored for it) into an offscreen target that is scaled to the Vita display, pillarboxed at 4:3 (725×544) by default, or stretched to fill.
* **Modern Twin-Stick Controls**:
  * **Left Analog Stick / D-Pad**: Move Atomic Tank left and right.
  * **Right Analog Stick**: 360° fluid turret aiming and direction-guided auto-fire.
  * **R1 / Cross (✕)**: Fire Heavy Cannon.
  * **L1 / Triangle (△)**: Nuclear Bomb (Nuke).
  * **Touchscreen**: Tap to navigate menus, select missions, and purchase Armory upgrades.
* **Survival mode**: endless waves from `data/survival0-9.xml` (one set picked per run, 20 tiers that step up every minute), scored by time survived. You get a single tank (one life), and supply drops (crates, power-up helicopters, nukes, shields) come twice as often as in the campaign for a better experience. The best time is saved in `settings.ini`.
* **Campaign**:
  * All 19 missions loaded from `data/levels.xml`, with the original bosses.
  * Complete wave scheduling loaded from `data/waves.xml`.
  * All 20+ enemy aircraft, ground vehicles, and multi-part bosses loaded from `data/craft.xml` and `data/bosses.xml`.
  * 4-plane parallax scrolling. The purely decorative background animations (igloos, snowmen, ...) are off by default (`kDecorativeAnims` in `Constants.h`).
  * Complete Armory upgrade tree (Heavy Cannon, Defense Pods, Homing Missiles, Laser, Flak, Thunderstrike, Nukes).
* **Hardware Accelerated 2D Rendering**: Powered by **VitaGL** and **SDL2**, featuring PopCap dual-file alpha mask merging (`.jpg` + `_.png`), additive blend modes, and particle physics.
* **Audio**: Sound effects and menu music via `SDL2_mixer`, with a music on/off option (Options on the main menu). `AtomicTank.mo3` cannot be decoded yet, so in-mission music is silent.
* **Loading**: all sprites and sounds are loaded once at start-up (about 8-9 s on a Vita, with a progress bar) so missions run without hitches.

---

## Installation & Setup

This port follows the **Bring-Your-Own-Game (BYOG)** standard. You must provide the data files from your legally purchased copy of *Heavy Weapon Deluxe* (e.g. from Steam).

1. Install `HeavyWeaponDeluxe.vpk` on your PS Vita using VitaShell.
2. Obtain your legal copy of *Heavy Weapon Deluxe* on PC.
3. Copy the game's asset directories onto your PS Vita into `ux0:data/heavyweapon/`:
   ```
   ux0:data/heavyweapon/
   ├── data/
   │   ├── bosses.xml
   │   ├── craft.xml
   │   ├── credits.xml
   │   ├── levels.xml
   │   ├── survival0.xml .. survival9.xml
   │   └── waves.xml
   ├── Fonts/
   ├── Images/
   ├── Music/
   │   ├── AtomicTank.mo3
   │   └── LoveTheme.ogg
   └── Sounds/
   ```
4. Launch **Heavy Weapon Deluxe** from the LiveArea!

---

## Building from Source

Both builds run in containers, so the host only needs Podman. The helper script wraps them:

```bash
git clone git@github.com:Chillaxhson/heavy-weapon-deluxe-vita.git
cd heavy-weapon-deluxe-vita

tools/desktop/hw.sh vpk      # PS Vita package -> build/HeavyWeaponDeluxe.vpk
```

### Desktop build (for development)

The same code also builds for Linux (SDL2 + OpenGL), so changes can be checked without
reinstalling a VPK. Copy your own game files to `Heavy Weapon Deluxe/` at the repo root first.

```bash
tools/desktop/hw.sh image    # once: build the dev container
tools/desktop/hw.sh build    # compile into build-desktop/
tools/desktop/hw.sh run      # play in a window (keyboard + mouse, sound)
tools/desktop/hw.sh shot shots/play.png --state play --level 2 --frames 300   # headless screenshot
```

Game options: `--state title|map|play|armory|options|help`, `--level N` (0-based mission),
`--frames N` (quit after N fixed-timestep frames), `--stretch`, `--scale N` (window size).
Desktop controls: A/D or arrows to drive, mouse to aim, left click or Ctrl to fire,
X or right click for a nuke, Tab for music on/off, Enter to confirm, Esc to pause/back.

---

## Controls Reference

| Input | Action |
| :--- | :--- |
| **Left Stick / D-Pad** | Drive tank Left / Right |
| **Right Stick** | 360° Turret Aiming (Hold to Auto-Fire) |
| **Cross (✕) / R1** | Fire Heavy Cannon |
| **Triangle (△) / L1** | Detonate Nuclear Bomb (Nuke) |
| **Start** | Pause / Resume Game |
| **Select** | Music on / off |
| **Circle (○)** | Cancel / Retire to Title |
| **Touchscreen** | Menus (aim / drive / fire by touch when no stick is in use) |

---

## Legal & Disclaimer

* *Heavy Weapon Deluxe* is © 2005 PopCap Games, Inc. / Electronic Arts.
* This repository does **not** contain copyrighted assets, music, or binaries from the original game.
* This is a fan-created homebrew port made for educational, preservation, and personal use.
