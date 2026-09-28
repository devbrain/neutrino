# Krypton Egg DOS enemies

Implemented normal-stage enemies from the supplied `Krypton-Egg_DOS_EN/ke.exe`
and `ke.rsc`. The executable fingerprint and LE address convention are in
[bonuses.md](bonuses.md). The matching `/home/igor/proj/ke/ke.c` and `ke.asm`
exports were used to trace behavior; animation data was checked directly
against the executable.

## Spawn and movement

`0x2E350` consumes the TAB spawn period and eight-entry sequence. It opens the
hatch when the countdown is 55 (before decrementing), spawns when it reaches
zero, and reloads the period. The sequence advances even if the eight-live-enemy
limit blocks a spawn. Type values above seven clamp to seven. Enemy and hatch
anchors are `(160,16)` and `(144,16)` respectively.

`0x2E8D4` creates an enemy with velocity `(±1,+1)` pixels per original 70 Hz tick,
random turn countdown `(random_low16 >> 7)`, and health `level_index / 20 + 2`.
Only type 2 has the aggressive flag. At countdown expiry, bit `0x20` reverses
x; otherwise bit `0x08` reverses y. Enemy sprite bounds bounce at x=16/304 and
y=24/200. Enemies pass through bricks.

The reimplementation advances enemy movement and animation at 70 Hz within the
engine's fixed update. It uses `std::mt19937`, so individual random paths do
not reproduce the DOS random stream. A zero turn countdown is stored as one;
both cause a turn on the next update. A zero TAB period disables spawning.

## Sprite identities and collision bounds

The table at `0x496B8` supplies these zero-based `KE_NMY.BOB` frames. Names are
descriptive source identifiers, not official enemy names.

| Type | Source name | Frame sequence | Ticks/frame |
| ---: | :--- | :--- | ---: |
| 0 | insectoid | 0,1 | 6 |
| 1 | green_alien | 2,3,4,3 | 15 |
| 2 | ship_demon | 5,5,5,5,6,7,64,65,64,7,6 | 3 |
| 3 | ufo_disc | 8,9,10 | 8 |
| 4 | green_egg | 11,12,13,12 | 15 |
| 5 | red_egg | 14,15,16,15 | 10 |
| 6 | blue_orb | 17,18,19,20,21,22 | 10 |
| 7 | gem_orb | 23,24,25,26,27,28,29,30 | 10 |

The hatch descriptor is `0x49384`: 30 frames at five ticks, opening through
52..63, holding 63, then closing. Enemy explosions use `0x49634`: frames 31..51
at two ticks, played once. Death effects inherit velocity and start at the
enemy bounding-box center plus five pixels vertically.

BOB signed offsets are **added** to the DOS anchor; Neutrino subtracts pivots.
The enemy sprite loader therefore negates BOB offsets when building the sprite
set. Rendering and collision use the same current frame. `0x21308` makes a
box with width/height minus two; `0x21354` insets that box by three pixels.
Ball contacts use the inset enemy box; paddle contacts use the full enemy box
against an inset paddle box. Ball motion remains the port's continuous physics
simulation rather than DOS integer stepping.

## Combat and lives

- The ball path at `0x2BAF0` kills enemies immediately. An ordinary ball reverses
  one random axis, then changes horizontal magnitude by ±8 DOS velocity units,
  clamped to 4..32 units (17.5..140 px/s); horizontal zero becomes -17.5 px/s.
- Blue power balls kill without deflecting. Green ghost balls ignore enemies.
  These behaviors are ready in enemy collision handling; the corresponding
  capsule effects and timers are still unimplemented.
- The one/four-point damage logic at `0x2CB00` belongs to **gun projectiles**.
  The old `ke_dump` notes incorrectly attributed it to balls. Health is stored
  for future weapons support but does not delay a ball kill.
- `0x2E7DC` awards `3 << score_shift` and plays sound 41 (zero-based sample 40).
  Normal-stage enemies do not drop bonuses on death.
- Paddle contact squashes ordinary enemies. A demon consumes one shield point
  and dies, or destroys an unshielded paddle. The shared damage handler also
  implements capsule ID 1; shield ID 15 adds its magnitude. Dynamite ID 27 kills
  all live enemies and awards their points once.

The port now displays score/lives, loses a life on lethal damage or after the
last ball and capsule are gone, preserves the brick grid and score on respawn,
and resets enemies and paddle effects. Paddle death uses the final six-frame
animation from `0x48EEC` (24 ticks). At zero lives, clicking starts a new game.

This covers the eight normal-stage types. Horde rounds, bosses, gun projectiles,
autopilot protection, dynamite's palette flash, automatic score-threshold extra
lives, and the complete DOS death/shrink/birth presentation are separate work.
The game-over panel is a small port UI, not a recreation of the DOS screen.

## Verification

```sh
python3 tools/inspect_enemies.py ~/games/ke/Krypton-Egg_DOS_EN/ke.exe
python3 tools/test_enemies.py
```

The first command checks all eight animation descriptors plus hatch, enemy
death, and paddle death against the fingerprinted executable. The second
compiles the actual game code and `tests/enemies.cc` using an existing Ninja
build's compilation database and libraries, in a temporary directory. It
does not configure CMake or modify the CLion build tree. Use `--build-dir` for
a different configured build and `KE_TEST_RSC` for a different resource path.

Integration checks cover spawn timing/sequence/cap, original sprite bounds,
wall turns, animation loops, ball and paddle contacts, shields, dynamite,
death cleanup, last-ball loss, brick/score preservation, life reset and the
game-over restart click. Rendering is exercised with all eight sprite types;
`KE_TEST_SCREENSHOT=/tmp/ke-enemies.bmp` saves that frame for visual inspection.
