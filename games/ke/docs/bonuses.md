# Krypton Egg DOS bonus mapping

Verified against the user's `Krypton-Egg_DOS_EN/ke.exe` and `ke.rsc` on
2026-09-28. Executable SHA-256:

```
5ee2470f159f7705d26469dcba4b0f153c5724c072acc7133af67a536c651849
```

The existing `/home/igor/proj/ke/ke.exe` has the same hash. Its `ke.c` and
`ke.asm` exports helped navigate the code; the dispatch pointers, animations,
and key decoder instructions were checked against the supplied executable.
Addresses below are LE virtual addresses: code object base `0x10000`, data
object base `0x40000`. File offsets are different. Effect names describe traced
behavior; they are not claims about official English names.

## The mapping bug

The old `ke_dump/docs/tab.md` omitted a subtraction in `destroy_brick`.
Consequently the reimplementation selected the next sprite/effect for every
valid TAB bonus, and incorrectly wrapped high codes through `& 31`.

The original path is:

1. `0x32840`: only destroyed group-B bricks (`0x31..0x60`) can drop a bonus.
2. If `attribute & 0xFC` is nonzero, pass **`(attribute >> 2) - 1`** and
   `attribute & 3` to `0x2D700`.
3. `0x2D700`: reject runtime IDs **>= 28**, and reject new capsules when 24
   already exist. Only then mask the ID with `0x1F` and store it.
4. `0x2D470`: on catch, set `mag = stored_low_bits + 1` and dispatch through
   `0x49304`. The matching animation pointer is at `0x49284`.

The decisive instructions are:

```asm
; destroy_brick: virtual 0x329BA (file 0x31DBA)
mov   al, [ebx]       ; attribute
shr   al, 2
movzx eax, al
dec   eax            ; 0x329C2: the missing subtraction
push  eax
call  0x2D700

; spawn_bonus: virtual 0x2D70D (file 0x2CB0D)
movzx ebx, byte ptr [esp+8]
cmp   ebx, 0x1C
jge   0x2D778        ; return before the later AND 0x1F
```

Thus `0x00..0x03` and `0x74..0xFF` never produce a capsule. `0x04..0x07`
means enlarge, not damage. `0x80` does not wrap to enlarge. `bonus::none` is
`0xFF`, outside the valid runtime IDs.

## Complete effect table

`attr` includes all four magnitude encodings; `frame` is the first **zero-based**
`KE_SPELL.BOB` block. Full sequences and timings remain in
`assets/sprites.hh` and are printed by the extraction tool. `mag` is 1..4.

| ID | TAB attr | Frame | Handler | Effect established from code |
|---:|:---------|------:|:--------|:-----------------------------|
| 0 | 04–07 | 62 | 2DE00 | Enlarge paddle by `mag` logical sizes, clamp original size to 0..12. |
| 1 | 08–0B | 67 | 2D7E8 | Damage paddle: consume one shield point; a negative counter triggers death. Magnitude is ignored. Autopilot protects an unshielded paddle. |
| 2 | 0C–0F | 53 | 2D838 | Increase score shift by `mag`: multiply subsequent awards by `2^mag`. |
| 3 | 10–13 | 50 | 2D860 | Reverse horizontal controls for `64*mag` ticks; another pickup extends the timer. Ignored during autopilot. |
| 4 | 14–17 | 76 | 2D908 | Shrink every ball by `mag` size steps, clamp to 0..5. Does not change speed. |
| 5 | 18–1B | 70 | 2D938 | Glue/slime paddle: attach an animation, then enable ball catching when it finishes. |
| 6 | 1C–1F | 37 | 2D96C | Add `mag` lives. |
| 7 | 20–23 | 48 | 2D994 | Launch one new smallest ordinary ball from the paddle, up to 25 balls. No splitting; magnitude is ignored. |
| 8 | 24–27 | 57 | 2D910 | Enlarge every ball by `mag` size steps, clamp to 0..5. |
| 9 | 28–2B | 34 | 2DA4C | Darkness: darken the palette and extend the timer by `128*mag` ticks; fade back near expiry. |
| 10 | 2C–2F | 69 | 2DAF8 | Speed up every ball's velocity components via `0x2C5BC`. |
| 11 | 30–33 | 68 | 2DAF0 | Slow every ball's velocity components through the same helper with negative magnitude. |
| 12 | 34–37 | 56 | 2DB08 | JAO/autopilot for `1024*mag` ticks; clear freeze/reverse, follow balls/useful capsules, auto-launch and auto-fire. |
| 13 | 38–3B | 54 | 2DBA0 | Flying paddle: enable vertical mouse movement over y=24..188. |
| 14 | 3C–3F | 41 | 2DBDC | Freeze paddle: suppress manual movement for `32*mag` ticks. Ignored during autopilot. |
| 15 | 40–43 | 35 | 2DC14 | Add `mag` shield/resistance points and initialize the shield animation if previously unshielded. |
| 16 | 44–47 | 46 | 2DC54 | Cannon: projectile type 2, one shot in flight, 15-tick reload. |
| 17 | 48–4B | 38 | 2DC94 | Power balls: blue sprites, damage bricks without reflecting, can destroy normally indestructible bricks. |
| 18 | 4C–4F | 40 | 2DCB4 | Ghost balls: green sprites, pass through bricks and enemies **without damaging them**; timer extended by `512*mag` ticks. |
| 19 | 50–53 | 49 | 2DD34 | Extra stationary spectre paddle at the pickup's paddle x; it can bounce balls. |
| 20 | 54–57 | 47 | 2DD60 | Rapid cannon: same as 16 but up to five shots in flight. |
| 21 | 58–5B | 36 | 2DDA0 | Random: dispatch another handler, preserving the pickup's magnitude. See exact selection below. |
| 22 | 5C–5F | 71 | 2DDF8 | Shrink paddle by `mag` logical sizes. |
| 23 | 60–63 | 42 | 2DE20 | Single gun: projectile type 0, one shot in flight, 4-tick reload. |
| 24 | 64–67 | 44 | 2DE60 | Double gun: projectile type 1, one shot in flight, 6-tick reload. |
| 25 | 68–6B | 43 | 2DEA0 | Rapid single gun: type 0, up to five shots in flight, 4-tick reload. |
| 26 | 6C–6F | 45 | 2DEE0 | Rapid double gun: type 1, up to three shots in flight, 6-tick reload. |
| 27 | 70–73 | 81 | 2DF20 | Dynamite: mark every active enemy for death and flash the palette. |

IDs 28..31 have question-mark animation pointers and a shared return-only
handler in the executable, but the factory rejects them. They are not valid
TAB bonuses.

## Evidence for previously guessed effects

`P` is the paddle state pointed to by `0x473F0`; `F` is the flag word pointed
to by `0x473F4`. These are byte offsets, not the decompiler's `int16_t` indices.

- **Reverse, freeze, darkness, autopilot:** the pickup handlers set `F & 0x10`,
  `0x100`, `0x800`, and `0x8`, respectively. The paddle update at `0x2AE30`
  consumes the corresponding timers at `P+0x20`, `+0x22`, `+0x1C`, `+0x1E`.
  Reverse computes `left_limit + right_limit - mouse_x`; freeze skips input;
  darkness calls the palette routine `0x210F8`; autopilot follows balls/capsules.
- **Glue:** `0x2D938` creates effect `0x490D0` with flags `0x0D`. The effect
  factory `0x2E240` converts these to follow-paddle, set-glue-on-completion,
  and remove-on-completion flags. `0x2DFC0` then sets `F & 0x40`. The ball
  update at `0x2BAF0` attaches balls on paddle impact and releases one per click.
- **Flight:** `0x2DBA0` sets `F & 0x400`. `0x2BA3C` changes the minimum paddle
  y from 188 to 24 and updates the input limits; it does not relaunch balls.
- **Shield/damage:** `0x2DC14` adds to `P+0x18`; `0x2D7E8` subtracts exactly
  one and invokes paddle death at `0x2B8CC` below zero. This is also the
  aggressive enemy's paddle-contact routine, not a gun handler.
- **Power/ghost:** `0x2DC94` sets `F & 0x1000`. `0x2BAF0` selects the blue
  ball table at `0x49034` and bypasses brick-bounce resolution; `0x2C6A0`
  permits destruction of normally indestructible bricks. Multi-hit coloured
  bricks still go through the normal durability downgrade. `0x2DCB4` instead
  sets `F & 0x2000` and bit 2 on existing balls, selects the green table at
  `0x49060`, and bypasses the brick/enemy collision section entirely. When
  the timer ends, balls return to normal on re-entering the lower paddle band.
- **Extra paddle:** `0x2DD34` stores current x at `P+0x3E` and sets
  `F & 0x200`. `0x2B5B0` draws `KE_RACK` block 48 there; `0x2BAF0` has a
  second paddle collision test with the spectre bounce sound. There is no warp.
- **Weapons:** the six handlers set `F & 0x80` and the fields at
  `P+0x34..0x3C`: current reload, reload period, maximum projectiles, projectile
  type, vertical speed. `0x2CB00` consumes these on the fire input and calls
  `0x2D38C`. They are not six unrelated paddle forms.

| Runtime IDs | Projectile type | Reload ticks | In-flight limit | Vertical px/tick | KE_SPELL projectile block |
|:------------|----------------:|-------------:|:----------------|------------------:|--------------------------:|
| 16 / 20 | 2 (cannon) | 15 | 1 / 5 | -2 | 33 |
| 23 / 25 | 0 (single) | 4 | 1 / 5 | -5 | 31 |
| 24 / 26 | 1 (double) | 6 | 1 / 3 | -4 | 32 |

Projectile type 2 can destroy normally indestructible bricks; type 1 deals
four enemy hit points, type 0 one. The in-flight limit counts projectile
records: a double-shot sprite is one record. The 15/4/6 values previously
called “frame counts” are reload timers.

The random handler selects `r = (random_state >> 2) & 31`, subtracts 28 for
28..31, replaces 1 with 15 (damage → shield), and replaces 21 with 19
(random → extra paddle). It is not a uniform choice among the remaining
26 effects, and does **not** ignore magnitude: the selected handler receives it.

## Size, speed, score, and sound details

- Ordinary paddle sizes are 0..12, mapped by `0x48D9C` to `KE_RACK` blocks
  **8..20**. Initial size is 3 (block 11), set at `0x22D38`.
  The C++ API retains 1-based sizes 1..13, with default 4. Armed/frozen tables
  `0x48DF0`/`0x48E44` repeat each frame twice except the final one; glue is
  an overlay from `0x48E98`. Original resizing advances one size every 3 ticks.
- Ball size is byte `ball+9`, clamped to 0..5. The three ball tables are
  ordinary `0x49008` (blocks 0..5), power `0x49034` (6..11), and ghost
  `0x49060` (12..17). The previous names “sticky”/“super” were misleading.
- New balls (`0x2C560`) start with size 0 and velocity `(10,-20)` in
  1/16-pixel-per-tick units. At the reimplementation's 70 Hz conversion,
  that is `(43.75,-87.5)` px/s. ID 7 adds one ball even when `mag` is 4.
- Speed adjustment (`0x2C5BC`) adds/subtracts `4*mag` to component magnitudes.
  Horizontal magnitude is clamped to 4..32 (zero remains zero); vertical
  magnitude to 4..40. A zero vertical component takes the negative branch.
- Score shift lives at `P+0x16`. Brick destruction awards `1 << shift`;
  pickup awards `2 << shift` **after** invoking the effect, so score bonuses
  immediately affect their own pickup award. The C++ port saturates at the
  maximum `long` instead of reproducing 32-bit overflow.
- Calls to audio at `0x10EB0` use **one-based** sound IDs. The important
  corrected zero-based `KE_MAIN.DIG` indices are: enlarge/speed-up 27,
  shrink/slow-down/damage 26, extra ball 25, extra life 34, score/autopilot 30,
  glue/extra paddle 29, fly 32, dynamite 31. Sample 28 is played when a
  capsule spawns; its “create” name does not mean the extra-ball effect.

## Real level fixtures and verification

The archive contains 60 levels and 3,300 bonus-bearing bricks, covering every
runtime ID 0..27. No group-B bonus uses a rejected high code. Level 1 uses
zero-based cell coordinates:

| Cell | Tile | Attribute | Correct effect |
|:-----|:-----|:----------|:---------------|
| (2,0), (15,0) | 37 | 3C | Freeze, ID 14 |
| (1,1), (16,1) | 37 | 28 | Darkness, ID 9 |
| (0,2) | 37 | 38 | Flying paddle, ID 13 |
| (4,4), (12,4) | 32 | 5A | Random, ID 21, magnitude 3 |
| (7,5) | 33 | 22 | One extra ball, ID 7, magnitude 3 |
| (12,7) | 35 | 1C | Extra life, ID 6 |
| (4..13,10) | 32 | 04 | Enlarge paddle, ID 0 |

Reproduce the extraction and source checks without running DOS or needing
Ghidra/IDA/Capstone:

```sh
python3 tools/inspect_bonuses.py \
  ~/games/ke/Krypton-Egg_DOS_EN/ke.exe \
  --rsc ~/games/ke/Krypton-Egg_DOS_EN/ke.rsc --verify
```

The script reads the LE object/page tables, verifies the exact executable
fingerprint and decoder instruction bytes, checks all 28 handler addresses,
compares all capsule frame sequences/timings with `assets/sprites.hh`, and
compiles a probe of the production decoder to compare **all 65,536** possible
tile/attribute pairs against the original algorithm. Python's standard library
and a C++20 compiler are sufficient. `resources/cell.cc` also contains
compile-time checks for boundary cases and actual level-1 fixtures.

## Reimplementation scope

All 28 IDs, names and capsule animations are mapped. This change corrects the
existing implemented effects: paddle size, ball size, ball speed, one extra
ball, lives and scoring. It also corrects the paddle size tables/default and
ball-kind names. `KE_DUMP_BONUS=1` now prints runtime IDs using the proper decode.

The [enemy implementation](enemies.md) also adds damage, shields, and dynamite,
plus score/lives display and life loss/respawn. Dynamite's palette flash is
still pending.

The other 16 effects still need gameplay implementation: reverse, glue,
darkness, autopilot, flight, freeze, six weapons, power/ghost balls, extra paddle,
and random dispatch. They are explicitly logged
as unimplemented instead of being given an unrelated effect. Their mappings
above are established from static code/data traces; they have not all been
exercised interactively in DOSBox. Original resize animation, palette/timer
behavior, exact DOS ball/brick collision geometry, automatic score-threshold
extra lives and the
24-falling-capsule limit remain separate implementation work.
