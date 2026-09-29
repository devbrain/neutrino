---
sidebar_position: 2
title: Building Neutrino
description: Clone, configure, build, and run one of the examples.
---

# Building Neutrino

Neutrino builds with CMake and fetches every dependency itself. There is no package to install
first and no vendored submodule to initialise — configuring the project downloads what it needs.

## Requirements

| | |
|---|---|
| **CMake** | 3.20 or newer |
| **Compiler** | Anything with solid C++20 support. The standard is set to 20 and required, so a compiler that cannot provide it fails at configure time rather than midway through the build |
| **Git** | Used by CMake to fetch dependencies |
| **Network** | Required at configure time, for the same reason |

The instructions below were verified from a clean clone on Ubuntu 24.04 with GCC 13.3 and
CMake 3.28.

## Clone, configure, build

```bash
git clone https://github.com/devbrain/neutrino.git
cd neutrino
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

The configure step is the slow one — it clones and configures every dependency, which took about
two minutes on a warm network. Subsequent configures reuse what it fetched.

:::note[What gets downloaded]

Configuring pulls in [neutrino-cmake](https://github.com/devbrain/neutrino-cmake) (the shared build
infrastructure), then SDL3 and sdlpp, euler, the onyx image/font/animation libraries, musac for
audio, stb, pugixml, nlohmann/json, zstd, and doctest. They land under `build/_deps/`. Nothing is
installed system-wide.
:::

## Run something

The examples bake their asset paths in at compile time, so they run from any working directory.

```bash
./build/bin/neutrino_sprite_demo   # sprite sheets, animation states, explicit cleanup
./build/bin/neutrino_map_viewer    # Tiled maps: orthogonal, isometric, hexagonal
```

The test suite is a good second check that the build is sound:

```bash
./build/bin/neutrino_tests
```

:::caution[The `ke` target needs assets it does not ship]

`ke` (Krypton Egg) also builds, but it reads game data from a commercial resource file that is not
in the repository, and it currently looks for it at a path hard-coded for the author's machine. It
will build and then fail to start. Use the examples instead.
:::

## Build options

Every option is prefixed `NEUTRINO_NEUTRINO_` — the component name appears twice because the
shared build infrastructure namespaces options per component, and this component is itself called
`neutrino`.

| Option | Default | Effect |
|---|---|---|
| `NEUTRINO_NEUTRINO_BUILD_TESTS` | `ON` when top-level | Builds `neutrino_tests` |
| `NEUTRINO_NEUTRINO_BUILD_EXAMPLES` | `ON` when top-level | Builds the example programs |
| `NEUTRINO_NEUTRINO_BUILD_BENCHMARKS` | `OFF` | |
| `NEUTRINO_NEUTRINO_BUILD_DOCS` | `OFF` | Builds the Doxygen API reference |
| `NEUTRINO_NEUTRINO_BUILD_SHARED` | `OFF` | Shared instead of static library |
| `NEUTRINO_NEUTRINO_INSTALL` | `ON` when top-level | Enables the install rules |
| `NEUTRINO_NEUTRINO_TMX_ENABLE_ZSTD` | `ON` | zstd-compressed TMX layer data |

The `BUILD_TESTS`, `BUILD_EXAMPLES` and `INSTALL` defaults flip to `OFF` when Neutrino is consumed
as a subproject, so adding it to your own build does not drag in its test suite.

For a faster build, turn off what you are not using:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DNEUTRINO_NEUTRINO_BUILD_TESTS=OFF \
  -DNEUTRINO_NEUTRINO_BUILD_EXAMPLES=OFF
```

## Using Neutrino in your own project

Add it as a subdirectory and link the `neutrino` target:

```cmake
add_subdirectory(neutrino)

add_executable(my_game main.cc)
target_link_libraries(my_game PRIVATE neutrino)
```

Linking the target brings its include directories and its C++20 requirement with it — you do not
need to set the standard or spell out include paths yourself.

## Next

With a build in hand, the [concepts](../concepts/index.md) explain the model behind the engine —
start with [The frame](../concepts/the-frame.md). The [tutorial](../tutorial/index.md) will build a
small platformer from an empty window up.
