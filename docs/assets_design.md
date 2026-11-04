# Assets — design draft

Status: **draft, low priority** (written 2026-11-04). Nothing here is scheduled; the
painter ([`render_model.md`](render_model.md)), fonts ([`font_plan.md`](font_plan.md))
and sprites ([`sprite_layer_plan.md`](sprite_layer_plan.md)) come first. This document
records the findings so that work on them does not paint the asset layer into a corner.

---

## 1. Why

Images, atlases, fonts, sound effects, music and maps are all assets, but neutrino only has
per-type *loaders*, not an asset *system*. Every game has built the missing parts itself:

- **Luxor** — its own `vfs` (`games/luxor/resources/storage/vfs.hh`: directory and archive
  mounts, its own archive formats in `archive.cc`, `set_locale` for localised overlays,
  `read` / `stat` / `list_files`), game detection (`storage/game.cc` recognises "5th
  Passage" by its files), and an `assets` class loading images and sounds through the vfs.
  Data path hard-coded (`luxor.cc:56`).
- **warlords** — its own `vfs_directory`, edition detection with a discover → load-manifest
  step per edition (`wl2`, `wl2deluxe`, `wl2scn`, `wl210`), a `font_service`, a
  `texture_cache`, and a `vault` that stores converted assets (msgpack): a derived-data
  cache.
- **KE** — data path from `argv[1]` (hard-coded in `ke_collage.cc`), `ke.rsc` read with
  `std::ifstream`, a hand-made `ke_assets` struct owned by the application.

This is the "push common machinery to neutrino" situation again.

## 2. Background

### 2.1 Gregory, *Game Engine Architecture* ch. 7

The runtime resource manager:

- keeps **one copy per resource**, through a **registry** keyed by a unique id — usually a
  path (string or hash); Unreal uses package + in-package path;
- manages **lifetime by scope**: global resources (HUD fonts, the player) live for the
  game; level resources per level, shared ones reference-counted *per level* (increment
  the next level's set, decrement the previous one, load / unload the difference); shorter
  ones (cinematics); music and video **streamed** in chunks;
- **never loads during gameplay unless asynchronously** — either everything up front
  behind a loading screen, or background streaming; synchronous on-demand loads hitch;
- handles **composite resources and referential integrity** (GUID cross-references or
  pointer fix-ups) and **post-load initialisation**;
- organises files as **loose files in development, packed archives** (ZIP / pak) for
  shipping: faster, compressed, modular — e.g. one archive per language;
- has an offline side: a resource database (metadata on *how* to build each asset) and an
  asset conditioning pipeline (export → compile → link) with dependency rules.

### 2.2 Current practice

| Engine / library | What is relevant |
|---|---|
| Godot | `res://` virtual paths and `uid://` ids that survive moves; typed loaders per format; path-keyed cache; import step (source + `.import` metadata → engine format); **PCK packs mounted as overlays** (patches, mods, DLC); threaded loading with progress |
| Bevy | `AssetServer`, typed refcounted `Handle<T>`; a loader per extension; async loading and load events; hot reload; asset sources (file, embedded, http); processed assets with meta files |
| Unity Addressables | assets addressed by key, grouped into bundles; async load handles with explicit release |
| PhysicsFS / LÖVE | mount directories and archives with precedence, a separate write directory, typed loaders on top |
| SDL3 | `SDL_IOStream` as the universal byte source; `SDL_Storage` (read-only *title* storage vs *user* storage, built for consoles); `SDL_AsyncIO` with completion queues; `SDL_GetBasePath` / `SDL_GetPrefPath`. lib_sdlpp wraps all of them (`sdlpp/io/iostream.hh`, `storage.hh`, `async_io.hh`, `filesystem.hh`) |

## 3. Neutrino today

| Asset | Loader input | Cache / identity | Lifetime |
|---|---|---|---|
| image | `load_image(path)`, `load_image(span)`, `decode_image(span, decoder)` | — | caller |
| atlas | `build_atlas` + internal `register_atlas` | atlas id (registry) | application (sprite plan D12) |
| tileset (world) | TMX loader, filesystem paths | `resource_cache`: content-keyed, acquire / release, cold LRU budget | refcount + budget |
| sound effect | `load_sfx(std::string)`, `load_sfx(std::istream&)` | — | RAII object |
| music | `load_music(std::string)`, `load_music(std::istream&)`, `play_music(path)` | — | RAII object, streamed |
| font | — (`font_plan.md`) | — | — |

Four input types (`std::string`, `std::filesystem::path`, `std::span<const uint8_t>`,
`std::istream&`) across loaders; path overloads go straight to the OS file system. Decoder
registration exists per type (`register_image_decoder`, the audio `register_decoder`, the
planned `register_font_decoder`) but is not unified.

## 4. libvfs (`~/proj/ares/vfs`)

The author's own VFS library; already on neutrino-cmake (commit `b9d7f4b`).

### 4.1 What it provides

- **A Unix-style mount tree**: `vfs::mount(fstype, args, mount_point)`, a global `fstab`,
  `open_directory`, `get_stats` (size, type, attributes), a global cwd.
- **File I/O**: `open` / `read` / `write` / `seek` / `tell` / `truncate` / `unlink` /
  `create_directory`, read-only and creation modes; `std::istream` / `std::ostream`
  adaptors (`file_input_stream`, `file_output_stream`).
- **Filesystem modules**: `physfs` (a real directory), `zipfs` (miniz), `tarfs`, `lhafs`
  (lhasa), `floppyfs` (FAT disk images); also `7zipfs`, `rarfs`, `olefs` (`iso9660` is a
  README only). LHA and floppy images fit DOS / Amiga data.
- **Extension**: a C plugin ABI (`include/vfs/api/vfs_module.h`, shared libraries loaded
  with `load_module(path)`) and in-process registration
  (`load_module(std::unique_ptr<module::filesystem>)`); `extra_tools/archived_fs.hh` and
  `read_only_fs.hh` turn "a list of entries + a reader" into a read-only filesystem —
  where Luxor's, KE's and warlords's archive formats would go.
- **`vfs::archive`**: use an archive without mounting it.
- **Tests**: mount, open, read / write, streams, unlink.

### 4.2 Fit against neutrino's needs

| Need | libvfs |
|---|---|
| mount directories and archives under virtual paths | ✅ |
| pluggable archive formats, including game formats | ✅ modules + `archived_fs` helpers |
| stream access for existing loaders and parsers | ✅ `file_input_stream` |
| write location for saves | ✅ a writable `physfs` mount (e.g. the pref path at `/save`) |
| **overlays** (patch / mod / locale over base data) | ❌ one filesystem per mount point; mounting the same path twice throws "this path is already mounted" (`fstab::mount`) |
| **case-insensitive lookup** (DOS data on Linux) | ❌ not handled in path resolution |
| **thread safety** (decoding on workers) | ❌ global tables, no locking |
| `SDL_IOStream` bridge (SDL_image, onyx, musac `io_stream`) | ❌ only `std::iostream` |
| RAII file handles | ❌ raw `vfs::file*` + `close()` |
| archives that are themselves VFS files (nested, in memory) | ❓ archive modules receive a `params` string (`zipfs::create_archive_io(params)`) and appear to open OS paths — to be confirmed |
| static builds and web | ⚠️ shared library by default (`NEUTRINO_VFS_BUILD_SHARED ON`) with DLL plugin loading; Emscripten needs a static build with in-process modules only |
| instances vs a global | ⚠️ one process-wide VFS: fine for a game, awkward for test isolation and tools that open several data sets (warlords editions) |
| dependencies | ⚠️ `bsw` (string tokenizer, buffered stream buffers) and `failsafe`; neutrino does not use bsw today |

## 5. Proposed architecture

The same arrangement as onyx_image and onyx_font: libvfs underneath, neutrino's own
vocabulary on top. Games mount data and open files through neutrino types only.

```
 game                    asset groups, typed loads, its own data layout
 ─────────────────────────────────────────────────────────────────────────────
 neutrino (public)       data locator   │ vfs facade              │ typed loaders + caches
                         base / pref    │ mounts with precedence, │ load<T>(vpath) → image,
                         path, original │ case-insensitive option,│ image_bank, atlas, font,
                         game detection │ RAII files, read_all,   │ sound, music, map, bytes;
                                        │ open_io → SDL_IOStream  │ caches; groups; async
 ─────────────────────────────────────────────────────────────────────────────
 libvfs (hidden)         mount tree, modules (physfs, zip, tar, lha, floppy, game archives)
```

### 5.1 Byte source

One input type for every loader: an `SDL_IOStream`-backed stream opened from the VFS
(SDL3 supports custom stream interfaces), plus `read_all(vpath) → bytes`. Image, font, TMX
and game-parser loaders take it directly; audio through a musac `io_stream` adapter. Path
overloads stay as conveniences that go *through* the VFS.

### 5.2 VFS facade

- `mount(vpath, source, options)` with **precedence** (overlays for patches, mods and
  locale, as Luxor's `set_locale` does today); `options.case_insensitive` per mount.
- RAII file handles, `exists`, `stat`, `list`.
- A writable mount for saves at the pref path, separate from read-only game data (SDL's
  title / user storage split).

### 5.3 Data locator

Base path, pref path, and a standard way to locate *original game data*: configuration or
command line first, then probing known install layouts. Edition / version detection
belongs to the same step (Luxor's `storage/game.cc`, warlords's discover per edition) —
neutrino provides the probing machinery, the game provides the signatures.

### 5.4 Typed loaders, identity and caching

- `load<T>(vpath)` over the existing decoders; one registration scheme for image,
  image bank, font, audio and map decoders.
- **One copy per key**: a generic cache keyed by virtual path or by the game's name
  (`atlas_cache` in the sprite plan, D13, is one instance). `resource_cache` (tilesets)
  shows the acquire / release / budget shape already in the tree.
- Identity stays id-based where it already is (atlas ids never reused, value handles):
  that gives referential integrity without fix-up tables.

### 5.5 Lifetime: scopes and groups

Gregory's global / level / cinematic / streamed split matches what is already decided:
atlases application-lifetime by default, *group release* later (`render_model.md` §8.2).
An **asset group** — a manifest of what a level or scene needs — is the natural unit for
that release and gives a loading screen something to count.

### 5.6 Background loading

Synchronous on the main thread today. Minimum: "load this group with progress behind a
loading scene" — file reads through `SDL_AsyncIO` or worker threads, decoding on workers,
GPU upload on the main thread. Requires a thread-safe VFS (§4.2).

### 5.7 Development conveniences

- **Hot reload**: file watching + replacement in place (`render_model.md` §8.2).
- **Diagnostics**: loaded assets and sizes; missing assets reported with the virtual path
  and the mount list; load timings.
- **Derived-data cache** (optional): converted results of expensive original-format
  conversions, keyed by source hash + converter version — the runtime form of Gregory's
  conditioning pipeline, as warlords's `vault` does. neutrino-native games could pack
  offline instead (atlases, baked fonts).

### 5.8 Not needed

A resource database with revision history, chunk / stack allocators, pointer fix-up tables
(Gregory himself: choose from measured need).

## 6. Open decisions

- **A1 — overlays and case-insensitive lookup: in libvfs or in the facade?** Proposed: in
  libvfs — they are filesystem semantics, and the facade would otherwise re-resolve paths.
- **A2 — libvfs build profile for neutrino.** Proposed: static, in-process modules only;
  the shared-library plugin path stays in libvfs for other users.
- **A3 — nested archives.** Can archive modules read from a stream / VFS file instead of
  an OS path? Needed for archives inside game archives and for in-memory data.
- **A4 — one global VFS or instances.** The engine needs one; tests and multi-edition
  tools want instances. Proposed: instances in libvfs, one engine-owned instance in
  neutrino.
- **A5 — `bsw` dependency.** Keep, or replace the few uses (tokenizer, stream buffers)
  so neutrino does not pull another library.
- **A6 — key type.** Virtual path strings, hashed paths, or game-chosen names; the cache
  needs one answer (Godot-style uids that survive moves are not needed for original-data
  games).

## 7. Milestones (unscheduled)

- **AS1 — libvfs fixes**: union mounts with precedence, case-insensitive mounts, locking,
  stream-input archive modules (A3), static profile (A2), instances (A4).
- **AS2 — neutrino VFS facade + byte source**: mounts, RAII files, `read_all`,
  `open_io` → `SDL_IOStream`; every loader accepts it; path overloads route through the
  VFS.
- **AS3 — data locator**: base / pref path, original-data discovery and edition probing;
  KE and Luxor drop their hard-coded paths.
- **AS4 — game archives as modules**: Luxor's archives, KE's `.rsc`; Luxor's own `vfs`
  retires.
- **AS5 — typed loaders and caches**: `load<T>`, unified decoder registration, one cache
  shape for every asset type.
- **AS6 — asset groups and background loading**: manifests, group release
  (`render_model.md` §8.2), progress for a loading scene.
- **Later**: hot reload, diagnostics, derived-data cache.

Dependencies: AS1 → AS2 → AS3, AS4, AS5 → AS6. AS2 is the step that pays off first: once
every loader takes one byte source, fonts, audio and images can be loaded from archives
without game code.

## 8. Relation to the other plans

- **Sprites** (D12, D13): atlases application-lifetime; `atlas_cache` is the first cache
  of §5.4; reload = replacement in place.
- **Painter** (§8): replacement in place, group release, device reset — the GPU side of
  lifetime; asset groups (§5.5) decide *when* a group is released.
- **Fonts** (FM4): `load_font(path | bytes)` takes the byte source of §5.1 once it exists;
  glyph-bank decoders are typed loaders (§5.4).
- **Audio**: `load_sfx` / `load_music` gain the byte-source overload; music keeps
  streaming.

## References

- J. Gregory, *Game Engine Architecture*, 3rd ed., ch. 7: §7.1 file system, §7.2.1
  offline resource management, §7.2.2 runtime resource management (registry, lifetime,
  memory, composite resources, cross-references, post-load initialisation).
- libvfs: `~/proj/ares/vfs` (`include/vfs/*.hh`, `include/vfs/api/vfs_module.h`,
  `src/extra/*`).
- SDL3: `SDL_iostream.h`, `SDL_storage.h`, `SDL_asyncio.h`, `SDL_filesystem.h`.
