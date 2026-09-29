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

:::info In progress

The platformer tutorial is being published step by step. Steps 1 and 2 are available below. In the meantime,
the [examples](https://github.com/devbrain/neutrino/tree/main/examples) in the repository are working
code you can read and run — `sprite_demo` for sprites and animation, `map_viewer` for Tiled maps.
:::

## The plan

| Step | You build | You learn |
|---|---|---|
| 1 | [A window and an empty scene](./step-01-window-and-scene.md) | The application, the frame loop, the scene stack |
| 2 | [One sprite on screen](./step-02-one-sprite.md) | Sprite definitions, sets, and the draw batch |
| 3 | Moving it with the keyboard | The input snapshot; edges versus held state |
| 4 | Standing on ground | The physics world, kinematic bodies, move-and-slide |
| 5 | A real level | Loading a Tiled map; the camera following the player |
| 6 | Platforms that move, ledges you jump through | Carrier bodies and one-way response modes |
| 7 | Pickups and hazards | Sensors, trigger events, and the collider owner slot |
| 8 | Sound, and a clean exit | Audio playback and resource lifetime |

## How this fits the rest of the docs

The tutorial is the **narrow path** — one route through the engine, chosen for teaching rather than
completeness. Each step links to the [concept page](../concepts/index.md) that explains its subject
properly, and those pages are where the edge cases, alternatives and reasoning live.

If you would rather read the wide explanation first, start at
[The frame](../concepts/the-frame.md).
