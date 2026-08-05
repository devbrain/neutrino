---
sidebar_position: 1
title: The frame
description: One clock for the simulation, rendering as an event, and why your update sees a constant dt.
---

# The frame

Neutrino has **one clock: the simulation clock.** Rendering is not a second timebase — it is an
event that fires when the display is ready to present. Getting this distinction right at the start
makes the rest of the engine read predictably, because almost every timing question reduces to it.

Concretely: your scene's `fixed_update` receives a **constant** `dt`, no matter what the frame rate
is doing. It may be called zero times in a frame, or once, or several times.

## Why not just use the frame delta?

Passing the real elapsed time straight into your logic is simpler, and it is what most small games
start with. It breaks in three ways that compound:

- **Physics stops being reproducible.** The same input on a 144 Hz machine and a 60 Hz machine
  produces different trajectories, because the integration steps differ. Bugs become unreproducible.
- **A stall becomes a teleport.** One long frame — a shader compile, the window being dragged,
  a breakpoint — hands your simulation a `dt` of half a second. Fast-moving bodies jump straight
  through walls that a smaller step would have caught.
- **Tuning stops transferring.** Jump heights and acceleration values found on one machine feel
  wrong on another.

A fixed step removes all three by construction. The simulation always advances in identical
increments; only *how many* increments a frame runs varies.

## The loop

Each frame, the engine adds the elapsed real time to an accumulator, then drains it in whole ticks:

```text
accumulator += min(frame_delta, max_frame)

while accumulator >= period and steps < max_substeps:
    fixed_update(period, input)
    accumulator -= period
    steps += 1

render()
```

Whatever time is left over stays in the accumulator and carries into the next frame, so no time is
lost — it is just deferred to the next whole tick.

## Configuring it

```cpp
neutrino::application_config cfg;
cfg.fixed.period = neutrino::sim_duration{1.0f / 60.0f};  // 60 Hz simulation
cfg.fixed.max_substeps = 5;
cfg.fixed.max_frame = neutrino::sim_duration{0.25f};
```

| Field | Default | What it controls |
|---|---|---|
| `period` | 1/120 s | The simulation tick. Every `fixed_update` receives exactly this `dt` |
| `max_substeps` | 5 | Most ticks one frame may run, capping catch-up after a stall |
| `max_frame` | 0.25 s | Clamp applied to the frame delta before it reaches the accumulator |

All three are validated once at construction. A non-positive or non-finite value is rejected there
rather than silently producing a frozen or exploding simulation.

### Why two separate guards

They protect against different things, which is why neither replaces the other.

`max_frame` clamps a single pathological frame — the debugger pause, the window drag. Without it,
one such frame dumps a huge delta into the accumulator and the loop tries to run it all at once.

`max_substeps` caps sustained inability to keep up. When the loop hits the cap with time still
owed, the engine **discards the backlog** rather than carrying it. That choice matters: carrying it
means that on any machine slower than `max_substeps / period`, the debt grows every single frame,
the simulation falls further and further behind real time, and input latency climbs without bound.
That is the spiral of death the cap exists to prevent. Dropping the debt makes the game run in slow
motion during the stall — visibly degraded, but responsive and recoverable.

## Input across substeps

One frame samples input **once**, and that snapshot goes to every substep of that frame. But a
button press is a *transition*, not a state, and replaying a transition would fire one-shot actions
more than once per physical press. At the default 120 Hz simulation on a 60 Hz display — two
substeps per frame — every click would fire twice.

So the engine splits the snapshot:

- **substep 0** receives the full snapshot, including `pressed` and `released` edges
- **later substeps** receive a copy with edges cleared, but `held` intact

`held` describes a state, so every substep should see it. `pressed` describes a moment, so exactly
one substep should. This applies to the polled input APIs as well, not just the snapshot object.

See [Input](./input.md) for the full model, including how this applies to the polled input APIs.

## Rendering

`render()` is called once per frame, after any substeps. It draws the latest committed simulation
state.

There is deliberately **no interpolation factor**. A renderer that blends between the previous and
current simulation states produces smoother motion for continuous-resolution graphics — but
Neutrino's first target is integer-scaled pixel art, where a sub-pixel blend rounds away to nothing
at draw time. The parameter would be noise in every signature for a benefit the intended output
cannot show. If you are drawing at continuous resolution and want it, that is a reasonable feature
request, not a limitation you should work around by re-deriving positions in `render`.

## What this means for your code

- Put simulation in `fixed_update` and drawing in `render`. Do not advance game state while drawing.
- Treat the `dt` you are handed as authoritative. Do not measure elapsed time yourself.
- Do not assume `fixed_update` runs once per frame. It may run zero times, or five.
- Do not assume `render` runs between every pair of substeps. It does not.

## Next

[Scenes](./scenes.md) — what `fixed_update` and `render` are called *on*, and how the stack decides
which scenes receive them.
