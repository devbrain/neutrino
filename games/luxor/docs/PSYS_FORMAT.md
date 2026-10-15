# PSYS (MumboJumbo Particle System) File Format Documentation

## Overview

In the MumboJumbo game engine (powering *Luxor*, *Luxor: Amun Rising*, and *Luxor Mahjong*), `.psys` files define 2D particle systems and particle emitters. They govern visual effects such as ball collapses, lightning strikes, explosion fireballs, powerup sparkles, smoke trails, scarab flashes, and UI progress gauges.

Particle systems are parsed and executed by the `gfx2d.dll` engine subsystem (`gfxParticleSystem` and `gfxParticleSystemInstance`). Each `.psys` file contains one or more named `Emitter` blocks that define particle generation rates, physical simulation parameters, sprite bindings, animation cycles, and color palettes.

---

## File Structure & Syntax

A `.psys` file is a plain text file using a C-style block format with `//` comments, key-value assignments, and bitwise OR flags (`|`).

```text
// Multi-emitter Particle System
Emitter <EmitterName1>
{
    Type = Sprite
    Flags = <Flag1> | <Flag2> | ...
    
    // Particle count & rates
    StartParticles = <int>
    MaxParticles = <int>
    ParticleRate = <float>
    
    // Visual assets & animations
    Sprite = <SpritePath>
    [Palette = <PalettePath>]
    [Spline = <SplinePath>]
    AnimMin = <int>
    AnimMax = <int>
    AnimRate = <int>
    ColorRate = <float>
    
    // Particle lifespan & alpha fade
    LifespanMin = <float>
    LifespanMax = <float>
    FadeInEndTime = <float>
    FadeOutStartTime = <float>
    RenderDelay = <float>
    FirstFrameDelay = <float>
    
    // Spawning geometry & offset
    PosX = <float>
    PosY = <float>
    SpawnRadiusMin = <float> <float>
    SpawnRadiusMax = <float> <float>
    
    // Kinematics & Physics
    StartVelMin = <float> <float>
    StartVelMax = <float> <float>
    Acc = <float> <float>
    DevDelay = <float>
    DevAngle = <float> <float>
    
    // Emitter movement & lifespan
    EmitterVelMin = <float> <float>
    EmitterVelMax = <float> <float>
    EmitterAcc = <float> <float>
    EmitterStart = <float> <float>
    EmitterLifespan = <float> <float>
    [EmitterWarmup = <float> <float>]
}

Emitter <EmitterName2>
{
    ...
}
```

---

## Emitter Flags Reference (`Flags`)

Emitter behavior and physics are controlled by bitwise OR combinations of `EF_` flags:

| Flag | Description |
|---|---|
| `EF_NONE` | Default state with no special modifiers. |
| `EF_SPRITE_ANIM_LOOP` | Particle sprite loops its animation sequence indefinitely. |
| `EF_SPRITE_RANDOM_FRAME` | Particle initializes playback at a randomized frame in the sprite sheet. |
| `EF_USE_COLOR_RATE` | Enables color shifting through the bound `Palette` texture at `ColorRate`. |
| `EF_PALETTE_LOOP` | Color cycling through the palette repeats continuously. |
| `EF_LIFESPAN_INFINITE` | Individual particles never expire from age (used for shadows, ball overlays). |
| `EF_ELIFESPAN_INFINITE` | Emitter itself never stops emitting (continuous / ambient effect). |
| `EF_POS_RELATIVE` | Particle coordinates stay relative to the moving parent entity (e.g. following the shooter or a rolling ball). |
| `EF_VEL_POSRELATIVE` | Initial velocity direction is calculated relative to spawn offset vector. |
| `EF_VEL_DEVIATION` | Velocity vector deviates randomly based on `DevAngle` after `DevDelay`. |
| `EF_VEL_ORBIT` | Particles orbit circularly around the emitter's origin. |
| `EF_VEL_TANGENTIAL` | Particles are projected tangentially along the spawn radius circumference. |
| `EF_ACC_ATTRACTION` | Particle acceleration acts as gravitational pull towards the emitter origin. |
| `EF_KILL_ON_STOP` | All live particles are deleted immediately when emitter is stopped, rather than allowing them to fade out naturally. |
| `EF_HARDWARE` | Forces hardware acceleration for particle geometry. |
| `EF_SOFTWARE` | Forces software rendering fallback. |

---

## Property Reference

### 1. Particle Counts & Emission Rates
| Property | Type | Description |
|---|---|---|
| `Type` | Enum | Emitter particle type (almost universally `Sprite`). |
| `StartParticles` | Integer | Burst particle count spawned immediately when the emitter starts. |
| `MaxParticles` | Integer | Hard upper limit on the total concurrent active particles for this emitter. |
| `ParticleRate` | Float | Emission rate in particles per second after startup burst. |

### 2. Lifespan & Alpha Fading
| Property | Type | Description |
|---|---|---|
| `LifespanMin`, `LifespanMax` | Float | Lifetime range $[T_{\min}, T_{\max}]$ in seconds for each particle. |
| `FadeInEndTime` | Float | Normalized life percentage $[0.0, 1.0]$ when particle reaches full alpha ($255$). |
| `FadeOutStartTime` | Float | Normalized life percentage $[0.0, 1.0]$ when particle begins fading out to alpha $0$. |
| `RenderDelay` | Float | Delay in seconds before spawned particles become visible. |
| `FirstFrameDelay` | Float | Delay in seconds before emitting the first particle frame. |

### 3. Spawning Geometry
| Property | Type | Description |
|---|---|---|
| `PosX`, `PosY` | Float | Anchor offset in pixels from parent entity origin. |
| `SpawnRadiusMin` | Float Float | Minimum $[X, Y]$ bounding box displacement relative to anchor. |
| `SpawnRadiusMax` | Float Float | Maximum $[X, Y]$ bounding box displacement relative to anchor. |

### 4. Physics & Trajectory
| Property | Type | Description |
|---|---|---|
| `StartVelMin` | Float Float | Minimum initial velocity vector $[V_x, V_y]$ in pixels/second. |
| `StartVelMax` | Float Float | Maximum initial velocity vector $[V_x, V_y]$ in pixels/second. |
| `Acc` | Float Float | Linear acceleration vector $[A_x, A_y]$ in pixels/second² (e.g. downward gravity: `0 -200`). |
| `DevDelay` | Float | Delay in seconds before directional velocity deviation takes effect. |
| `DevAngle` | Float Float | Minimum and maximum angular deviation range in degrees (e.g. `-200 200`). |

### 5. Emitter Origin Kinematics & Lifetime
| Property | Type | Description |
|---|---|---|
| `EmitterVelMin`, `EmitterVelMax` | Float Float | Movement velocity range $[V_x, V_y]$ for the emitter origin itself. |
| `EmitterAcc` | Float Float | Acceleration vector $[A_x, A_y]$ of the emitter origin. |
| `EmitterStart` | Float Float | Initial startup delay / offset vector for the emitter. |
| `EmitterLifespan` | Float Float | Active duration range $[T_{\min}, T_{\max}]$ in seconds the emitter remains active. |
| `EmitterWarmup` | Float Float | Pre-simulation warmup interval in seconds before the first game tick. |

### 6. Visual Assets & Color Animation
| Property | Type | Description |
|---|---|---|
| `Sprite` | File Path | Relative path to `.spr` descriptor (e.g. `data\sprites\particles\spark_ball_2.spr`). |
| `Palette` | File Path | 256-step color gradient bitmap (`.jpg` / `.png`) used for particle color mapping. |
| `Spline` | File Path | Path to pre-defined Bezier trajectory file (`.spl`). |
| `ColorRate` | Float | Cycling speed stepping across the palette gradient (samples/sec). |
| `AnimMin`, `AnimMax` | Integer | Sub-animation index range in the sprite sheet. |
| `AnimRate` | Integer | Animation playback rate in frames per second (e.g. `30` or `60`). |

---

## Real Game Examples

### Example 1: Compound Powerup Effect (`powerup_bomb.psys`)
Combines four distinct emitters (drop shadow, gem body, bomb cartouche, and sparks) into one synchronized particle system:

```text
Emitter shadow
{
    Type = Sprite
    Flags = EF_LIFESPAN_INFINITE | EF_POS_RELATIVE
    StartParticles = 1
    MaxParticles = 1
    ParticleRate = 25.000000
    Sprite = data\sprites\particles\gem_shadow.spr
    PosX = 12.000000
    PosY = 12.000000
    LifespanMin = 1.000000
    LifespanMax = 1.000000
    EmitterLifespan = 0.500000 0.500000
}

Emitter gem
{
    Type = Sprite
    Flags = EF_LIFESPAN_INFINITE | EF_POS_RELATIVE
    StartParticles = 1
    MaxParticles = 1
    ParticleRate = 0.000000
    Sprite = data\sprites\powerups\gem.spr
    LifespanMin = 1.000000
    LifespanMax = 1.000000
    EmitterLifespan = 1.000000 1.000000
}

Emitter cartouche
{
    Type = Sprite
    Flags = EF_SPRITE_ANIM_LOOP | EF_USE_COLOR_RATE | EF_PALETTE_LOOP | EF_LIFESPAN_INFINITE | EF_POS_RELATIVE
    StartParticles = 1
    MaxParticles = 1
    ParticleRate = 5.000000
    Sprite = data\sprites\powerups\bomb.spr
    ColorRate = 30.000000
    AnimRate = 30
    LifespanMin = 1.000000
    LifespanMax = 1.000000
    EmitterLifespan = 2.000000 2.000000
}

Emitter sparks
{
    Type = Sprite
    Flags = EF_SPRITE_ANIM_LOOP | EF_ELIFESPAN_INFINITE
    StartParticles = 7
    MaxParticles = 15
    ParticleRate = 25.000000
    Sprite = data\sprites\particles\spark_ball_2.spr
    AnimRate = 60
    FadeOutStartTime = 0.500000
    LifespanMin = 0.750000
    LifespanMax = 1.000000
    SpawnRadiusMin = -16.000000 -16.000000
    SpawnRadiusMax = 16.000000 16.000000
    Acc = 0.000000 -200.000000
    DevAngle = -200.000000 200.000000
    EmitterLifespan = 0.000000 0.000000
}
```

### Example 2: Extra Life Burst (`extra_life.psys`)
A three-stage burst consisting of an initial flash flare, outward sparks, and floating ankh glyphs:

```text
Emitter flash
{
    Type = Sprite
    Flags = EF_NONE
    StartParticles = 1
    MaxParticles = 1
    Sprite = data\sprites\particles\flare_big.spr
    FadeInEndTime = 0.150000
    FadeOutStartTime = 0.500000
    LifespanMin = 0.250000
    LifespanMax = 0.250000
    EmitterLifespan = 0.500000 0.500000
}

Emitter sparks
{
    Type = Sprite
    Flags = EF_SPRITE_ANIM_LOOP | EF_SPRITE_RANDOM_FRAME | EF_VEL_POSRELATIVE | EF_VEL_DEVIATION
    StartParticles = 25
    MaxParticles = 25
    Sprite = data\sprites\particles\spark_ball_5.spr
    AnimRate = 30
    FadeOutStartTime = 0.500000
    LifespanMin = 0.250000
    LifespanMax = 0.750000
    SpawnRadiusMin = -5.000000 -5.000000
    SpawnRadiusMax = 5.000000 5.000000
    StartVelMin = 45.000000 45.000000
    StartVelMax = 60.000000 60.000000
    DevAngle = -500.000000 700.000000
    EmitterLifespan = 0.500000 0.500000
}

Emitter ankh
{
    Type = Sprite
    Flags = EF_NONE
    StartParticles = 2
    MaxParticles = 2
    Sprite = data\sprites\particles\ankh.spr
    FadeOutStartTime = 0.800000
    LifespanMin = 2.000000
    LifespanMax = 2.000000
    StartVelMin = 0.000000 -25.000000
    StartVelMax = 0.000000 -25.000000
    EmitterLifespan = 0.250000 0.250000
}
```
