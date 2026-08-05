---
sidebar_position: 2
title: Scenes
description: The scene stack, lifecycle hooks, opaque versus overlay, and how transitions are applied.
---

# Scenes

A scene is one screen of your game: a menu, the gameplay, a pause overlay. The engine keeps them in
a **stack**, and each frame it dispatches updates and drawing to some slice of that stack.

You implement [`base_scene`](pathname:///neutrino/api/). Four members are pure virtual, so the
compiler makes you supply them:

```cpp
class my_scene : public neutrino::base_scene {
  public:
    void fixed_update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render() override;
    void handle_action(const sdlpp::event& ev) override;
    [[nodiscard]] bool is_opaque() const override { return true; }
};
```

The lifecycle hooks — `on_enter`, `on_exit`, `on_pause`, `on_resume`, `on_resize` — all have
do-nothing defaults, so override only the ones you need.

## Opaque or overlay

`is_opaque()` is the single most consequential thing a scene declares. It answers "does this scene
completely cover the screen?", and the engine uses that answer to decide how much of the stack
stays alive.

- **Opaque** (`true`) — a full screen. Nothing below it is drawn.
- **Overlay** (`false`) — a pause menu, a modal dialog, a loading spinner. The scene below keeps
  drawing *and* keeps updating.

Each frame the engine scans down from the top of the stack to the **first opaque scene**, and
dispatches from there upward.

```text
stack (top last)     is_opaque   updated?   rendered?   gets input?
─────────────────────────────────────────────────────────────────────
main_menu            true        no         no          no
gameplay             true        yes        yes         no
pause_overlay        false       yes        yes         yes   ← top
```

`gameplay` is below an overlay, so it still runs — that is the entire point of an overlay. A
spinner shown over a loading screen must not freeze the work it is waiting on.

## Only the top scene receives input

Updating and input are deliberately *not* the same question.

Covered scenes are updated, but they receive a **neutral input snapshot**: nothing held, nothing
pressed, pointer off-screen. Raw SDL events go to the top scene alone.

Without this, a background scene would react to clicks *through* a modal — the player dismisses a
dialog and the button behind it also fires. Because covered scenes get a genuinely empty snapshot
rather than being skipped, they need no "am I on top?" checks: code that steers from the pointer
simply sees `on_screen == false` and does nothing.

:::tip This is why `pointer_state::on_screen` exists

A scene that moves something to follow the pointer **must** check `on_screen` before acting on the
position. It is false both when the cursor is outside the window and when the scene is covered.
Acting on the position regardless snaps your object to a meaningless coordinate.
:::

## Lifecycle

| Hook | Fires when |
|---|---|
| `on_enter` | The scene has been added to the stack |
| `on_exit` | The scene is being removed |
| `on_pause` | An **opaque** scene was pushed on top |
| `on_resume` | The opaque scene above was popped |
| `on_resize` | The scene becomes active, and whenever the render space changes |

### Pause and resume are for mode changes, not overlays

Pushing or popping an **overlay** does not fire `on_pause` / `on_resume` on the scene below.

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

**A throwing `fixed_update` drops that scene.** The error is logged and the top scene is popped, so
the next frame runs a known-good one.

**At shutdown, every scene exits.** `on_exit` is guarded per scene and the scene is popped either
way, so one scene failing to exit cannot strand the rest. This matters more than it looks: the
application's `teardown()` runs immediately after and may release assets the scenes were using, so
a scene surviving shutdown would be holding freed resources.

:::warning `replace_scene` and `pop_scene` are not rolled back

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

    void handle_action(const sdlpp::event&) override {
        // Discrete one-shot events. Continuous input belongs in fixed_update,
        // read from the snapshot.
    }

    [[nodiscard]] bool is_opaque() const override { return true; }
};
```

Note the division: **continuous** input (where the pointer is, whether a key is down) is polled
from the snapshot in `fixed_update`; **discrete** one-shot events arrive in `handle_action`. Mixing
them is the most common source of input bugs, and [Input](./input.md) explains why.

## Next

[Input](./input.md) — the frame snapshot, why edges and held state are different things, and the
two coordinate spaces a pointer position can be in.
