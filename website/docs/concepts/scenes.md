---
sidebar_position: 2
title: Scenes
description: The scene stack, lifecycle hooks, what a scene does to the scenes below it, and how transitions are applied.
---

# Scenes

A scene is one screen of your game: a menu, the gameplay, a pause overlay. The engine keeps them in
a **stack**, and each frame it dispatches updates and drawing to some slice of that stack.

You implement [`base_scene`](pathname:///api/). Two members are pure virtual, so the
compiler makes you supply them:

```cpp
class my_scene : public neutrino::base_scene {
  public:
    void fixed_update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render() override;
};
```

Everything else has a default, so override only what you need:

- `flags()` returns `hides_below | pauses_below`: a full screen (see below).
- `handle_event(ev)` ignores raw events.
- The lifecycle hooks — `on_enter`, `on_exit`, `on_pause`, `on_resume`, `on_resize` — do nothing.

See [Tutorial Step 1: A window and an empty scene](../tutorial/step-01-window-and-scene.md) for a working
minimal scene implementation.

## Full screen or overlay: `scene_flags`

`flags()` is the single most consequential thing a scene declares: what it does to the scenes
beneath it. The bits are independent; the scene carrying them is always updated and drawn itself.

| Flag | The scenes below |
|---|---|
| `hides_below` | are not drawn: this scene covers the display |
| `pauses_below` | are not updated; the one directly below gets `on_pause` when this scene is pushed and `on_resume` when it is popped |
| `passes_input` | see the live input and events after this scene: input runs from the top down through such scenes |
| `none` | keep updating and are drawn: an overlay |

The default, `hides_below | pauses_below`, is a full screen. Override `flags()` for the rest:

```cpp
// A pause menu: the game stays visible but stops.
neutrino::scene_flags flags() const override { return neutrino::scene_flags::pauses_below; }

// A loading spinner: what is below keeps running and drawing; its async work completes.
neutrino::scene_flags flags() const override { return neutrino::scene_flags::none; }
```

Each frame the engine draws from the topmost scene with `hides_below` upward, and updates from the
topmost scene with `pauses_below` upward.

```text
stack (top last)     flags                         updated?   drawn?   gets input?
──────────────────────────────────────────────────────────────────────────────────
main_menu            hides_below | pauses_below    no         no       no
gameplay             hides_below | pauses_below    yes        yes      no
spinner_overlay      none                          yes        yes      yes   ← top
```

`gameplay` is below an overlay, so it still runs — that is the entire point of an overlay. A
spinner shown over a loading screen must not freeze the work it is waiting on.

`passes_input` may change at any time; the other bits must not change while the scene is on the
stack, because push and pop each read `pauses_below`.

## Input runs from the top down

Updating and input are deliberately *not* the same question.

Live input and raw events go to the top scene, and on down through scenes with `passes_input`,
stopping at the first scene without it — and never reaching a scene paused by `pauses_below`.
Every other scene that is updated receives a **neutral input snapshot**: nothing held, nothing
pressed, pointer off-screen.

Without this, a background scene would react to clicks *through* a modal — the player dismisses a
dialog and the button behind it also fires. Because covered scenes get a genuinely empty snapshot
rather than being skipped, they need no "am I on top?" checks: code that steers from the pointer
simply sees `on_screen == false` and does nothing.

:::tip[This is why `pointer_state::on_screen` exists]

A scene that moves something to follow the pointer **must** check `on_screen` before acting on the
position. It is false both when the cursor is outside the window and when the scene is covered.
Acting on the position regardless snaps your object to a meaningless coordinate.
:::

## How scenes are drawn

A scene is a logical part of the game, and the engine draws each visible one as it is over
the scenes below, bottom to top. Its flags alone decide how: a scene with `hides_below` is
a plain copy, an overlay is blended over what is below. Code that draws today draws
unchanged, and a full screen costs nothing extra: the bottom full-screen scene draws
straight into the window.

- **An overlay is isolated.** It is drawn on its own, then blended over the scenes below. A
  translucent colour drawn with blend `none` (the default draw blend) is therefore blended
  over them, where drawing into the window used to overwrite them; to cover what is below,
  draw opaque colour.
- **Fades, shakes and transitions are the scene's layers' business**, not settings on the
  scene: a pause menu fades in by fading its layer, `style_of(m_menu).opacity = t`. See
  [Drawing, part 6](./drawing.md#6-layers-and-the-compositor).

## Lifecycle

| Hook | Fires when |
|---|---|
| `on_enter` | The scene has been added to the stack |
| `on_exit` | The scene is being removed |
| `on_pause` | A scene with `pauses_below` was pushed directly on top |
| `on_resume` | That scene was popped |
| `on_resize` | The scene becomes active, and whenever the render space changes |

### Pause and resume are for mode changes, not overlays

Pushing or popping a scene without `pauses_below` does not fire `on_pause` / `on_resume` on the
scene below.

This looks like an inconsistency and is a deliberate choice. Refresh logic naturally hangs off
`on_resume` — "I was away, re-sync my state." If dismissing a spinner fired `on_resume`, every
overlay dismissal would kick off a fresh background sync, including while the first one was still
in flight. An overlay is not a mode change; the scene below never stopped running.

### `on_resize` fires on activation too

You do not need to poll the render size. A scene is told its size when it becomes active and again
whenever the render space changes, so laying out in `on_resize` and caching the result is the
intended pattern.

## Transitions

```cpp
neutrino::push_scene(std::make_unique<pause_scene>());
neutrino::pop_scene();
neutrino::replace_scene(std::make_unique<game_over_scene>());
```

These are free functions because the application is effectively a singleton.

A scene can also be **pinned** above the stack — a console, an FPS counter:
`neutrino::pin_scene(std::make_unique<console_scene>())`, and `unpin_scene()` to drop it. Push,
pop and replace act on the stack below it; otherwise the pinned scene is the topmost one, drawn
and updated last.

**They are queued, not immediate.** Each posts a request to the SDL event queue, applied on the
next event pump. That is what makes them safe to call from inside a scene callback — including from
the scene being replaced. A scene can call `pop_scene()` on itself from `fixed_update` without
destroying the object it is currently executing in.

The consequence to remember: **the stack does not change on the line you call it.** Code after a
`push_scene()` still runs in the current scene, and the pushed scene's `on_enter` has not happened
yet.

## Failures are contained

No scene callback can take the process down. An exception escaping to SDL becomes
`SDL_APP_FAILURE` and terminates, so the engine catches at every boundary. What differs is how much
is *repaired* afterwards.

**A throwing `on_enter` under `push_scene` is rolled back.** The engine gives the scene `on_exit`
to clean up, drops it from the stack, resumes the scene below, and logs. The application stays
responsive on the scene that was already working.

**A throwing `fixed_update` or `render` drops that scene.** The error is logged and the top scene is
popped, so the next frame runs a known-good one. The scenes finished below a scene whose `render`
threw are still shown in that frame.

**At shutdown, every scene exits.** `on_exit` is guarded per scene and the scene is popped either
way, so one scene failing to exit cannot strand the rest. This matters more than it looks: the
application's `teardown()` runs immediately after and may release assets the scenes were using, so
a scene surviving shutdown would be holding freed resources.

:::warning[`replace_scene` and `pop_scene` are not rolled back]

The rollback above applies to `push_scene` only. On the other two paths an exception is caught one
level up — logged, so the application survives — but the stack is **not** repaired:

- `replace_scene` with a throwing `on_enter` leaves the half-initialised new scene on top.
- `pop_scene` with a throwing `on_exit` leaves the scene on the stack, because the throw happens
  before it is popped.

Both are recoverable in practice, but a scene whose `on_enter` can fail is safer to `push` than to
`replace`. If your `on_exit` can throw, catch inside it.
:::

## Worked shape

```cpp
class gameplay_scene : public neutrino::base_scene {
  public:
    void on_enter() override {
        m_level.load();                 // throwing here is safe: rolled back
    }

    void on_exit() override {
        m_level.clear();                // release what this scene owns, not what it borrowed
    }

    void fixed_update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override {
        if (in.pointer().on_screen) {   // false when covered or off-window
            m_player.aim_at(in.pointer().render);
        }
        m_world.step(dt);
    }

    void render() override {
        m_level.draw();
    }

    // handle_event() and flags() keep their defaults: continuous input is read
    // from the snapshot in fixed_update, and a gameplay scene is a full screen.
};
```

With the default application hooks, `fixed_update()` receives a snapshot containing both
held state and button edges, while `handle_event()` receives raw event payloads such as text
input. The application can replace the event route through `event()`.
See [Input](./input.md) for the hook boundaries and substep delivery rules.

## Common pitfalls

- **Expecting immediate stack changes:** Calling `push_scene()` or `pop_scene()` queues an event;
  it does *not* switch scenes on that exact line of code. Never write code that assumes the stack
  has changed before the current callback finishes.
- **Dropping `hides_below` on full screens:** If your gameplay or title scene's `flags()` leaves it
  out, the engine keeps drawing whatever scene is underneath — and without `pauses_below`, keeps
  updating it — burning CPU/GPU cycles.
- **Polling window dimensions in `render()`:** Laying out your HUD or UI every frame is wasteful. Override
  `on_resize(neutrino::dim size)` instead, which fires whenever the scene becomes active and whenever the render
  canvas dimensions change.

## Next

[Input](./input.md) — the frame snapshot, why edges and held state are different things, and the
two coordinate spaces a pointer position can be in.
