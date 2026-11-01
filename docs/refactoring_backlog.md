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

- [x] **F. Image-source decode dedup.** ✅ New internal `src/…/video/sprite/image_decode.hh`
  with `details::load_encoded_image(source)` for the identical disk/memory arms; `tileset_bundle.cc`
  (surface_for) and `sprite_set.cc` (`decode`) both use it, keeping their differing surface-arm
  policies local (tile borrows, sprite clones). Suite green.
- [x] **G. `erase_if_live`.** ✅ Added `erase_if_live(id, manager)` to `service_access.hh`; the
  three `unregister_sprite_{state,sheet,animation}` collapse to one line each. (`unregister_atlas`
  keeps its extra `uses()` guard, so it stays explicit.) Suite green.

### Deferred with rationale
- [ ] **D. Handle/lease convergence — deferred (header architecture).** `sprite_set_handle` is in a
  **public** header (`sprite_cache.hh`) but `resource_lease`/`resource_cache_core` are an **internal**
  `src` template that includes the private `utils/lru.hh`. Holding a `resource_lease` member would
  force the cache engine (and lru) public — a big API-surface cost to dedup a correct, tested ~50-line
  bespoke lease. Reconsider only if the cache core is intentionally made public.
- [ ] **E. `registration_guard` — deferred (doesn't fit address-keyed registration).** The audio
  managers key on the owner's `this` pointer, which changes on move. A guard **member** can't learn
  the containing object's new address on its own move, so the owner would still need custom move ops
  to rebind the guard — the guard wouldn't let them default. The move ops are already correct
  (fixed in Tier-1 review); a clean guard would require switching to a stable (heap control-block) key.
- [ ] **H. Strong-id macro — skipped (net-negative).** The 5 id types carry per-type doxygen and are
  public API; a `NEUTRINO_DEFINE_STRONG_ID` macro would move documentation outside the type and hurt
  "go to definition". The boilerplate is stable; the macro indirection isn't worth it. (The pure
  `std::hash` specializations could be macro'd, but that's ~15 lines total.)

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
  `src/…/video/sprite/surface_lock.hh`; `image_atlas.cc` and `image_identity.cc` share it.
