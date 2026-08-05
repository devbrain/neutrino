---
sidebar_position: 1
title: What Neutrino is
description: What the engine does, what it deliberately does not, and who it suits.
---

# What Neutrino is

Neutrino is a C++20 game engine for 2D games, built on [SDL3](https://www.libsdl.org/) through the
[sdlpp](https://github.com/devbrain/lib_sdlpp) wrapper. It gives you an application and frame loop,
a scene stack, sprites and atlases, a tile-world renderer, a collision and movement system, and
audio — as a library you link against, not a program you launch.

There is no editor and no scripting layer. You write C++, you build with CMake, and you get a
binary.

## What it gives you

| | |
|---|---|
| **Application** | Window, renderer, main loop, and a fixed-timestep simulation clock decoupled from the display rate |
| **Scenes** | A stack with lifecycle hooks, opaque and overlay modes, and queued transitions |
| **Input** | A per-frame snapshot delivered to the simulation, with press/release edges separated from held state |
| **Sprites** | Definitions, sheets, runtime atlas packing, animation clips and playback state |
| **Drawing** | Depth-sorted batches with ordering bands, cameras with parallax, offscreen render textures |
| **Tile worlds** | Tiled (TMX/TSX) loading, tilesets, layers, and a compositor |
| **Physics** | Static, kinematic, carrier and bullet bodies; move-and-slide; swept CCD; sensors and events |
| **Audio** | Sound effects, music streaming, a PC-speaker synth, and custom codec registration |

## What it deliberately is not

Being clear about this saves you evaluating it for a job it will not do.

- **Not 3D.** The geometry, the renderer and the physics are all two-dimensional.
- **Not a rigid-body simulator.** Movement is kinematic — you say where a body should go and the
  engine resolves that against geometry. There is no mass, no torque, no joints, no solver
  iterating a constraint system. This suits platformers, top-down games and arcade action; it does
  not suit a physics puzzler about stacking crates.
- **Not an editor or asset pipeline.** Levels come from Tiled. Sprites come from your own files.
  There is no GUI, no scene serialisation format, no hot reload.
- **Not a general renderer.** Drawing is sprite-and-tile oriented, on SDL3's 2D renderer. No
  shaders, no custom render passes.

## Where the design leans

A few decisions run through the whole engine, and knowing them early makes the rest read
predictably.

**The simulation has one clock.** Logic advances in fixed steps at a rate you choose, independent
of how fast frames present. Rendering is an event that happens when the display is ready, not a
second timebase. This is why scene updates receive a constant `dt` and why physics is
deterministic regardless of frame rate.

**Intent is expressed, not computed.** Where an engine might make you compute a velocity to reach a
position, Neutrino lets you state the position and resolves it against geometry itself. The
difference matters when the target is unreachable — an expressed intent stops at the wall, while a
computed velocity becomes an enormous number pointed into it.

**Quantities carry their meaning in their type.** Positions, offsets and velocities are distinct
types that combine only in ways that mean something: subtracting two positions gives an offset,
integrating a velocity over a duration gives an offset, and adding two positions does not compile.
The same separation applies to render space and window space, which differ by display scaling and
letterboxing.

**Resources are owned, leased, and released explicitly.** Sprite sets are leased from a cache.
Applications get a matching pair of `ready()` and `teardown()` hooks, and the second runs while the
renderer is still alive — because releasing a GPU handle after the device is gone is a bug that
hides until it crashes on exit.

## Whether it suits you

It probably suits you if you want to write a 2D game in modern C++, you are comfortable with CMake,
and you would rather read an engine's source than its manual.

It probably does not if you want a visual editor, a scripting language, 3D, or a large ecosystem of
existing assets and tutorials. Those are real needs, and Godot or Unity serve them far better.

## Next

Build it and run something: [Building Neutrino](./build.md).
