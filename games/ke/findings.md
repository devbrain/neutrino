• I found several places where KE is exposing real Neutrino design weaknesses. The most important are coordinate semantics, physics-body state, fixed-step timing, and sprite metrics.

No files were changed.

## Highest-priority engine improvements

1. Make world space canonical, with strong quantity types

KE currently mixes:

- Integer render coordinates in paddle_info.
- world_point for both positions and velocities.
- physics::vec for physics positions, normals and velocities.
- Integer sprite rectangles.
- Raw float seconds and millisecond durations.

This creates conversion code throughout games/ke/game/model.hh:17 and games/ke/game/mechanics.cc:33. Even the default, screen-space sprite_batch accepts a world_point, so its coordinate space is determined by constructor state rather
than its argument type (include/neutrino/video/world/sprite_batch.hh:58).

I would not introduce a separate physics_pos if physics is operating directly in game-world coordinates. Instead:

- world_pos
- world_delta
- world_velocity
- world_rect or world_bounds
- render_pos
- window_pos
- screen_pixel

Physics shapes should use world_pos; physics-specific types should describe quantities and behavior, not duplicate the coordinate system.

This would catch mistakes such as using a velocity where a position is expected while avoiding pointless world-to-physics copying. Neutrino already applies this idea internally with units::velocity, duration and displacement, but
deliberately removes that safety at the public boundary (include/neutrino/physics/geometry/units.hh:15).

Also, world_point should move out of the large tile-world header. KE includes include/neutrino/world/world_common.hh:1 merely to obtain an SDL float point declared at line 100.

2. Separate physics body transform from collider geometry

Neutrino stores shapes at absolute world coordinates. Consequently, set_shape means both “resize” and “teleport,” while get_shape returns a wide variant that callers must inspect (include/neutrino/physics/collide/world.hh:167).

KE therefore has to:

- Duplicate physics positions and velocities in its model.
- Keep collider arrays parallel to model arrays.
- Reconstruct complete AABBs when resizing the paddle.
- Visit shape_t merely to obtain a ball’s centre.
- Copy resolved positions back after every step.
- Truncate the paddle’s float position to integers.

See games/ke/game/mechanics.hh:45, games/ke/game/mechanics.cc:44, and the read-back at games/ke/game/mechanics.cc:431.

A better body API would have:

- A transform or position independent of local collider shape.
- position(), set_position(), move_by() and possibly collision-resolved move_to().
- Shape replacement or resizing with an explicit preserved anchor.
- Typed accessors such as circle_of(), bounds(), center() and body_view().
- Typed body handles where practical.

A collision-resolved move_by(displacement) is particularly appropriate for the mouse-controlled paddle. KE currently converts a positional target into (target-current)/dt, potentially creating enormous artificial velocities (games/ke/
game/mechanics.cc:192).

3. Provide a real fixed-step scheduler

base_scene::update_physics is documented as fixed-timestep, but the application passes the variable frame delta directly once per frame (include/neutrino/scene/base_scene.hh:47, src/neutrino/application.cc:159).

This affects KE collision behavior and mouse response during stalls. It also conflicts with the renderer API, which already exposes a “fixed-step interpolation factor” without Neutrino producing that factor.

Neutrino should own:

- A configurable fixed simulation period.
- An accumulator with maximum catch-up and frame-delta clamping.
- A fixed-update callback.
- A render interpolation factor.
- One public chrono duration type, preferably seconds-based.

KE currently receives milliseconds, divides by 1000, and passes a raw float to physics (games/ke/scenes/play_game_scene.cc:105). Physics should accept the same strong duration type.

4. Preserve complete sprite metrics at runtime

This is the clearest flaw exposed by the earlier paddle/ball visual mismatches.

sprite_visual_def contains logical source size, trim offset and logical origin (include/neutrino/video/sprite/sprite_def.hh:65). During construction, Neutrino keeps only the packed atlas rectangle and baked origin. Runtime
sprite_visual cannot recover the logical, untrimmed bounds (include/neutrino/video/sprite/sprite_sheet.hh:132).

KE consequently:

- Overrides all BOB origins with {0,0}.
- Uses packed frame width and height as gameplay dimensions.
- Manually subtracts half the ball frame when drawing.
- Manually calculates effect and capsule offsets.

See games/ke/assets/sprites.cc:34 and games/ke/scenes/play_game_scene.cc:45.

The built sprite should retain a sprite_metrics value containing:

- Packed atlas rectangle.
- Logical source size.
- Trim offset.
- Logical pivot.
- Local visible bounds relative to the pivot.
- Helpers such as local_bounds(), visible_bounds_at(anchor) and logical_bounds_at(anchor).

That does not mean graphics should automatically define gameplay collision. It means games can deliberately align authored collision bounds and visuals without reverse-engineering draw behavior.

5. Decouple simulation correctness from camera culling

world::run requires an active_region. Off-region kinematic bodies freeze, while off-region bullets continue moving but skip collision tests (include/neutrino/physics/collide/world.hh:374, include/neutrino/physics/collide/
world.hh:431).

That makes camera position affect physics correctness. A projectile can pass through an off-camera wall, while an actor at the same location stops aging entirely.

Simulation sleeping should instead be:

- Explicitly configured per body or world.
- Independent from render visibility.
- Deterministic.
- Able to use larger simulation regions than the current camera.
- Conservative: skipping narrow-phase must not permit tunnelling through geometry.

KE currently avoids the worst consequences by passing the whole 320×200 screen as a static active region (games/ke/game/mechanics.cc:417).

6. Support same-step projectile responses

The bullet pass stops at the first contact, emits an event and discards the remaining frame movement. KE reflects the velocity only after world::run, so the ball leaves the surface on the following frame (include/neutrino/physics/
collide/world.hh:448, games/ke/game/mechanics.cc:264).

For a ricochet game, Neutrino should offer either:

- An iterative projectile solver that consumes remaining time after each hit.
- A contact-response callback that can reflect, stop, pierce or destroy the projectile.
- A material-driven bounce mode.

Game policy such as paddle “english” still belongs in KE, but the engine should let that response continue through the unused part of the current step.

## Secondary improvements

7. Typed collision ownership instead of numeric entity namespaces

KE assigns bricks their vector index and reserves large numeric ranges for paddle, wall and ball identities (games/ke/game/mechanics.cc:19). Ball event routing then subtracts EID_BALL_BASE.

Consider a templated or typed user payload for physics worlds, or a collider-owner binding API. Raw uint32_t makes unrelated entity namespaces accidentally compatible and encourages parallel arrays.

8. Render-space input snapshots

Scenes receive raw sdlpp::event, and the only polled mouse-position API returns window coordinates. KE therefore updates the paddle target only when a mouse-motion event arrives and performs the render conversion itself (games/ke/
scenes/play_game_scene.cc:134).

A per-frame input snapshot should provide:

- Current pointer position in window and render coordinates.
- Button edge and held state.
- Keyboard/gamepad state.
- Optional action bindings.
- Focus/capture status.

This would make polling the current render-space mouse position during fixed update the default. It directly addresses the class of intermittent paddle-response problem previously observed.

9. A general compositor independent of tile worlds

Neutrino’s render_layer and world_compositor are attached to world_renderer. A non-tile game such as KE therefore manually loops over every entity category and uses magic depth offsets such as +1000, +1500 and +2000 (games/ke/scenes/
play_game_scene.cc:22).

A general scene compositor could offer named or typed render layers with keys like (layer, y, insertion_order). The tile-world compositor could then be built on that same primitive.

10. Improve required asset lookup and ownership

KE repeatedly turns missing frame metadata into zero rectangles using value_or(rect{}). For configuration invariants this produces bad geometry rather than a useful load-time error; examples appear in games/ke/assets/backdrop.cc:12
and games/ke/game/model.cc:134.

Add required counterparts such as:

- require_visual
- require_frame_metrics
- require_clip
- Whole-definition validation before GPU registration

However, part of the current complexity is not an engine gap: Neutrino already supports clips inside sprite_def and RAII sprite_instance. KE manually registers animations and stores raw sprite_state_id values in its domain model
instead (games/ke/assets/sprites.cc:65, games/ke/game/model.hh:58). Moving the KE animations into sprite_def.clips would use existing Neutrino facilities and eliminate much manual unregistering.

11. Scoped codec registration and an audio-bank API

Every audio::load registers the DIG decoder again, while register_decoder returns no ownership token and exposes no unregister/idempotence mechanism (games/ke/game/sfx.cc:35, include/neutrino/audio/audio.hh:106).

KE also opens the bank, extracts each sample into another encoded one-sample bank, wraps that in a stream, and feeds it back into load_sfx.

Useful engine additions would be:

- An RAII or idempotent codec-registration handle.
- Registration by stable codec identifier.
- sound_bank support.
- Loading PCM with source rate/channel metadata, letting Neutrino resample it.
- Named or indexed effects owned by an audio resource bundle.

12. Explicit scene/application context

KE mirrors Neutrino’s global-service style with global model, asset and audio accessors. The asset registry is app-owned, published once, but cleared by the gameplay scene on exit; recreating that scene would call require_ke_assets()
before republishing it (games/ke/krypton_egg.cc:125, games/ke/scenes/play_game_scene.cc:99).

Constructor injection can fix this in KE today. Longer-term, a scene context containing scoped asset, render, audio and input services would make ownership and test substitution clearer while retaining global convenience functions.

## Recommended engine roadmap

I would prioritize the work in this order:

1. Canonical world-space and public quantity types.
2. Physics transforms, positional movement and typed ownership.
3. Actual fixed-step scheduling.
4. Complete sprite metrics and anchor-relative bounds.
5. Explicit simulation/sleep policy and multi-contact projectiles.
6. Render-space input snapshots.
7. General compositor, asset validation and scoped service/resource APIs.

Those first four would remove most of KE’s conversion, synchronization and visual-alignment code while making the same classes of mistakes harder to express in future games.
