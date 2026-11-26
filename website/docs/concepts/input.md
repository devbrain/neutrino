---
sidebar_position: 3
title: Input
description: Per-frame input snapshots, application and scene hooks, button edges, and pointer coordinate spaces.
---

# Input

The application captures one `input_snapshot` per frame. It contains pointer position,
button and key states, wheel movement, and gamepad state. Use it for movement and aiming,
and use its `pressed` and `released` edges for one-shot actions.

Raw SDL events carry individual notifications such as text input and window events.
The engine also uses input events to maintain the state copied into the snapshot.
Move an object during an update using held state; moving it only on key-down events
would make its speed depend on OS key repeat.

## Where input is delivered

| Hook | When it runs | Default behavior |
|---|---|---|
| `base_scene::fixed_update(sim_duration dt, const input_snapshot& input)` | Zero or more times per frame, from the engine's fixed-step scheduler | Your scene implements simulation using a constant time step. |
| `simplex::application::on_update(float dt, const input_snapshot& input)` | Zero or more times per frame, on the same fixed tick | Your Simplex callback receives the constant tick length and the snapshot; return `false` to stop. |

The final application callbacks keep common input bookkeeping in place. After input
tracking, `handle_event()` detects render-space size changes and calls `resize_scene(size)`
when needed, then calls `event(e)`:

- The default `event(e)` dispatches scene transitions and forwards other events to the
  top scene's `handle_event(e)`, and on down through scenes with `passes_input`. Overriding it replaces that dispatch; call
  `application::event(e)` explicitly if you also want the default behavior.
- The default `resize_scene(size)` calls the active scene's `on_resize(size)`. An override
  replaces this notification. Scene activation delivers an initial size separately;
  an application without scenes can query `render_size()` in `ready()`.
- The default event and resize hooks contain scene exceptions. Exceptions from custom
  overrides propagate to the application entry point.

Simplex replaces the scene update, rendering, event, and resize hooks. It forwards the
snapshot to `on_update()` and lets SDL presentation scale its fixed canvas. It does not
dispatch raw events to scenes. Ignored scene-transition requests remain owned until shutdown.

```mermaid
flowchart TD
    Pump["SDL events"] --> Track["Track keyboard, mouse and gamepad state"]
    Track --> Size{"Render-space size changed?"}
    Size -- Yes --> Resize["resize_scene(size)"]
    Resize --> Event["event(e)"]
    Size -- No --> Event
    Resize -. "default" .-> SceneResize["Active scene: on_resize(size)"]
    Event -. "default" .-> Actions["Scene transitions / top scene: handle_event(e)"]
    Track --> Sample["Once per frame: capture input_snapshot"]
    Sample --> Update["Fixed-step scheduler"]
    Update --> Steps["0..N scene fixed_update calls<br/>Only the first substep receives edges"]
    Steps -- "Simplex" --> Simplex["on_update(tick_dt, input)<br/>per fixed_update"]
```

The scheduler is not a hook: every application runs it. Overriding `event()` or
`resize_scene()` replaces only that dispatch route. See [Scenes](./scenes.md) for the default
stack behavior.

For example, this complete Simplex application checks Escape once per frame:

```cpp
#include <neutrino/simplex/entry_point.hh>

class input_demo final : public neutrino::simplex::application {
public:
    input_demo() : application(320, 200, "Input demo", 3.0f) {}

    bool on_update(float, const neutrino::input_snapshot& input) override {
        return !input.pressed(neutrino::key::escape);
    }
};

SIMPLEX_MAIN(input_demo)
```

## The snapshot

In a scene, use the snapshot passed to each simulation step:

```cpp
void my_scene::fixed_update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    if (in.pointer().on_screen) {
        m_cursor = in.pointer().render;
    }
    if (in.pressed(neutrino::mouse_button::left)) {
        fire();                                   // only on a delivered press edge
    }
    if (in.held(neutrino::mouse_button::right)) {
        aim();                                    // every step the button is down
    }
}
```

`in.mouse(button)` returns the button's whole `button_state` (`pressed`, `released`, `held`)
when you need more than one of them.

The snapshot stores input state by value. You can construct one in a test and drive a
scene with synthetic input:

```cpp
const neutrino::input_snapshot in{
    neutrino::pointer_state{{100.0f, 50.0f}, {50.0f, 25.0f}, true},
    neutrino::button_state{true, false, true},   // left: pressed this frame, and held
    {}, {}
};
scene.fixed_update(neutrino::sim_duration{1.0f / 60.0f}, in);
```

The reference passed to an update callback is valid during that call. A saved copy records
past input; its edges do not clear themselves. Do not reuse it as fresh input in later frames.
Keycode queries also have a [keyboard-layout dependency](#scancodes-keycodes-and-modifiers).

## Edges versus held

Every button reports three booleans:

| Field | Meaning in the frame snapshot |
|---|---|
| `pressed` | A transition to down was recorded for this frame. |
| `released` | A transition to up was recorded for this frame. |
| `held` | The button is currently down. |

`pressed` and `released` are **edges**; `held` is a **state**. Use `pressed` for one-shot
actions such as fire, jump, and confirm. Use `held` for continuous actions such as walking
or charging. A snapshot describes state and edges, not a queue counting every physical click.

### Input across simulation substeps

A frame samples input once, but [may run several fixed substeps](./the-frame.md). Passing
the full snapshot to every substep would repeat the same one-shot action within that frame.
The default scene scheduler therefore delivers edges to **at most one substep**:

- **Substep 0**, if it runs, gets the full snapshot, including edges and wheel movement.
- **Later substeps** get `input.without_edges()`: `pressed`, `released`, and wheel movement
  are cleared; held state, axes, modifiers, and pointer position remain available.

This applies to the snapshot supplied to `fixed_update()`, which is the engine's input API:
there is no separate live-polling call that could report the same `pressed` edge in several
substeps.

:::caution[Frames with no simulation steps]

The current scene scheduler does **not** latch edges for a future simulation step. If a
frame runs zero substeps, its press/release edges and wheel movement are cleared at frame
completion without reaching `fixed_update()`.

This also affects a press that remains held: the next stepping frame sees `held == true`
but `pressed == false`. It does not require a short click; for example, a 144 Hz display
with a 120 Hz simulation can have frames with no simulation step.

Simplex's `on_update()` runs on the same ticks and follows the same rule. Guaranteed delivery to fixed-step logic would require buffering actions until
a simulation step consumes them; the default scheduler currently does not do this.
:::

## Pointer position: two spaces

```cpp
struct pointer_state {
    window_pos window;    // window coordinates (points), as SDL reports mouse positions
    render_pos render;    // coordinates mapped into the renderer's view
    bool on_screen;
};
```

Window coordinates are not necessarily drawable pixels. A HiDPI window can have more
drawable pixels than window points, and logical presentation can add scaling and letterboxing.
The distinct `window_pos` and `render_pos` types prevent accidentally mixing these spaces.
Use `pointer().render` for drawing and hit testing in render space.

For diagrams, worked HiDPI examples, and button, viewport, and minimap use cases, see
[window and renderer coordinates](./window-and-renderer-coordinates.md).

If you need to convert explicitly, the mapping is available in both directions:

```cpp
const neutrino::render_pos r = neutrino::to_render_coords(w);
const neutrino::window_pos w2 = neutrino::to_window_coords(r);
```

### Always check `on_screen`

The engine samples the live pointer position from SDL, including on the first frame, and
uses mouse focus to determine availability. `on_screen` is false when there is no mouse
focus and in the neutral snapshot supplied to a [covered scene](./scenes.md#input-runs-from-the-top-down).
It does not depend on receiving a first mouse-motion event.

```cpp
if (in.pointer().on_screen) {
    model.set_target(static_cast<int>(in.pointer().render.x));
}
// Otherwise, keep the last target.
```

Mouse focus does not imply that the pointer is inside the logical canvas. Over letterbox
bars, the mapped render position can lie outside the canvas; check your content bounds
as well when deciding whether a click should interact with it.

## Keyboard, Gamepad, and Query Matchers

Query keys, mouse buttons, wheel movement, and gamepad buttons or axes from the snapshot:

```cpp
if (in.pressed(neutrino::key::space)) { fire(); }      // edge
if (in.held(neutrino::key::a))        { move_left(); } // state
if (in.released(neutrino::mouse_button::left)) { drop(); }

// Gamepad slot 0 is Player 1.
if (in.gamepad_button_state(0, neutrino::pad_button::south).pressed) { jump(); }
const float move_x = in.gamepad_axis(0, neutrino::pad_axis::leftx);
const float trigger = in.gamepad_axis(0, neutrino::pad_axis::right_trigger);
```

Triggers are **axes**, not `gamepad_button` values. A trigger threshold tests a level;
detecting a one-shot threshold crossing requires comparing against previous state.

### Expressive matchers: `hotkey`, `mouse_click`, `gamepad_button`

Matchers describe which key or button to query and, for keyboard and mouse shortcuts,
which modifiers must be present:

```cpp
const neutrino::hotkey jump_key{neutrino::key::space};
const neutrino::hotkey save_key{neutrino::modifier::ctrl, neutrino::key::s};
const neutrino::mouse_click ctrl_click{neutrino::modifier::ctrl, neutrino::mouse_button::left};
const neutrino::gamepad_button shoot_button{neutrino::pad_button::right_shoulder};

if (in.pressed(jump_key))     { jump(); }          // equivalent to jump_key.pressed(in)
if (in.held(jump_key))        { charge_jump(); }   // equivalent to jump_key.held(in)
if (in.pressed(save_key))     { save(); }
if (in.pressed(ctrl_click))   { select(); }
if (in.pressed(shoot_button)) { fire(); }
```

A modifier and a key also combine with `+` into a `hotkey`, in either order, with a
scancode or a keycode; a modifier and a mouse button into a `mouse_click`. Several modifiers
chain with `+` too, left to right. (`|` also combines modifiers, but `+` binds tighter than
`|`, so a `|` group needs parentheses.)

```cpp
const neutrino::hotkey save_key = neutrino::mods::ctrl + neutrino::key::s;
const neutrino::hotkey save_as  = neutrino::mods::ctrl + neutrino::mods::shift + neutrino::key::s;
const neutrino::mouse_click add = neutrino::mods::shift + neutrino::mouse_button::left;
```

### Scancodes, keycodes, and modifiers

A **scancode** identifies a physical key position, useful for layout-independent movement.
A **keycode** identifies a logical key in the current keyboard layout, useful for a
shortcut the player thinks of as a letter. Both are accepted by `hotkey`.

Scancode queries and mouse/gamepad matchers read only stored snapshot state. Keycode
queries, including keycode-based `hotkey` matchers, call SDL to resolve the keycode to a
scancode **at query time**. The snapshot does not capture that layout mapping. Use scancodes
or resolved game actions for tests and replay data that must be independent of SDL's current layout.

Modifier matching is strict:

- `hotkey{scancode::space}` requires no modifiers. Direct `in.pressed(scancode::space)`
  ignores modifiers. The same holds for the mouse: `mouse_click{mouse_button::left}`
  requires no modifiers, and `in.pressed(mouse_button::left)` ignores them.
- `modifier::ctrl` accepts either Ctrl key, but rejects additional Shift, Alt, or GUI modifiers.
- `modifier::lctrl` requires left Ctrl and rejects right Ctrl, including when both are held.

:::tip[Testing and replay]

Construct synthetic snapshots with `set_key()`, `set_mouse()`, and the gamepad setters.
For gamepad tests, also call `set_gamepad_connected(slot, true)`; queries treat disconnected
slots as neutral. Scancode, mouse, and gamepad queries need no running SDL application.

The default scene stack supplies a neutral snapshot to covered scenes that continue updating.
This isolation and the per-substep edge stripping apply to the snapshots it dispatches,
not to live polling APIs.

Recording snapshots or resolved actions can support replay, but does not by itself guarantee
identical results. Reproduction also needs the same initial state, simulation-step sequence,
randomness, and other external inputs, together with deterministic simulation behavior.
:::

## Actions

An `action` names what the game asks for ("left", "fire") and binds it to alternative
inputs, which the player can rebind and the game can save and load. Check it against the
snapshot as you would a key:

```cpp
#include <neutrino/input/input.hh>

class game_scene final : public neutrino::base_scene {
    neutrino::action m_left{"Move left", neutrino::key::left, neutrino::key::a,
                            neutrino::pad_button::dpad_left};
    neutrino::action m_fire{"Fire", neutrino::key::space, neutrino::mouse_button::left};

    void fixed_update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override {
        if (m_left.held(in)) { move_left(dt); }
        if (in.pressed(m_fire)) { fire(); }     // the same as m_fire.pressed(in)
    }
};
```

**Bindings.** A plain `key`, `mouse_button` or `pad_button` ignores modifiers, as
`in.pressed(key)` does; `hotkey`, `mouse_click` and `gamepad_button` match as described above
(`mods::ctrl + key::s` only while Ctrl, and no other modifier, is held). A plain `pad_button`
is that button on slot 0.

**One virtual button.** An action is held while any of its bindings is held; pressed when it
goes from none held to some held (a tap within one frame counts); released when it goes to
none held. Holding Left and then pressing A gives no second press. A strict binding follows
the modifiers held now, so letting go of Ctrl while S is held ends `Ctrl+S` without a
released edge.

**Lifetime.** An action is a handle to a record in the application's actions table, and the
record lives exactly as long as the object: a scene member while the scene lives, an
application member for the whole run. Actions made before the application is ready (members
built in its constructor) wait in a pending table and join it at startup; those alive at
shutdown go back, so they serve the next application too. Actions can be made on a loading
thread; check them on the main thread. Actions are movable, not copyable.

**Rebinding and state.**

```cpp
m_fire.rebind({neutrino::mods::ctrl + neutrino::key::f, neutrino::mouse_button::left});
m_fire.rebind({});                                // deliberately unbound: answers false, silently
bool ok = m_fire.rebind("Ctrl+F, Mouse Left");    // the text form; false (and logged) if malformed
m_fire.disable();                                 // answers false until enable()
```

A default-constructed action is empty and not in the table until `rebind()` gives it
bindings; checking it answers false and logs a warning, once per object. The first bindings
an action is given are kept as its defaults.

### Global actions

A `global_action` fires on its own, before the scenes see the input, so it works under a
pausing scene or while a console has the keyboard:

```cpp
class my_game final : public neutrino::application {
    neutrino::global_action m_fullscreen{
        "Fullscreen",
        {neutrino::mods::alt + neutrino::key::return_key, neutrino::mods::alt + neutrino::key::kp_enter},
        [] { neutrino::toggle_fullscreen(); }};
    // ...
};
```

It fires once on the press, checked once per frame before the fixed steps (so a frame that
runs no step does not lose it), and takes that press out of the scenes' snapshot: the edges
of the bindings that fired. Held state stays, so the scenes see the key held and then
released, and the raw key events still reach `handle_event()`. Holding the key does not
repeat it. If two global actions
share an input, the one registered first takes it; a disabled one is skipped. No scene is
current while the callback runs, so drawing fails there; `toggle_fullscreen()`,
`pin_scene()` and `screenshot()` work. A throwing callback is logged and contained.
`rebind()` keeps the callback.

A `global_actions_suspended` object suspends every global action while it lives (they nest),
for example while a rebinding screen captures Alt+Enter.

### Rebinding screens

`neutrino::action_records()` lists the live actions and global actions in registration order:
those of the application and of the scenes alive now. Each `action_record` gives the
`description()`, `is_global()`, `bindings()` and `default_bindings()`, and rebinds through
`rebind()` or `reset()`. A record stays valid across frames; once its action is gone, it
answers empty and rebinding does nothing.

`in.capture_binding()` gives the binding that went down this frame, with the modifiers held:
Ctrl+S gives `mods::ctrl + key::s` (either side), S alone the plain `key::s`. A modifier
pressed alone is skipped, so the screen waits for the real key; cancelling with Esc is the
screen's job.

### Text form and saving

Bindings have a text form: `"Ctrl+F, Space, Pad South, Mouse Left"`. Keys go by scancode
names, so the text does not depend on the keyboard layout (a keycode hotkey is written as the
scancode it maps to). The names are the engine's own: SDL's default ones, fixed, so a file
written on one platform reads on another (SDL renames some keys per platform, such as "Left
Option" on macOS; those names are read too). `None+Space` is the strict hotkey with no
modifier, `Space` the plain key; `LCtrl+S` asks for the left Ctrl; `Pad 2 South` is slot 1.
Case and extra blanks do not matter when reading. `neutrino::to_string()`,
`binding_from_string()` and `bindings_from_string()` convert; a malformed text gives nothing.

`action_bindings` names the actions once and saves and loads them on any stream:

```cpp
const neutrino::action_bindings bindings{
    {"left", m_left}, {"fire", m_fire}, {"fullscreen", m_fullscreen}};

std::ofstream out(path);   // e.g. under sdlpp's get_pref_path
out << bindings;           // "left = Left, A, Pad DPad Left" ...

std::ifstream in(path);
in >> bindings;            // rebinds each named action, in any line order
```

A bad or missing line keeps the old bindings and is logged; a line for a name not listed is
logged and skipped; blank lines and `#` comments are skipped. Loaded bindings do not become
the defaults, so a rebinding screen's reset still goes back to the game's. An action never
given bindings is not written.

## Common pitfalls

- **Ignoring pointer availability or content bounds:** Check `on_screen`, then check canvas
  bounds when clicks over letterbox bars should be ignored.
- **Using held state for a one-shot action:** It repeats on every simulation step while down.
- **Polling live state inside a scene update:** It bypasses substep edge stripping and scene
  input isolation. Use the supplied snapshot.
- **Moving on key-down events:** Movement should use held state during `fixed_update()` or
  a per-frame application update, rather than advancing only when key-repeat events arrive.
- **Expecting every press to reach a fixed step:** Account for the current zero-substep
  limitation described above.

## Next

See [The frame](./the-frame.md) for fixed-step scheduling, or follow
[Step 3: Moving with the keyboard](../tutorial/step-03-moving-with-keyboard.md) for a scene-based example.
