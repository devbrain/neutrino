# Input actions — rebindable bindings and global actions (P3.11, design)

Games ask for actions ("left", "fire", "fullscreen"), not keys. Each action is bound to
alternative inputs, can be rebound, and its bindings can be saved and loaded as text.
Modelled on brix's `hot_key` / `events_manager` (without its bugs: see the card). Usage is
documented on the website's input page (`website/docs/concepts/input.md`, §Actions); this
file records the design and how it is built.

## Public API

| Header | What |
|---|---|
| `input/binding.hh` | `binding` (a variant of `key`, `mouse_button`, `hotkey`, `mouse_click`, `gamepad_button`, made only from those and `pad_button`, which becomes `gamepad_button{0, b}`: not from a number, although sdlpp's `keycode` is one), `check()` of one binding or of a list as one virtual button, the text form (`to_string`, `binding_from_string`, `bindings_from_string`). |
| `input/action.hh` | `action`, `global_action`, `global_actions_suspended`, `action_record` and `action_records()`. |
| `input/action_bindings.hh` | `action_bindings`: named actions, `out << b` / `in >> b`. |
| `input/input_snapshot.hh` | `in.pressed(action)` etc., `in.capture_binding()`. |
| `input/hotkey.hh`, `mouse_click.hh` | `mods + mods`, `mods + mouse_button` (both orders); `==` on the matchers. |

## Decisions (agreed 2026-11-23 on the card, completed 2026-11-26)

- **One virtual button.** Held while any binding is held; pressed when it goes from none held
  to some held (a tap counts); released when it goes to none held. No memory: whether a binding
  was down before the frame follows from its own state (`held && !pressed`, or `released &&
  !pressed`).
- **RAII records.** An action is a handle to a record; the record lives as long as the object.
  Movable, not copyable. Its constructors are explicit (review, 2026-11-26): an implicit one let
  a key, a pad button or a keycode convert to an action, which made `in.pressed(pad_button::south)`
  ambiguous and turned `'x'` into a registered action. Default-constructed: no record (not listed) until `rebind()`;
  `set_description()` / `disable()` on it make a record that is not listed yet. Checking one
  never given bindings answers false and logs a warning, once per object; `rebind({})` is the
  deliberate, silent "unbound". The first bindings given are the defaults.
- **A description may come first** in the constructor: `action left{"Move left", key::left}`.
  With a description only, the action still has to be given bindings.
- **Global actions** fire once on the press, checked once per frame before the fixed steps
  (revised 2026-11-26, from "each tick": a frame that runs no step lost the press), and take that
  press out of the scenes' snapshot: the edges of the bindings that fired. Held stays, so the
  scenes see the key held and then released; the raw events still reach `handle_event()`. Registration order decides between two on one input;
  a disabled one is skipped. A callback runs outside the table's lock, may add or remove
  records (a removed one is skipped, a new one is checked from the next tick), and is contained
  if it throws. No scene is current while it runs. `global_actions_suspended` is a process-wide
  counted scope.
- **Scene-held global actions** fire while they live, even under a pausing scene (option A):
  nothing records an owner. A scene that wants otherwise disables it in `on_pause()`.
- **Rebinding screens:** `action_records()` returns handles by id (`action_record`), asked on
  every call, valid across frames; a gone action answers empty and rebinding does nothing.
  The listed records are those bound (and, for a global action, with a callback), in handle
  order, which is registration order.
- **Capture:** `in.capture_binding()`. Keys first (lowest scancode), then mouse buttons, then
  pads; modifier keys alone are skipped; modifiers are recorded side-agnostic (left Ctrl gives
  `mods::ctrl`); no modifier gives the plain key or button.
- **Queries outside an application** use the pending table, as binding and unbinding do, so a
  scene's `fixed_update` can be tested with a synthetic snapshot and no application.
- **Text form.** `", "` separates bindings; modifiers come first, each followed by `+`:
  `Ctrl`, `Shift`, `Alt`, `GUI` (either side), `LCtrl`, `RCtrl`, ... (one side), `None` (strict,
  no modifier: `None+Space` is `hotkey{key::space}`, `Space` is the plain key). Keys go by
  scancode names (layout-independent; the engine's, see below); a keycode hotkey is written as
  its current scancode. Mouse: `Mouse Left` .. `Mouse X2`. Pads: `Pad South`
  (slot 0), `Pad 2 South` (slot 1), with names of our own (`South`, `Left Shoulder`, `DPad Up`,
  `Misc 1`, `Right Paddle 2`, `Touchpad`, ...); pads take no modifiers. Case and runs of blanks
  are ignored when reading. A part before a `+` is a modifier only if it is one of those names,
  so `Keypad +` and `Ctrl+Keypad +` read whole; no key name contains `", "`, so `Ctrl+,, Space`
  splits at the second comma.
- **Key names are the engine's** (revised 2026-11-26, from "SDL scancode names"): SDL renames
  keys per platform when video starts (macOS "Left Option", "Left Command"; Windows "Left
  Windows"; Windows, X11 and Wayland call Application "Menu"), so its current names made a file
  depend on the platform and on when it was written. `src/neutrino/input/key_names.hh` fixes
  SDL 3's default names (generated from `SDL_GetScancodeName` before video starts), with Return2
  as `Return 2` (SDL calls it "Return" too). Reading also takes the platform names above (but
  "Menu" stays the Menu key) and SDL's current names.
- **Everything written reads back:** unnamed keys `Scancode <n>`, mouse buttons `Mouse <n>`, pad
  buttons `Pad Button <n>`, any pad slot number. A keycode that maps to no key under the current
  layout, or that SDL turns into a scancode past its table, is written as `Scancode 0` (no key)
  and logged when saved.
- **Saving:** `action_bindings` with `<<` / `>>` on any stream (revised 2026-11-26, replacing a
  `bindings_file` with paths): the game chooses the stream. One `name = bindings` line each;
  the name ends at the first `=` (the `=` key is in the bindings). Reading takes the whole
  stream, in any line order; a bad or missing line keeps the old bindings and is logged; a line
  for a name not listed is logged and skipped; blank lines and `#` lines are skipped; nothing
  throws. Loaded bindings are not the defaults (review, 2026-11-26: a description-only action
  loaded from a file took the file's bindings as its defaults), and an action never given
  bindings is not written. Names are checked when the list is built (ENFORCE: not empty, no
  `=`, `#`, line break or blank at either end, listed once).

## How it is built

- `src/neutrino/input/action_tables.{hh,cc}`: `detail::action_table`, a `std::map` from
  `action_id` to `action_entry` (bindings, defaults, bound, description, enabled, global, the
  callback as `shared_ptr<const function>`), under one mutex. `std::map` keeps handle order and
  moves nodes in `merge()` without copying.
- The application's table is `application_scoped<detail::action_table>`; while no application
  runs, records live in `get_pending_actions_table()` (made once, never destroyed: actions may
  die in static destruction).
- `scenes_manager::setup_actions()` (on_ready, before `ready()`) calls
  `detail::adopt_pending_actions()`: the pending records move into the application's table,
  which becomes current. `scenes_manager::release_actions()` (on_quit and the destructor's
  fallback, just before the application scope closes) calls `detail::return_actions()`: they
  move back, and the pending table is current again. Moves, not copies, so a record never comes
  back after its action is gone.
- **The lock** (review, 2026-11-26): `detail::actions_mutex()` is held while an action picks the
  current table and uses it (`with_current_table()`), and while records move between tables. So
  a loading thread never adds to a table that was just emptied, and never uses the application's
  table after `return_actions()`, before the scope destroys it. Each table also has its own
  mutex, taken second. Firing runs on the main thread, the only one that moves records, without
  the outer lock, so a callback may make or destroy actions.
- `application::update_scene()` calls `scenes_manager::fire_global_actions()` on the frame's
  snapshot before the steps. It returns a copy without the taken edges only when something
  fired, so the scenes otherwise get the snapshot object itself.
- `binding.cc`: `check()` and the text form, through sdlpp's scancode-name wrappers.

## Tests

`test/input/test_binding.cc` (conversions, the virtual button, the text form including every
SDL-named key, capture), `test_action_table.cc` (records, defaults, listing, merge, global
firing: order, edges, disabled, suspended, throwing, add/remove during the walk),
`test_action.cc` (actions, explicit construction, global actions through real frames including
one with no step and as application members, records for rebinding screens, `action_bindings`
on string and file streams and its defaults, lifetime across applications, actions made on a
thread while applications start and stop).
