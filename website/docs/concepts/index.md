---
sidebar_position: 0
title: Concepts
description: The model behind the engine — how it thinks about time, space, resources and collision.
---

# Concepts

These pages explain the engine's model: not what each function is called, but why the pieces are
shaped the way they are. They are meant to be read in roughly this order the first time, and dipped
into afterwards.

For exhaustive signatures, see the [API reference](pathname:///api/). For a single worked
example that touches most of this, see the [tutorial](../tutorial/index.md).

## Available now

Read these in order — together they describe the shape of every frame your game runs and how space and time are structured.

- **[The frame](./the-frame.md)** — one simulation clock, fixed steps, rendering as an event, and
  why `fixed_update` receives a constant `dt`.
- **[Scenes](./scenes.md)** — the stack, lifecycle hooks, opaque versus overlay, and why
  transitions are queued rather than immediate.
- **[Input](./input.md)** — the per-frame snapshot, why edges and held state are different things,
  and the two spaces a pointer position can be in.
- **[Coordinate spaces](./coordinate-spaces.md)** — window, render, and world space; the strong quantity
  types (`world_pos`, `world_delta`, `world_velocity`) that prevent space confusion at compile time.
- **[Sprites](./sprites.md)** — the three-tier sprite pipeline (`sprite_def` → `sprite_set` → `sprite_instance`),
  sheets, dynamic atlases, clips, and automatic cache leasing.
- **[Drawing](./drawing.md)** — immediate-mode primitives, depth-sorted batches, coarse ordering bands,
  look-at cameras with parallax alignment, and offscreen render textures.

## Planned

The remaining pages are being written. Until each lands, the header comments in the corresponding
public header are the best source — they are extensive, and several of these pages will be drawn
directly from them.

| Page | Covers | Header |
|---|---|---|
| Tile worlds | Tiled loading, tilesets, layers, the compositor | `world/` |
| Physics | Body kinds, the four passes, move-and-slide, targets vs velocities, owner slots, events | `physics/collide/world.hh` |
| Audio | Effects, music, the PC speaker, codec registration | `audio/audio.hh` |
| Resource lifetime | Caches, leases, `ready()` and `teardown()` | `application.hh` |
