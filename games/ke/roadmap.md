# Neutrino engine roadmap (KE-driven)

Derived from `findings.md` (the raw engine-weakness survey) plus a cost/evidence
re-ranking. `findings.md` is the *what*; this is the *do-this-in-this-order-and-why*.

## How this roadmap was ordered

`findings.md` ranked by architectural ideal. This reorders by three rules:

1. **Lead with what actually bit us.** An item with a real bug trace outranks an
   item that is only inelegant. The intermittent paddle freeze this session is the
   anchor case.
2. **Reduce the number of coordinate spaces before labeling them.** Physics already
   runs in world coordinates. Most conversions vanish if the plumbing is fixed; a
   strong-type vocabulary applied first would just label churn that should not exist.
3. **Weight by effort, not just value.** Two of the highest-value items are also the
   cheapest and were buried near the bottom of `findings.md`.

Effort: **S** ≈ ≤ a day · **M** ≈ a few days · **L** ≈ a week+ and invasive.
"Bug attached" = we have already observed a defect this maps to.

---

## Tier 0 — The paddle story · bug attached · effort M · ✅ DONE

> **Shipped.** All three parts (0A/0B/0C) landed as a hard break across all 5
> `base_scene` implementers. Engine + KE + both examples + tests build; suite green
> (623 cases / 293,523 assertions); KE runs headless clean. The freeze is now
> unrepresentable at two layers: the engine clamps the target to geometry (0A), and the
> sim runs at a fixed dt regardless of frame stalls (0B).
>
> Two deliberate deviations from the design below, both improvements:
> - `button_state` is a small **neutrino** struct, not an sdlpp alias, so `base_scene.hh`
>   (included everywhere) need not pull in `game_application.hh`.
> - `pointer_state` carries an **`on_screen`** flag, and the position is read from
>   `SDL_GetMouseState` rather than the event-tracked position (which sits at `{0,0}`
>   until the first mouse event). A scene steering from the pointer must check the flag.
>
> **Edge delivery (review fix).** A frame running N substeps hands the full snapshot to
> substep 0 and an `without_edges()` copy to the rest, so a `pressed`/`released`
> transition is observed exactly once. Without this the default 120 Hz sim on a 60 Hz
> display ran 2 substeps per frame and fired every one-shot action **twice per click**.
> `held` is delivered to every substep (it is a state, not a transition). Still
> best-effort in one direction: a press landing on a zero-substep frame is seen only if
> the button is still down next stepping frame — a latch can close that if ever needed.
>
> Landed surface: `world::set_target/clear_target/has_target` + `move_to/move_by` →
> `move_result` (`world_types.hh`); `neutrino::sim_duration` + `fixed_step_config`
> (`application.hh`); `base_scene::fixed_update(sim_duration, input_snapshot)` +
> `render()`; `neutrino::input_snapshot` (`input/input_snapshot.hh`), sampled once per
> frame by `application::sample_input()`.

`findings.md` scatters this across items #2, #3, #8 as if independent. They are one
defect: **the paddle is driven by a position target, but the engine only speaks
velocity, sampled at event time, integrated at a variable dt.**

- target → velocity hack: `vx = (desired_x - cur_x)/dt` — `mechanics.cc:192`
- sampled only on mouse-motion events — `play_game_scene.cc:134`
- variable frame delta passed straight to physics — `application.cc:159`
  (despite `base_scene::update_physics` being documented fixed-step, `base_scene.hh:47`)

This combination is what produced the wall-pinning freeze (huge artificial velocity
into a wall). Current mitigation is a `desired_x` clamp in `handle_paddle` — a
band-aid over an API that should not force the hack.

The fix is three coherent engine changes that ship together. Signatures below are
**proposals**, written to fit the existing conventions — the `set_*` intent / `get_*`
read-back / `run` model on `world` (`world.hh:167–242`), the `units::duration`
boundary (`units.hh:44`), and the `base_scene` callbacks (`base_scene.hh:47–52`).

### 0A · Collision-resolved kinematic motion — `world`

Today the paddle is a kinematic body whose velocity KE fabricates as
`(target-current)/dt`, then reads back so the ball's bounce inherits paddle "english."
Both directions of that are wrong: the fabricated velocity can be enormous (the freeze),
and a paddle blocked by a wall still reports motion. Replace it with a *positional*
intent, resolved by the same move-and-slide the movement pass already runs
(`world.hh:380`), reporting the **effective** velocity (resolved delta / dt) so english
reads a truth, not a request.

```cpp
namespace neutrino::physics {

    // Outcome of a resolved positional move (mirrors the internal slide_result,
    // world.hh:398, but public and position-first).
    struct move_result {
        vec  position;   // resolved shape reference point, post-slide
        vec  velocity;   // EFFECTIVE velocity = applied_delta / dt; 0 on a fully
                         // blocked axis -> english reads no motion into a wall
        vec  remaining;  // requested-but-blocked delta (0,0 == move fully applied
        int  contacts;   // solids touched this move (details via the event buffer)
        bool blocked() const noexcept { return contacts > 0; }
    };

    class world {
        // -- positional intent (the mirror of set_velocity, world.hh:182) --
        // Drive a kinematic actor toward an absolute world target. run() computes the
        // per-step delta ITSELF and clamps it to what geometry allows, so a target on
        // the far side of a wall stops at the wall -- no synthetic velocity, ever.
        void set_target(collider_id cid, vec target);
        void clear_target(collider_id cid);            // revert to set_velocity control
        [[nodiscard]] bool has_target(collider_id cid) const;

        // -- immediate form (out-of-run helper, like snap_to_ground/step_up which
        //    already mutate the actor, world.hh:982). Resolves NOW and returns the
        //    outcome; use when you drive the body yourself each frame. --
        move_result move_to(collider_id cid, vec target, units::duration dt);
        move_result move_by(collider_id cid, units::displacement delta, units::duration dt);
    };
}
```

Resolution: a body with a target is resolved in `run`'s movement pass by move-and-slide
toward `target` (delta clamped to reachable), instead of `velocity*dt`. `get_velocity`
then returns the *effective* velocity, so nothing downstream changes shape. One
resolution point, deterministic.

KE before → after:

```cpp
// before (mechanics.cc:192) — fabricate a velocity, hope it isn't huge
const float vx = (desired_x - cur_x) / dt;      // <- the freeze lived here
m_world.set_velocity(m_paddle, {vx, 0.0f});

// after — state intent; the engine clamps to geometry, english reads back clean
m_world.set_target(m_paddle, {target_render_x, paddle_y});
// ... post-run, for english:
const vec paddle_v = m_world.get_velocity(m_paddle);   // 0 when pinned at a wall
```

### 0B · Real fixed-step scheduler — `application` / `base_scene`

**There is one clock: the simulation clock.** Render is not a second clock — it is an
event that fires when the display is ready to present. So the model is "one clock you
step (sim), one signal you react to (vsync)," not two timebases.

Today `application::on_update(float dt)` forwards the raw variable frame delta straight
into `scenes_manager.update_physics` (`application.cc:174`), and KE hands physics
`ms/1000` (`play_game_scene.cc:105`). Give the engine an accumulator and one
seconds-based duration. The fixed step earns its place for **determinism and stable
collision** (the freeze was partly variable-dt), not for rendering.

```cpp
namespace neutrino {

    // The ONE simulation duration, seconds-based. Physics stops receiving ms.
    using sim_duration = std::chrono::duration<float>;   // seconds

    struct fixed_step_config {
        sim_duration period{1.0f / 120.0f};  // simulation tick
        int          max_substeps = 5;       // catch-up cap (spiral-of-death guard)
        sim_duration max_frame{0.25f};       // per-frame dt clamp (stall guard)
    };
    // add to application_config: fixed_step_config fixed{};

    class base_scene {
        // fixed timestep: called 0..max_substeps times per frame with the SAME dt AND
        // the same frame-scoped input snapshot (input_snapshot: see 0C). Passing input
        // in -- rather than a service pull -- makes the dependency explicit and testable
        // (findings #12), and makes "sampled once per frame, shared across substeps"
        // visible in the signature. A scene that ignores input marks it [[maybe_unused]].
        virtual void fixed_update(sim_duration dt, const input_snapshot& in) = 0;
        // render: draw the latest committed sim state. No time argument, no interpolation
        // factor -- see the interpolation note below for why KE needs neither.
        virtual void render() = 0;
    };
}
```

The loop the engine owns (replacing `application.cc:159`):

```cpp
void application::on_update(float frame_dt) {
    const input_snapshot& in = m_input.sample();    // sample ONCE per frame (frame-scoped)
    m_accum += std::min(frame_dt, cfg.fixed.max_frame.count());
    for (int n = 0; m_accum >= cfg.fixed.period.count() && n < cfg.fixed.max_substeps; ++n) {
        scenes.fixed_update(cfg.fixed.period, in);  // constant dt + same snapshot every substep
        m_accum -= cfg.fixed.period.count();
    }
    // m_accum now holds the leftover un-stepped time in [0, period). render() ignores it.
}
```

`world::run(active_region, dt)` then always gets the fixed `period` — its `float dt`
becomes `units::duration{period.count()}` at the boundary (`world.hh:242`), and KE's
`ms/1000` conversion disappears. (Breaking change to `base_scene`: `fixed_update` +
`render()` replace `update_physics(frame_duration)`. See the migration section.)

**Why no interpolation factor (`alpha`) in `render`.** After the loop, `m_accum` is
un-stepped time in `[0, period)`, and `alpha = m_accum / period` is the *phase* between
the last committed step and the next — not a clock. Interpolating `lerp(prev, curr,
alpha)` only buys visible smoothness when the sim rate is *low* relative to the display.
It buys **nothing** for KE, which renders 320×200 pixel-art at integer scale: positions
snap to whole pixels, so a fractional interpolated position rounds to the same pixel. So
KE draws the latest state and `render()` takes no arguments. The engine *may* later add
an optional `render(sim_duration since_last_frame, float alpha)` overload for a future
high-res, smooth-scrolling game — a capability, not a requirement — but KE opts out and
stays single-clock.

### 0C · Per-frame input snapshot — input service

Today scenes see only raw `sdlpp::event` via `handle_action` (`base_scene.hh:52`), and
the only polled mouse API returns window coordinates — so KE updates the paddle target
*only when a motion event arrives* and does the render-space conversion itself
(`play_game_scene.cc:134`). Build one snapshot per frame, render-space ready, and hand
it to `fixed_update` (0B) — no per-frame service lookup.

```cpp
namespace neutrino {
    struct pointer_state {
        point       window;    // window pixels
        point       render;    // logical/presentation space (SDL_RenderCoordinatesFromWindow,
                               // the same transform application.cc:126 already configures)
        bool        on_screen;
    };
    struct button_state {
        bool down;             // held this frame
        bool pressed;          // edge: up->down since last snapshot
        bool released;         // edge: down->up since last snapshot
    };
    class input_snapshot {
    public:
        [[nodiscard]] const pointer_state& pointer() const;
        [[nodiscard]] button_state mouse(sdlpp::mouse_button b) const;
        [[nodiscard]] button_state key(sdlpp::scancode s) const;
        [[nodiscard]] bool key_down(sdlpp::scancode s) const;
        // gamepad state / action bindings: later
    };
    // built by the engine once per frame and passed to fixed_update (0B) -- not stored
    // in service_locator. An explicit, mockable dependency (findings #12), not a global.
}
```

KE before → after:

```cpp
// before — target moves only when a motion event fires, KE converts coords itself
void handle_action(const sdlpp::event& ev) {                 // play_game_scene.cc:134
    if (ev is mouse_motion) m_model.set_paddle_target(to_render(ev.x));
}

// after — the frame's snapshot arrives as a param; poll render-space pointer directly,
// no event gating, no manual conversion. handle_action keeps only discrete events.
void fixed_update(sim_duration dt, const input_snapshot& in) {
    m_model.set_paddle_target(in.pointer().render.x);
    ...
}
```

**Deletes in KE:** the `desired_x` band-aid + `(target-cur)/dt` hack (0A), the `ms/1000`
conversion (0B), the event-gated target update + manual coord conversion (0C). Net: the
paddle becomes "target = polled render-space mouse; engine resolves it against geometry
at a fixed dt" — and the freeze is no longer expressible.

**Migration notes.**

- *0A is additive* — `set_velocity` stays; `set_target` is a parallel intent. No caller
  breaks; KE opts in.
- *0B breaks `base_scene`* — `update_physics(frame_duration)` splits into
  `fixed_update(sim_duration, input_snapshot)` + a no-arg `render()`. Every scene (KE
  has a few: play, gallery) updates its two overrides. A transition shim can keep
  `update_physics` as a default `fixed_update` (ignoring the new snapshot arg) until
  scenes move over. Physics speaks `sim_duration`/`units::duration` end to end; the
  existing `frame_duration` (ms) can stay for any render-only wall-clock cosmetics, but
  KE has none, so `render()` needs no time at all.
- *0C rides on 0B's signature change* — the snapshot is the second `fixed_update`
  parameter, so it lands with the 0B break rather than as a separate one. `handle_action`
  stays for discrete events (quit, pause); only the *continuous* polling (mouse position,
  held keys) migrates to the snapshot.

Order within Tier 0: **0B first** (establishes the fixed dt everything else assumes),
then **0A** and **0C** in parallel on top of it.

---

## Tier 1 — Cheap footguns & alignment · effort S each · ✅ DONE

> **Shipped.** Both parts landed additively (nothing removed, no caller broken). Suite
> green at 626 cases / 293,594 assertions; KE builds and runs headless clean.
>
> **1a** — `require_visual` / `require_frame_rect` / `require_origin` / `require_metrics` /
> `require_clip` on `sprite_set` **and** `sprite_set_handle`, each aborting with the missing
> key *and* the set size (a torn-down set and a bad index are otherwise indistinguishable).
> An empty lease reports "no set acquired" rather than blaming the key. All 5 KE
> `value_or(rect{})` sites converted; the worst was `model.cc`, which silently fell back to
> a **1×1 paddle** — present to physics, invisible, unplayable. The `backdrop.cc` fill-tiling
> guard became an explicit assert: the frame size is the loop *step*, so a degenerate 0×0
> frame would have spun forever, and the old `if (w > 0 && h > 0)` quietly skipped the whole
> backdrop instead.
>
> **1b** — `sprite_metrics {texture_rect, source_size, trim_offset, pivot, logical_pivot}`
> with `trimmed()` / `local_bounds()` / `visible_bounds_at()` / `logical_bounds_at()`.
> `sprite_visual` now carries `source_size` + `trim_offset` (untrimmed-by-default sentinel,
> resolved inside `metrics()` so no consumer sees it), and `build_sprite_set` forwards the
> def's trim metadata instead of discarding it.
>
> **Honest scope note on 1b:** this closes the *engine* information gap — the authored frame
> is no longer unrecoverable at runtime. It does **not** by itself delete KE's manual
> half-frame centring math. KE's BOB frames are untrimmed and every KE pivot is forced to
> `{0,0}` (`sprites.cc`), so `metrics()` is accurate for KE but returns
> `source_size == texture_rect` today. Deleting the manual `- fr.w*0.5f` offsets needs KE to
> author **centre pivots** for the ball/effect/capsule sets — a KE-side change with visual
> risk, best done with the game on screen. Tracked as follow-up, not silently claimed here.

### 1a. `require_*` asset lookups (findings #10) · S
`value_or(rect{})` silently turns missing frame metadata into **zero geometry**
instead of a load-time error (`model.cc:134`, `backdrop.cc:12`). Add
`require_visual` / `require_frame_metrics` / `require_clip` + whole-definition
validation before GPU registration. Turns a silent runtime footgun into a loud
startup failure.

### 1b. Preserve complete sprite metrics (findings #4) · S–M
`sprite_visual_def` carries logical source size, trim offset, logical origin
(`sprite_def.hh:65`), but construction keeps only the packed atlas rect + baked
origin; runtime `sprite_visual` can't recover logical bounds (`sprite_sheet.hh:132`).
KE compensates by overriding all origins to `{0,0}` (`sprites.cc:34`) and manually
subtracting half a ball frame when drawing (`play_game_scene.cc:45`).

Retain a `sprite_metrics` (packed rect, logical size, trim, pivot, local visible
bounds) with `local_bounds()` / `visible_bounds_at(anchor)` helpers. Annoying-not-
dangerous, so it sits below the paddle story — but it deletes the manual offset math.

---

## Tier 2 — Ricochet correctness (findings #6) · effort M · ✅ DONE

> **Shipped.** Suite green at 637 cases / 293,697 assertions; KE builds and runs headless clean.
>
> `bullet_on_hit {stop, bounce, slide}` on the `bullet` DTO, plus
> `world_config::max_bullet_responses` (default 4). The bullet pass is now an iterative
> solver: a step is a **time budget**, and after a contact a `bounce`/`slide` bullet
> re-aims both its stored velocity and its leftover displacement and keeps spending,
> so it leaves the surface within the same step. `stop` is the default, so every
> pre-existing bullet test passes untouched — this is purely additive.
>
> **Chose geometric reflection over material-driven restitution.** `eval_velocity_response`
> (what move-and-slide uses) was the tempting reuse, but it would have made a clean bounce
> depend on tuning `restitution` on every wall, brick and paddle material — with a default
> material silently absorbing the ball instead. A speed-preserving `v - 2(v·n)n` is what an
> arcade ricochet wants and needs no material setup. Material-driven response can layer on
> later as a fourth policy.
>
> **A skin cushion was needed.** After responding, the bullet resumes from exactly *on* the
> surface. A bounce separates so it is fine, but a **slide** moves tangentially — neither
> approaching nor separating — so the next cast re-hit the same surface at toi 0 and
> reported two contacts for one physical touch (caught by the test asserting one event).
> Responses now back off along the normal by `world_config::skin`, the same cushion
> move-and-slide keeps.
>
> **KE migration was one line.** `spawn_ball` sets `on_hit = bounce`; `handle_balls` needed
> no change, because it reflects its *own* model copy of the velocity (which physics never
> writes to), so it recomputes exactly the value the engine already applied and its
> write-back agrees rather than double-reflecting. The paddle still overrides the geometric
> bounce with "english" after `run()` — that override now applies to a ball whose position
> already continued past the contact, a sub-frame discrepancy in exchange for losing the
> visible one-frame rest against every surface.

## Tier 2 — original plan

The bullet pass stops at first contact, emits an event, and **discards remaining
frame movement** (`world.hh:448`); KE reflects velocity only *after* `world::run`,
so the ball leaves the surface on the following frame (`mechanics.cc:264`).

Offer one of: an iterative projectile solver that consumes remaining time after each
hit, or a contact-response callback (reflect / stop / pierce / destroy). Game policy
(paddle "english") stays in KE; the engine just lets the response continue through
the unused part of the step. Visually tolerable today, so below Tiers 0–1.

---

## Tier 3 — Typed world space, on a simplified surface (findings #1) · effort L · ✅ DONE

> **Header hygiene: DONE.** `world_point` / `world_rect` moved out of the heavy tile-world
> header into `video/geometry_types.hh` (which `world_common.hh` already included, so every
> existing consumer keeps compiling unchanged — the move is source-compatible). KE's three
> files that pulled in `world/world_common.hh` *purely* for one float point — `backdrop.cc`,
> `model.hh`, `mechanics.hh` — now include the light header instead, dropping
> `<filesystem>`, `<map>`, `<variant>`, `<vector>` and the colour header from their
> translation units.
>
> **The strong-type vocabulary: DONE.** `include/neutrino/world_space.hh` defines
> `world_seconds` / `world_delta` / `world_pos` / `world_velocity` / `world_bounds`, plus
> `render_pos` / `window_pos` at the input edge, as **distinct structs** (aliases would compile
> everywhere and catch nothing) with only the meaningful algebra: `pos - pos = delta`,
> `pos + delta = pos`, `velocity * seconds = delta`, `delta / seconds = velocity`. Adding two
> positions does not compile.
>
> **Correction to the earlier plan recorded here: "it must land in one piece" was wrong.**
> Introduced as *overloads* rather than replacements, the migration is incremental — every
> caller keeps compiling and converts one at a time, with a green build at each step. That
> mistaken premise was the only thing making this look like a big-bang refactor.

### What landed

| Layer | Change |
|---|---|
| `world_space.hh` | The vocabulary + algebra. Header-only, `<chrono>`/`<cmath>` only. |
| `physics::world` | Typed overloads: `set_velocity`, `set_target`, `move_by`, `move_to`, `velocity_of`, `position_of`, `bounds_of`; `world_move_result`. Thin delegations to the `vec` forms. |
| `geometry_types.hh` | `to_world_point` / `to_world_pos` — the **named**, non-implicit crossing to the untyped point the renderer wants. |
| `globals.hh` | `to_render_coords(window_pos) -> render_pos`, `to_window_coords(render_pos) -> window_pos`. |
| `input_snapshot` | `pointer_state::window` is a `window_pos`, `::render` a `render_pos` — previously both a bare `point<float>`, hence interchangeable. |
| KE model | `ball_state`, `brick`, `capsule`, `hit_effect` hold `world_pos` / `world_velocity`. Integration reads as physics: `b.pos += b.vel * dt`. |
| KE mechanics | `reflect` / `scaled` / `rotate` / `paddle_bounce` take and return `world_velocity`; `tick` takes a `sim_duration`. |

### What it deleted

- **`shape_center()`** — KE's hand-rolled `get_if<aabb>` / `get_if<circle>` centre read-back,
  which silently froze `ball.pos` for any shape it did not enumerate. `world::position_of`
  replaces it for every shape.
- The paddle's `get_if<aabb>` read-back, for `bounds_of(...).min` (exact corners, no round trip
  through the centre).
- Every `vec{v.x, v.y}` / `world_point{...}` repack in `mechanics.cc` except **one**:
  `bullet::velocity` is a plain struct field, so construction still crosses explicitly.

`world_point` now appears in KE only at the drawing edge.

### Still open

- **Retiring the untyped forms.** The `vec` overloads stay public while the engine's own tests
  and any non-KE consumer use them. Nothing forces the order; convert, then delete.
- **`sprite_batch`** still draws in `world_point`. That is the deliberate boundary today
  (`to_world_point` at the call), not an oversight — typing the renderer is its own change.

### Original design guidance

`findings.md` ranks this #1; deliberately deferred here. Rationale:

- Most invasive item, least direct bug evidence.
- Neutrino **intentionally** strips `units::` at the public boundary
  (`units.hh:15`); re-adding a full vocabulary re-litigates that without a bug to
  justify it.
- Physics already runs in world coordinates, so after Tiers 0–1 the real space count
  is small (world vs. render-int vs. window-input). **Fewer spaces to label.**

When it lands, keep it minimal: `world_pos`, `world_delta`, `world_velocity`,
`world_bounds`, plus `render_pos` / `window_pos` at the input edge. Physics shapes
use `world_pos`; do **not** introduce a separate `physics_pos`. Also move
`world_point` out of the heavy tile-world header — KE pulls in
`world/world_common.hh` only for one SDL float point (`world_common.hh:100`).

---

## Tier 4 — Architectural hygiene

Titled "real warts, no current bug" when written. **Item 12 turned out to have two.**

| # | Item | Evidence | Effort | Status |
|---|------|----------|--------|--------|
| 12 | Explicit scene/application context | assets published once then cleared on scene exit (`krypton_egg.cc:125`, `play_game_scene.cc:99`) | M | ✅ **DONE** — see below |
| 9 | General compositor independent of tile worlds | magic `+1000/+1500/+2000` depths, `play_game_scene.cc:22` | M | ✅ **DONE** — see below |
| 7 | Typed collision ownership vs. numeric EID ranges | `mechanics.cc:19`, `EID_BALL_BASE` subtraction | M | ✅ **DONE** — see below |
| 5 | Decouple sim from camera culling | `world.hh:374/431` — off-region actors freeze, off-region bullets skip narrow-phase | M | ✅ **DONE** — see below |
| 11 | Scoped codec registration + audio-bank API | DIG decoder re-registered per `audio::load` (`sfx.cc:35`, `audio.hh:106`) | M | 🟡 **PARTIAL** — registration fixed; bank/PCM API not started. See below |

### 12 — Explicit scene/application context ✅

**Two bugs, not zero.**

1. **Scene cleared what it did not own.** `play_game_scene::on_exit()` called
   `rs::clear_ke_assets()`, but the assets belong to the application and are published once, in
   `ready()`. Any *second* gameplay scene — a level transition, or a restart after game over —
   would hit the `ENFORCE` in `require_ke_assets()` on entry. Latent only because KE has no
   level transition yet.
2. **Teardown ran too late, and silently did nothing.** `ke_assets` was a by-value member, so it
   died in `~ke()` — after `game_application::on_quit()` had taken the renderer down. It exited
   0, but only because the sprite services were already unpublished by then, so the release path
   no-opped. **The sheets were never actually unregistered.** Moving the release into the new
   `teardown()` hook — where the services *are* live — made it run, and it immediately aborted
   with *"Cannot unregister sprite sheet while it is still used"*: KE registers 30 animations
   against the KE_SPELL sheet in `define_sprites` and never unregistered them. A registered
   animation counts as a sheet user, and the abort surfaces from a destructor (`noexcept`), so
   it terminates rather than throws.

**What landed**

| Layer | Change |
|---|---|
| `application` | `virtual void teardown()` — the counterpart to `ready()`. Called in `on_quit()` after the scene stack is finished and before the engine cache / renderer / audio go away. Wrapped in try/catch. |
| `play_game_scene` | Takes `rs::ke_assets&` at construction. No longer looks the registry up in `on_enter`, no longer clears it in `on_exit`. |
| `ke` | Owns `std::unique_ptr<rs::ke_assets>`; `teardown()` releases sprites → unpublishes → frees. The `KE_DUMP_BONUS` path now allocates nothing, so teardown no-ops there. |
| `rs::release_sprites()` | New counterpart to `define_sprites()`: unregisters the 30 animations and invalidates the ids. |

Verified: both shutdown paths exit 0, and **valgrind reports 0 errors / 0 bytes definitely lost**
on the normal path — which is the evidence that the release now genuinely runs.

> Follow-up (KE-side, not an engine gap): moving those animations into `sprite_def::clips` —
> which the engine already supports — would make the sets own them and delete
> `release_sprites()` entirely. See "Not an engine gap" below.

### 9 — Ordering bands instead of magic depth offsets ✅

`sprite_batch` had one float sort key, so a caller wanting *category* order on top of *positional*
order had to fake it by adding a constant larger than any coordinate: KE drew balls at `y + 1000`,
capsules at `y + 1500`, sparks at `y + 2000`. Those constants silently encode "y never exceeds
500", an assumption nothing checked and nothing wrote down — and one taller playfield away from
sprites crossing bands.

**`neutrino::draw_layer`** — a coarse band sorted *before* depth. The batch's full key is now
`(layer, depth, insertion order)`, with `add(pos, layer, depth, ...)` overloads alongside the
existing depth-only ones (which are the new form at the default band `{0}`, so the two mix
cleanly and nothing existing changed behaviour).

KE now names its four bands — `layer_playfield` / `layer_balls` / `layer_capsules` /
`layer_effects` — and passes plain `pos.y` as the depth. No magic constants remain in the draw
path. Pinned by a test where a depth of `9999` in a low band still draws under `-999` in a high
one, which is exactly what the old scheme could not guarantee.

> The tile-world compositor is untouched. Building it on this same primitive is the rest of
> finding #9; the ordering key it would need now exists.

### 11 — Idempotent codec registration 🟡

**Done: the registration defect.** `register_decoder` appends to musac's registry unconditionally
and the registry has no unregister, so KE's `audio::load` — which runs on *every* gameplay-scene
entry — added one more duplicate DIG decoder per level entered, and every later codec probe walked
them all.

`neutrino::register_decoder_once(codec_id, accept, factory, priority)` is the de-duplicating form:
first call with an id registers, later calls with that id return `false` and do nothing. This lets
a codec be registered next to the code that needs it instead of being hoisted to a one-time init
path purely to dodge duplicates. KE registers `"ke.dig"`.

De-duplication is engine-side, in `sound_system`, and the ids die with the audio system. It is
deliberately **not** an RAII handle: musac exposes no unregister, and the standing rule is that
neutrino consumers touch musac only to write a `musac::decoder`. A handle promising release we
cannot deliver would be worse than an honest one-way registration. The plain `register_decoder`
stays, now carrying a warning about exactly this.

**Not done: the bank/PCM half.** `sound_bank`, PCM-with-source-rate loading, and named/indexed
effects owned by an audio bundle. KE still opens its bank, extracts each sample into a fresh
one-sample encoded bank, wraps that in a stream and feeds it back to `load_sfx`. That round trip
is real waste, but it is an API *addition*, not a defect, so it did not come with this pass.

### 7 — An owner slot instead of reserved entity-id ranges ✅

`entity_id_t` is one flat `uint32_t` namespace, so KE carved ranges out of it to encode *kind*:
bricks took their index `0..N-1`, the paddle and walls sat at `1<<20`, balls at `1<<21 + i`. A
ball hit was then routed by `mover - EID_BALL_BASE`. Nothing checked those boundaries — a level
with `1<<20` bricks would have had a brick read as the paddle — and the subtraction quietly
depended on balls never being erased from the model.

**A per-collider owner slot**: 8 bytes of opaque, game-defined payload the simulation never
touches. Set it at `add()` time via the DTOs' `user_data` field or afterwards with `set_owner`,
read it back from an event handle with `owner_as<T>`. The `collider_owner` concept constrains it
to trivially-copyable values of at most 8 bytes; `to_user_data` / `from_user_data` do the packing.

KE now carries `struct ke_owner { ke_kind kind; uint32_t index; }`, so routing is a switch on a
named kind and the index travels with the collider. Entity ids remain (the physics uses them for
its own dedup) but identify nothing. Pinned by a test where two colliders **share an entity id**
and are still told apart.

> The slot is untyped storage — nothing records which type was written, so reading as a different
> type reinterprets the bits. Documented, and the natural discipline is one owner type per world.
> A stale handle is still rejected by the generation check rather than read from a recycled slot.

### 5 — The activity region is a gameplay choice, not a camera ✅

`run()` took an `active_region` documented as "the camera/simulation window" — one argument
serving two unrelated purposes. Off-region movers freeze and off-region bullets skip the narrow
phase, so passing the camera's view makes the simulation depend on the camera: the same play
yields different outcomes at a different window size or zoom, and a bullet crossing off-screen
geometry tunnels through it.

- **`run(dt)`** — no region, simulate everything. This is now the default form, and what a game
  without dormancy actually wants.
- **`run(active_region, dt)`** — the region is `std::optional<aabb>`, kept as an explicit argument
  rather than stored config so the decision to freeze part of the world is visible at every call
  site that makes it. Documented as a **gameplay** decision ("enemies do not act until the player
  is near"), with a warning against passing a camera view and a pointer to renderer-side culling
  for the drawing case.

`nullopt` means no culling rather than an all-encompassing box — sentinel coordinates that large
overflow in the sweep tests.

KE dropped its hand-built 320×200 box: a full-world region that was spelled like a camera view,
and only coincided with one because KE has no camera.

---

## Not an engine gap — use what already exists

`findings.md` is honest that some friction is KE not using facilities Neutrino
already ships. Worth a KE-side cleanup independent of the roadmap above:

- **Clips + RAII `sprite_instance` already exist.** KE hand-registers animations and
  stores raw `sprite_state_id` in its model (`sprites.cc:65`, `model.hh:58`). Moving
  KE animations into `sprite_def.clips` would delete most manual unregistering.
- **Constructor injection** already resolves the asset publish/clear fragility (#12)
  without any engine change.

---

## Execution order

1. ~~**Tier 0** — paddle story (resolved `move_to` + fixed-step + input snapshot).~~ ✅ **DONE**
2. ~~**Tier 1** — `require_*` (1a) and sprite metrics (1b).~~ ✅ **DONE**
3. ~~**Tier 2** — multi-contact projectile.~~ ✅ **DONE**
4. ~~**Tier 3** — typed world space.~~ ✅ **DONE**
5. ~~**Tier 4** — hygiene.~~ ✅ **DONE** (item 11's bank/PCM half deferred; see there)

All five roadmap tiers are now closed. What remains is listed under *Open follow-ups* below and
in "Not an engine gap" — plus the gameplay work KE needs, which no engine change unblocks.

### Open follow-ups from the shipped tiers

- **KE centre pivots** (from 1b): author centre pivots for the ball / effect / capsule sets
  so `logical_bounds_at` replaces the manual `- fr.w*0.5f` offsets in
  `play_game_scene.cc` and `mechanics.cc`. Needs the game on screen to verify alignment.
  Note these offsets are now typed `world_delta`s, so the replacement is local.
- **Impact point on `BULLET_HIT`** (from Tier 2): `world_event` carries the normal and the toi
  but not the position, so KE cannot adopt `bullet_on_hit::bounce` — its post-`run` read-back
  would see the rebound position, not the impact. Small change; unblocks removing the ball's
  visible one-frame rest against a surface.
- **Retire the untyped physics overloads** (from Tier 3): the `vec` forms of `set_velocity` /
  `set_target` / `move_by` / `move_to` remain for the engine's own tests. Convert, then delete.
- **Frame-perfect input edges** (from 0C): a one-frame `pressed` edge can be missed on a
  zero-substep frame. Add a latch if a scene ever needs it; KE does not.

`findings.md`'s conclusion — "the first four would remove most of KE's conversion,
synchronization and visual-alignment code" — is correct. This roadmap keeps that
conclusion but swaps in the first four that have bugs attached and lower cost.
