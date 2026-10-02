# Heavy Weapon Deluxe - PS Vita Port

A native **PlayStation Vita** source port of PopCap Games' classic arcade side-scrolling shooter, **Heavy Weapon Deluxe**.

![PS Vita](https://img.shields.io/badge/Platform-PS%20Vita-blue.svg)
![Target Resolution](https://img.shields.io/badge/Resolution-960x544%20(60FPS)-green.svg)
![Controls](https://img.shields.io/badge/Controls-Twin--Stick-orange.svg)

---

## Features

* **Native Vita Widescreen (960×544)**: The original PC game's 960-wide parallax background textures match the PS Vita's screen width pixel-for-pixel with locked 60 FPS.
* **Modern Twin-Stick Controls**:
  * **Left Analog Stick / D-Pad**: Move Atomic Tank left and right.
  * **Right Analog Stick**: 360° fluid turret aiming and direction-guided auto-fire.
  * **R1 / Cross (✕)**: Fire Heavy Cannon.
  * **L1 / Triangle (△)**: Nuclear Bomb (Nuke).
  * **Square (□)**: Megalaser screen-clearing blast.
  * **Touchscreen**: Tap to navigate menus, select missions, and purchase Armory upgrades.
* **Full Campaign & Survival**:
  * All 19 missions with historical intel briefings loaded from `data/levels.xml`.
  * Complete wave scheduling loaded from `data/waves.xml`.
  * All 20+ enemy aircraft, ground vehicles, and multi-part bosses loaded from `data/craft.xml` and `data/bosses.xml`.
  * 4-plane parallax scrolling and ambient animations from `Images/Anims/Anims.xml`.
  * Complete Armory upgrade tree (Heavy Cannon, Defense Pods, Homing Missiles, Laser, Flak, Thunderstrike, Nukes).
* **Hardware Accelerated 2D Rendering**: Powered by **VitaGL** and **SDL2**, featuring PopCap dual-file alpha mask merging (`.jpg` + `_.png`), additive blend modes, and particle physics.
* **Audio**: High-fidelity sound effects and music via `SDL2_mixer`.

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

The project includes containerized build support using the official `vitasdk/vitasdk` Docker/Podman image:

```bash
# Clone the repository
git clone git@github.com:Chillaxhson/heavy-weapon-deluxe-vita.git
cd heavy-weapon-deluxe-vita

# Build with Podman / Docker
podman run --rm -v "$(pwd):/src:Z" -w /src docker.io/vitasdk/vitasdk:latest bash -c "
    cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/usr/local/vitasdk/share/vita.toolchain.cmake
    cmake --build build
"
```

The compiled package will be located at `build/HeavyWeaponDeluxe.vpk`.

---

## Controls Reference

| Input | Action |
| :--- | :--- |
| **Left Stick / D-Pad** | Drive tank Left / Right |
| **Right Stick** | 360° Turret Aiming (Hold to Auto-Fire) |
| **Cross (✕) / R1** | Fire Heavy Cannon |
| **Triangle (△) / L1** | Detonate Nuclear Bomb (Nuke) |
| **Square (□)** | Fire Megalaser (when meter is charged) |
| **Start** | Pause / Resume Game |
| **Circle (○)** | Cancel / Retire to Title |
| **Touchscreen** | Armory Upgrades, Mission Select, and Menus |

---

## Legal & Disclaimer

* *Heavy Weapon Deluxe* is © 2005 PopCap Games, Inc. / Electronic Arts.
* This repository does **not** contain copyrighted assets, music, or binaries from the original game.
* This is a fan-created homebrew port made for educational, preservation, and personal use.
