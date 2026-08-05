# Neutrino

A C++20 game engine for 2D games, built on [SDL3](https://www.libsdl.org/) through the
[sdlpp](https://github.com/devbrain/lib_sdlpp) wrapper.

Scenes, sprites and atlases, tile worlds, kinematic collision, and audio — as a library you link
against. There is no editor and no scripting layer: you write C++, build with CMake, and get a
binary.

📖 **[Documentation](https://devbrain.github.io/neutrino/)**

## Build

Neutrino fetches every dependency itself. Nothing to install first, no submodules to initialise.

```bash
git clone https://github.com/devbrain/neutrino.git
cd neutrino
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

Requires CMake 3.20+, a C++20 compiler, and network access at configure time (that step clones the
dependencies, and takes a couple of minutes the first time).

Then run something:

```bash
./build/bin/neutrino_sprite_demo   # sprite sheets, animation, explicit cleanup
./build/bin/neutrino_map_viewer    # Tiled maps: orthogonal, isometric, hexagonal
./build/bin/neutrino_tests         # the test suite
```

See [Building Neutrino](https://devbrain.github.io/neutrino/docs/get-started/build) for build
options and how to consume Neutrino from your own CMake project.

## What it does and does not do

**Does:** fixed-timestep simulation decoupled from the display rate; a scene stack; sprite
definitions, runtime atlas packing and animation; depth-sorted draw batches with cameras and
ordering bands; Tiled (TMX/TSX) loading and rendering; static/kinematic/carrier/bullet bodies with
move-and-slide, swept CCD and sensor events; sound effects, music streaming and custom codecs.

**Does not:** 3D, rigid-body dynamics (no mass, joints or constraint solver), a visual editor, an
asset pipeline, scripting, or shaders.

[What Neutrino is](https://devbrain.github.io/neutrino/docs/get-started/what-is-neutrino) goes into
this properly, including whether it suits what you are building.

## Repository layout

| Path | |
|---|---|
| `include/neutrino/` | Public headers — extensively documented, and the reference until the API site lands |
| `src/neutrino/` | Implementation |
| `test/` | doctest suite |
| `examples/` | `sprite_demo`, `map_viewer` |
| `games/ke/` | Krypton Egg, an Arkanoid clone used to exercise the engine against a real game |
| `docs/` | Internal design notes |
| `website/` | The documentation site (Docusaurus) |

## Documentation

The site is built with Docusaurus and published from `website/` on every push to the default
branch.

```bash
cd website
npm ci
npm start        # dev server with hot reload
npm run build    # production build; fails on broken links
```

## Status

Pre-1.0 and moving. The API still changes between commits, and there is no release or versioning
scheme yet.
