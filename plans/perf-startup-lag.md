# Plan: mission-start lag, spikes, decorations, music toggle

Author: Opus (investigation only, nothing here is implemented or measured on a Vita yet).
Executor: Sonnet. Call `opus-reviewer` if a step contradicts what is written here, or after step 1
if the numbers point somewhere unexpected.

## Reported
1. Every mode start: tank ignores input for ~20 s, but fps is fine.
2. Occasional lag spikes, mostly right after a mission starts.
3. Decorative anims (igloos, snowmen...) could go. 60 entries in Images/Anims/Anims.xml.
4. Want a BG music on/off option. SFX are correct, leave them.

## What the code shows
- `GameEngine::StartLevel` (src/core/GameEngine.cpp:225) loads the 5 theme planes and every anim
  strip for the level synchronously, then `PlayMusic("Music/AtomicTank.mo3")`. CONTEXT.md says MO3
  cannot be decoded, so that load fails every time and plays nothing: pure wasted time.
- Everything else is loaded lazily on first use: `Img("name")` -> `TextureManager::Get` -> `Load`
  (src/render/TextureManager.cpp:~335). Each craft, bullet, explosion and boss part hitches the
  frame it first appears. This is the mission-start spike (first waves = many new sprites).
- A texture load is expensive on the Vita because of the VFS, not just decoding.
  `Vfs::Resolve` (src/platform/Vfs.cpp) tries `access()` and, if the exact case misses, does
  `opendir/readdir` on EVERY path component for EVERY search root (ux0:, app0:, ...). `FindImageFiles`
  calls `Vfs::Exists` up to ~10 times per texture (3 colour exts + 6 mask candidates), each a full
  Resolve, then `Vfs::Resolve` once more. A missing candidate is the worst case: it scans every
  directory level in every root. Misses are the common case here.
- `PreloadSound` has no negative cache: a sound whose file is missing re-runs `Vfs::Resolve` (slow
  path) every time it plays.
- Input: the tank cannot act while `Board::mRespawn > 0`. It starts at 300 ticks = 3 s at 100 Hz
  (include/game/Board.h:192, GET READY). That is by design (matches the original) and cannot explain
  20 s by itself. Candidates: the sim is running much slower than 100 Hz on the Vita (the tick loop
  `GameEngine::UpdatePlaying` caps catch-up at 10 ticks/frame, so slow ticks stretch the 3 s
  countdown without lowering rendered fps), or the first frames stall in the loading above.
  I could not tell which from the code. STEP 1 EXISTS TO ANSWER THIS. Do not guess a fix for the
  20 s before it.

## Step 1: instrument (do first, ship it, get a Vita log)
- Add a tiny profiler `src/core/Perf.{h,cpp}`: `PERF_SCOPE("name")` accumulating ms with
  `SDL_GetPerformanceCounter`; no-op unless built with `-DHW_PERF=ON` (add CMake option).
- Log, to stdout and to `ux0:data/heavyweapon/perf.log` on Vita (plain `fopen`, flush per line):
  - time spent in `StartLevel` (split: SetTheme, Board ctor, PlayMusic).
  - every `TextureManager::Load` miss with its ms (name + ms), and every `Vfs::Resolve` > 5 ms.
  - once per second during play: ticks executed, ticks/sec, worst frame ms, `mRespawn`, `mApp.tick`.
    ticks/sec should read ~100. If it reads far below, the 20 s is a slow simulation (cost per
    `Board::Update`): profile `UpdateF`, collision (`PixelHit`) and the spawner next.
  - the worst `Board::Update` ms in that second.
- Build the VPK with HW_PERF=ON, user plays one mission, send back perf.log. Desktop check:
  `tools/desktop/hw.sh shot` still passes with HW_PERF off AND on.

## Step 2: fixes that are safe regardless of the log
a. VFS cache (biggest likely win). In `Vfs::Resolve`: memoize `relative path -> resolved path`
   (and misses -> "") in an `unordered_map`, keyed on the lowercased normalized path.
   Additionally cache each directory's listing once (`dir path -> lowercase name -> real name`) so the
   case-insensitive fallback never calls `readdir` twice for the same directory. Invalidate nothing
   (assets are read-only). `Vfs::Exists` then costs a hash lookup.
b. `FindImageFiles`: keep as is after (a); the ~10 lookups become cheap.
c. Negative cache in `AudioSystem::PreloadSound` for missing sound files.
d. Drop the MO3 attempt: in `PlayMusic`, return immediately for `.mo3` (log once). Do not try to
   decode it. (Converting AtomicTank.mo3 to OGG is a separate later task needing the user's OK; the
   in-game music today is effectively LoveTheme only in menus.)
e. Preload at level start instead of mid-fight. Add `Board::PreloadAssets()` called from
   `StartLevel` before the GET READY countdown: load the textures every level needs (tank, gun,
   shells, explosion, particles, crates, HUD, bullets) plus the craft sprites named by this
   level's `waves.xml` entries (names come from `CraftDef`). Keep the list data-driven, not
   hand-maintained per craft. Preload the boss sprites for the level's boss in the background of
   the countdown, or at least when `mProgress == mLength - 1500`, not at spawn.
   The GET READY countdown (3 s of no control) already exists, so loading under it is invisible.
f. Preload sounds used by Board at start (explosions, tankfire*, diesel, voice lines): loop over
   `sSoundMetaTable` for the ones Board plays, or simply all 92 once at boot behind the existing
   splash. Measure with Step 1 before choosing; Mix_LoadWAV of OGG decodes fully into RAM.
Pass criterion: perf.log shows no `Load` > 20 ms after GET READY ends, and `StartLevel` total drops.

## Step 3: remove decorative anims
- In `WorldRenderer::SetTheme` skip anims that have no gameplay meaning. Gameplay-relevant ones are
  `def.nuke == true` ones only if nuking them matters to scoring (check `TriggerNuke` users; I
  believe it is purely visual). Make it a single constant/flag `kDecorativeAnims = false` in
  Constants.h so the user can turn them back on. When off, `SetTheme` must not load their textures
  (this is also a load-time saving) and `Tick/RenderAnims` just see an empty list.
- Keep the parallax planes (sky, bg2, bg, ground) and `milemarker`/`mappointer`. Those are not
  in Anims.xml.
- Check with a desktop shot of mission 0 (Frigistan, snow) before/after; the igloos and snowmen
  should be gone and nothing else should move.

## Step 4: background music toggle
- There is no options screen. Do the smallest thing that works on Vita: add a `musicEnabled` bool to
  the app/profile state, toggled from the PAUSE screen (`STATE_PAUSED`, GameEngine.cpp) with a
  visible line "MUSIC: ON/OFF" and a button (use SELECT; also a touch hit-box). Also toggleable from
  the title screen with the same button.
- Persist it in `ux0:data/heavyweapon/settings.ini` (`music=0|1`), written on change, read in
  `GameEngine` init. Missing/corrupt file -> default ON. Desktop: `./settings.ini`.
- When OFF: `PlayMusic` is a no-op (do not load the file at all), existing music is stopped.
  When toggled back ON mid-mission, start the correct track for the current state.
- SFX untouched.

## Order and commits
Step 1 (own commit) -> user runs it on the Vita and sends perf.log -> Steps 2a-2d together ->
2e/2f -> 3 -> 4. One commit per step. Do NOT push; the user pushes.
After step 1, stop and report the numbers; I will decide whether the 20 s needs a deeper change
(e.g. a Board::Update cost fix) before you continue past 2.

## Verification
- Desktop: `tools/desktop/hw.sh build`, then `shot` on levels 0, 5 and 9 (several themes + a boss),
  compare to `shots/` for visual regressions. Check the log has no new "Failed to load".
- Vita: VPK via `tools/desktop/hw.sh vpk`. Success = tank responds right after GET READY
  (about 3 s), no hitch when new enemy types appear in the first minute, music toggle works and
  survives restart.

## Risks
- VFS cache keyed on lowercase path assumes no two files differing only by case. True for this
  asset set; assert on collision in HW_PERF builds.
- Preloading all craft sprites raises GPU/RAM use; textures keep a CPU copy for per-pixel
  collision. Vita has 512 MB total, so check Step 1 memory log (`SDL_GetPerformanceCounter` is not
  enough; log the texture count and the sum of w*h*4) before preloading everything.
