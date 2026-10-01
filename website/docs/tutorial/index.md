---
sidebar_position: 0
title: Tutorial
description: Build a small platformer, one engine concept at a time.
---

# Tutorial

This tutorial builds a small platformer from an empty window up. Each step adds one capability and
introduces exactly one part of the engine, so by the end you have both a running game and a reason
for every piece of it.

A platformer specifically, because it is what the engine's movement system is *for*: move-and-slide
against level geometry, standing on ground, one-way ledges you can jump up through, moving
platforms that carry you. Those are the parts hardest to discover from headers alone.

:::info[In progress]

The platformer tutorial is being published step by step. Steps 1 and 2 are available below. In the meantime,
the [examples](https://github.com/devbrain/neutrino/tree/main/examples) in the repository are working
code you can read and run — `sprite_demo` for sprites and animation, `map_viewer` for Tiled maps.
:::

## Tutorial Tracks

Neutrino offers two dedicated tutorial tracks depending on what you want to learn:

### 1. Platformer Tutorial (End-to-End Game)

A complete journey building a 2D platformer from an empty window up to moving platforms, one-way ledges, pickups, and audio.

| Step | You build | You learn |
|---|---|---|
| 1 | [A window and an empty scene](./step-01-window-and-scene.md) | The application, the frame loop, the scene stack |
| 2 | [One sprite on screen](./step-02-one-sprite.md) | Sprite definitions, sets, and the draw batch |
| 3 | [Moving it with the keyboard](./step-03-moving-with-keyboard.md) | The input snapshot; edges versus held state |
| 4 | Standing on ground | The physics world, kinematic bodies, move-and-slide |
| 5 | A real level | Loading a Tiled map; the camera following the player |
| 6 | Platforms that move, ledges you jump through | Carrier bodies and one-way response modes |
| 7 | Pickups and hazards | Sensors, trigger events, and the collider owner slot |
| 8 | Sound, and a clean exit | Audio playback and resource lifetime |

### 2. [Drawing Tutorial Series](./drawing/index.md) (Rendering Deep Dive)

A 4-part hands-on series focused entirely on mastering 2D drawing, sorting, cameras, and offscreen buffers:

| Part | You build | You learn |
|---|---|---|
| **1** | [Primitives & UI](./drawing/01-primitives-and-ui.md) | Player HUD, status gauges, and diagnostic physics gizmos |
| **2** | [Batches & Ordering Bands](./drawing/02-batches-and-ordering.md) | 2.5D depth sorting, coarse `draw_layer` bands, and tie-breaking |
| **3** | [Camera & Parallax](./drawing/03-camera-and-parallax.md) | Look-at tracking, smooth zoom, and drift-free multi-plane parallax |
| **4** | [Offscreen Render Textures](./drawing/04-render-textures.md) | `render_texture::compose()`, bake-once static art, and cached minimaps |

## How this fits the rest of the docs

The tutorials are the **narrow path** — focused practical implementations chosen for teaching. Each step links to the [concept pages](../concepts/index.md) that explain the architectural reasoning, mathematics, and edge cases.

If you would rather read the wide architectural explanation first, start at
[The frame](../concepts/the-frame.md) or [Drawing](../concepts/drawing.md).
