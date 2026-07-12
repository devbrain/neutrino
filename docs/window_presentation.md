# Window Presentation — resize, fullscreen, HiDPI (design)

Runtime handling of a changing render surface: window resize, fullscreen toggle, and
HiDPI. These are one problem — *the drawable's pixel size and DPI scale can change at
runtime, and input arrives in window points while rendering happens in a different
space.* Solve "the render surface changed" + "map input into render space" once, and all
three fall out (fullscreen is just a resize; HiDPI is just points ≠ pixels).

## 1. Where it stands

- Windows are resizable by default (`application_config::flags`). No fullscreen/HiDPI config.
- Events already reach scenes: `application::handle_event` forwards every event to the
  active scene's `handle_action` (application.cc:189) — but there is no structured hook.
- Size is **polled**: `map_viewer` reads `get_renderer().get_output_size<dim>()` every frame
  as its viewport (map_viewer.cc:172). No logical presentation is set (native-pixel render).
- Input is in **window points**; render is in **pixels**. `map_viewer`'s zoom-to-cursor treats
  mouse coords as viewport pixels — correct only because `high_pixel_density` isn't set.
- sdlpp exposes the raw `SDL_Renderer*`/`SDL_Window*` (`.get()`) and `window::set_fullscreen`,
  but does **not** wrap `SDL_SetRenderLogicalPresentation` / `SDL_RenderCoordinatesFromWindow` —
  neutrino wraps those raw SDL3 calls.

## 2. The two presentation modes (both supported)

The engine offers both; **reflow is the default**.

- **Reflow** — render at the native drawable pixel size; layout recomputes on resize; a
  bigger window shows *more* content; crisp on HiDPI. For editors / viewers / map tools
  (`map_viewer`). `logical_size` unset.
- **Logical** — a fixed design resolution scaled to the drawable by SDL
  (`SDL_SetRenderLogicalPresentation`) with a mode; a bigger window shows the *same* content,
  scaled; HiDPI is transparent (you always draw in logical coords). For fixed-resolution /
  pixel-art games (`sprite_demo` at 640×360, integer-scaled). `logical_size` set.

## 3. Key models (locked)

### Config

```cpp
enum class scale_mode : std::uint8_t {   // -> SDL_RendererLogicalPresentation
    letterbox,       // preserve aspect, black bars
    integer_scale,   // preserve aspect, integer multiples only (crisp pixel art)
    overscan,        // preserve aspect, fill + crop
    stretch,         // fill, ignore aspect
};

struct application_config {
    // …existing: title, width, height, flags, target_fps, vsync…
    std::optional<dim> logical_size;              // unset => reflow (default); set => logical
    scale_mode         scale{scale_mode::letterbox}; // used only in logical mode
    bool               high_pixel_density{false}; // opt into native-resolution (crisp) rendering
    bool               fullscreen{false};
};
```

### The render space + input boundary (the two primitives)

```cpp
// video/globals.hh (next to get_renderer/get_window)

// The size of the coordinate space scenes draw in: the logical size when logical
// presentation is active, else the drawable pixel size. This is what a scene's viewport
// and camera use. Derived from the renderer state -- no caching.
[[nodiscard]] dim render_size();

// Map a window point (as delivered in events -- mouse, touch) into render coordinates.
// Backed by SDL_RenderCoordinatesFromWindow, so ONE call accounts for BOTH the DPI scale
// AND logical-presentation letterboxing. All input coordinates must go through this.
[[nodiscard]] sdlpp::point<float> to_render_coords(sdlpp::point<float> window_pt);
[[nodiscard]] sdlpp::point<float> to_window_coords(sdlpp::point<float> render_pt); // inverse
```

`to_render_coords` is the elegant bit: it makes reflow/logical, HiDPI on/off, and
fullscreen letterboxing all "just work" at the input edge — a scene never scales coords
by hand.

### Resize hook

```cpp
// base_scene: default no-op
virtual void on_resize(dim render_size) {}
```

Fired with the current `render_size()` when a scene becomes active (enter/resume) and when
the render space changes. It only *changes* in reflow mode (logical mode's render space is
the fixed design resolution — SDL scales it, so `on_resize` fires once at entry). Replaces
both "poll `get_output_size` every frame" and "parse `window_event` in `handle_action`".

### Fullscreen

```cpp
// application
void set_fullscreen(bool on);
void toggle_fullscreen();
[[nodiscard]] bool is_fullscreen() const;
```

Wraps `get_window().set_fullscreen(...)`. It just changes the drawable → a
`window_pixel_size_changed` event → `on_resize` fires. Config `fullscreen` applied at ready.
A default Alt+Enter / F11 binding is left to the game (the engine provides the API).

## 4. Design decisions (agreed)

- **Support both modes**, reflow default; `logical_size` (set/unset) selects.
- **Single input boundary** — all input coords flow through `to_render_coords`
  (`SDL_RenderCoordinatesFromWindow`); no per-scene scaling.
- **Structured `on_resize(dim)`** hook, not frame polling; `render_size()` is the queryable
  render coordinate-space size.
- **`render_size()` is derived** from the renderer's logical/output state, not cached, so it
  can't drift.
- **Fullscreen is a resize** — one code path; the engine exposes the toggle, the game binds it.
- **On_resize fires on the active (top) scene**; a lower scene gets it when it resumes.
  Overlay scenes that need it while paused can propagate themselves.

## 5. Phase plan

**Status: implemented (W1–W5).** Config fields, `render_size()`, `to_render_coords`/
`to_window_coords`, `base_scene::on_resize`, the fullscreen API, and both example
migrations are in tree; `test/video/test_window_presentation.cc` covers the headless-
testable parts (render_size in both modes, the input-mapping identity + round-trip, the
on_resize activation fire, and the resize-event dedup/fire path). Fullscreen and true-HiDPI
visuals remain manual (need a real display).

### Phase W1 — presentation config + `render_size()` + logical setup
Config fields (`logical_size`, `scale`, `high_pixel_density`, `fullscreen`); apply at ready
(`SDL_SetRenderLogicalPresentation` when logical; `high_pixel_density` into the window flags;
fullscreen). `render_size()` free function (logical size if logical presentation active, else
output size).
- **Tests:** `render_size()` returns the output size in reflow and the logical size in logical
  mode (set logical presentation on a headless renderer and check).

### Phase W2 — `to_render_coords` / `to_window_coords`
The input-boundary helpers over `SDL_RenderCoordinatesFromWindow`/`…ToWindow`.
- **Tests:** identity in reflow with no DPI; a window point maps to the expected logical point
  under a known logical presentation (headless renderer with logical presentation set);
  round-trip `to_window_coords(to_render_coords(p)) ≈ p`.

### Phase W3 — `on_resize(dim)` hook + wiring
`base_scene::on_resize`; the app fires it on scene activation and on
`window_pixel_size_changed` (deduped by size). Scenes stop polling.
- **Tests:** a probe scene records `on_resize` calls; a simulated `window_pixel_size_changed`
  fires it with the pixel size; logical mode fires it once at entry (constant render space);
  no spurious fire when the size is unchanged.

### Phase W4 — fullscreen API
`set_fullscreen`/`toggle_fullscreen`/`is_fullscreen` + config application at ready.
- **Tests:** limited headlessly (real fullscreen needs a display); unit-test the config→call
  wiring and that `is_fullscreen` reflects the flag. Manual check on a real display.

### Phase W5 — migrate the examples (the payoff)
`map_viewer`: reflow + `high_pixel_density`, replace the per-frame `get_output_size` poll with
`on_resize`/`render_size()`, and route the wheel cursor through `to_render_coords` (fixes
zoom-to-cursor on HiDPI). `sprite_demo`: `logical_size = {640,360}`, `integer_scale` — the
fixed-resolution demonstration.
- **Tests:** examples build + run headless; manual HiDPI/resize/fullscreen check.

### Build order
    W1 → W2 → W3 → W4 → W5

W1/W2 are the primitives; W3 wires the signal; W4 is the toggle; W5 proves both modes on the
two examples. Testing note: `render_size`, `to_render_coords` math, and `on_resize` wiring
(with simulated events) are unit-testable headlessly; fullscreen and true-HiDPI visuals need a
real display and are manual.
