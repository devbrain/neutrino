# Refactoring backlog

Recurring patterns / duplication found in a codebase-wide review (five parallel sweeps:
physics, TMX loader, audio/scene/input, video, tests). Items confirmed by more than one
sweep are highest-confidence. Ordered by value ÷ risk. Check off as done.

## Tier 1 — biggest win, lowest risk

- [x] **A. Service-locator accessor layer.** ✅ New `src/…/services/service_access.hh` with
  `require_sprites_manager`/`maybe_`, `require_texture_registry`/`maybe_`, `require_sound_system`/
  `maybe_`, `maybe_application`, `maybe_gamepads`. Migrated all sprite manager/registry sites
  (`sprite_state.cc`, `sprite_sheet.cc`, `sprite_animation.cc`, `texture_atlas.cc`), removed
  `draw.cc`'s private copies, and deduped the three audio `system_ptr()` copies + `pc_speaker`.
  ~34 sites collapsed. Suite green. (Input `mouse_click`/`hotkey`/`gamepad_button` still inline —
  small, follow-up.)
- [x] **B. `draw.cc` color-guard wrappers.** ✅ Added `with_draw_color(c, fn)`; all 29 colored
  wrappers now route through it (`return with_draw_color(c, [&](auto& r){ return r.op(...); })`).
  Suite green.
- [x] **C. Test-fixture duplication (images + cameras).** ✅ New `test/video/test_images.hh`
  (`bmp_image(w,h)`/`bmp_image(size)`, `mem_image`, `surface_image(w,h,id=nullopt)`,
  `patterned_surface`) — consolidated 7 `bmp_image` copies + the drifting `surface_image` (fixed
  the same-name/different-impl landmine, bumping call sites to the uniform arity). New
  `test/video/world/test_cameras.hh` (`cam_at`, `plane_at`). Asserted values kept at call sites.
  Suite green.
  *Remaining (smaller):* the tileset/world/sprite_def **builders** (`uniform_ts`/`square_ts`/
  `uniform_tileset`, the three `make_world` signatures, `grid_def`/`basic_def`) — deferred because
  they're entangled with per-test gids/clip-sets; consolidate carefully so no assertions move
  out of view (`test_world_assets.hh`, `test_sprite_defs.hh`).

## Tier 2 — clear dedup, moderate effort

- [ ] **D. Handle/lease convergence.** `sprite_set_handle` (`sprite_cache.cc:18-70`) hand-rolls
  the copy-retain/move/reset that `resource_lease<Bundle>` already provides; `bundle_handle`
  restates `resource_cache_core::handle`. → make `sprite_set_handle` hold/derive `resource_lease`,
  delete its 5 special members; alias `bundle_handle` onto the core handle.
- [ ] **E. Self-registration RAII guard.** `sound_effect`/`music_stream` hand-roll register/
  unregister + custom move ops (partly fixed already — see Done). A `registration_guard<T>` member
  would let both default their move ops + dtor.
- [ ] **F. Image-source decode across 3 files** (`tileset_bundle.cc:32`, `sprite_set.cc:30`,
  `image_identity.cc:66`). Memory/disk arms identical; surface arm differs (borrow/clone/
  canonicalize). → `decode_source(src, surface_policy)` with the surface arm a caller callback.
- [ ] **G. `unregister_*` free-functions (4×)** repeat `if (!id.valid()) return; get mgr;
  if (null) return; mgr.erase(id);` → one `erase_if_live(id, eraser)`.
- [ ] **H. Strong-id boilerplate (5×)** → a `NEUTRINO_DEFINE_STRONG_ID(Name)` macro
  (`sprite_visual_id` keeps its extra `m_owner`).

## Tier 3 — subsystem-local (do when touching that area)

- [ ] **TMX loader.** Image-field parse block copy-pasted 3× (`tilesets.cc:43,162`,
  `layers.cc:166`; also fixes a `has_element` vs `has_attribute` inconsistency); tile-data
  branching 2×; document-load+root-detection 2×; `is_json()? plural:singular` ternary 3× →
  `for_each_child(node, json, xml, fn)`; `parse_group_context` 6 inherit-blocks → table-driven;
  `validate_cell_count` defined twice.
- [ ] **Physics `collide/world.hh` (2051 lines).** `BODY/BULLET/TILE` dispatch hand-written 8×
  (`material_of:657`, `is_valid:1993`, `get_shape/velocity/eid`, `remove`, `set_shape/velocity`)
  → one `visit_handle` dispatcher; 5 near-identical `add()` overloads → `add_resident(...)`;
  3 un-helpered nested `std::visit` narrow-phase sites → `visit2`/free `intersects(shape,shape)`;
  `fatten` re-implements existing `detail::tight_box`. Split impl bodies into
  `world_passes.inl`/`world_query.inl`/`world_controllers.inl`; move impl-only types out of the
  client-facing header.

## Tier 4 — opportunistic / low value

- [ ] Audio `clamp_volume` duplicated (and `sound_effect` never clamps — inconsistency);
  null-guarded stream forwarding ~15 methods each in `music_stream`/`pc_speaker`.
- [ ] `parse_enum` inline tables → `static constexpr`.
- [ ] Physics test variant-visit helpers.

## Done (from the review pass)

- [x] **Audio move-assignment.** `music_stream` had a real bug (conditional registration + defaulted
  move-assign left a target holding a live stream but unregistered) — fixed with explicit
  unregister→move→conditional-register; `sound_effect` made explicit for clarity (unconditional
  registration, so it was safe).
- [x] **Surface lock dedup.** Extracted the SDL3-correct `surface_pixel_lock` into
  `src/…/video/sprite/surface_lock.hh`; `cpu_texture_atlas.cc` and `image_identity.cc` share it.
