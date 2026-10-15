# MumboJumbo 2D Particle System Engine Architecture

This document details the reverse-engineered runtime architecture, internal data structures, physics simulation, lifecycle management, and rendering pipeline of the particle system engine in `gfx2d.dll`, as used in *Luxor*, *Luxor: Amun Rising*, and *Luxor Mahjong*.

---

## 1. Engine Subsystem Overview

The particle engine is implemented in C++ across four primary modules in `gfx2d.dll`:
- `gfxparticlesystem.cpp` & `gfxparticlesystemfactory.cpp`: Descriptor parsing, loading, caching, and template storage (`gfxParticleSystem`).
- `gfxparticlesysteminstance.cpp`: Runtime simulation, physics integration, emitter state, and rendering (`gfxParticleSystemInstance`).
- `gfxpalette.cpp` & `gfxpalettefactory.cpp`: 256×16 color gradient textures and 1D color sampling (`gfxPalette`).
- `gfxspline.cpp` & `gfxsplinefactory.cpp`: Parametric spline curve evaluation for emitter paths (`gfxSpline`).

```
+---------------------------------------------------------------------------------+
|                                  gfxSubsystem                                   |
+---------------------------------------------------------------------------------+
           |                                                    |
           v                                                    v
+-------------------------------+              +----------------------------------+
|   gfxParticleSystemFactory    |              |     Global Particle Pool         |
| (Resource Cache / Hash Table) |              |  (10,240 nodes, 1,105,920 bytes) |
+-------------------------------+              +----------------------------------+
           | (creates template)                                 |
           v                                                    v
+-------------------------------+              +----------------------------------+
|       gfxParticleSystem       |              |       Free Particle List         |
|  Linked List of _EmitterData  |              |        (m_pFreeParticleList)     |
+-------------------------------+              +----------------------------------+
           | (instantiates)                                     | (alloc / free)
           v                                                    v
+---------------------------------------------------------------------------------+
|                            gfxParticleSystemInstance                            |
|  - Emitter instances array (_EmitterInstance_s[m_NumEmitters])                 |
|  - Active particle doubly linked lists (m_pParticles)                           |
|  - Promoted child instances hierarchy (m_pChildInstances)                       |
+---------------------------------------------------------------------------------+
```

### Static Preallocation Architecture
To avoid heap fragmentation during intensive arcade gameplay (such as chained marble explosions, lightning strikes, and multi-emitter scarab bursts), the engine allocates a static pool of **10,240 particles** (`0x10e000` bytes) at subsystem initialization:
- **Node Size**: 108 bytes (`0x6c`) per particle.
- **Allocation Mechanism**: $O(1)$ pop from the global free list head (`0x10017c94`).
- **Deallocation Mechanism**: $O(1)$ push back onto the global free list head (`0x10017c94`).
- **Zero Runtime Allocations**: Particle emission and death never invoke `malloc` or `free`.

---

## 2. Memory Layouts & Data Structures

### 2.1 Emitter Configuration Descriptor: `_EmitterData_s`
- **Total Size**: 984 bytes (`0x3d8`)
- **Location**: Allocated on heap during `.psys` loading; stored as a singly linked list in `gfxParticleSystem`.

| Offset | Type | Field Name | Description |
|---|---|---|---|
| `+0x000` | `char[128]` | `m_szName` | Emitter name string (null-terminated). |
| `+0x080` | `char[128]` | `m_szGroup` | Sub-emitter / parent group name. |
| `+0x100` | `uint8_t` | `m_nType` | Emitter type: `0` = Sprite, `1` = Point. |
| `+0x101` | `uint8_t[3]` | `_pad0` | Alignment padding. |
| `+0x104` | `uint32_t` | `m_nFlags` | Emitter flags bitmask (`EF_*`). |
| `+0x108` | `uint16_t` | `m_nStartParticles`| Initial burst particle count. |
| `+0x10a` | `uint16_t` | `m_nMaxParticles` | Maximum simultaneous active particles. |
| `+0x10c` | `float` | `m_fParticleRate` | Emission frequency (particles per second). |
| `+0x110` | `char[128]` | `m_szSprite` | Bound sprite resource path. |
| `+0x190` | `int32_t` | `m_nSpriteId` | Factory resource ID for sprite (-1 if unassigned). |
| `+0x194` | `char[128]` | `m_szPalette` | Bound palette resource path. |
| `+0x214` | `int32_t` | `m_nPaletteId` | Factory resource ID for palette (-1 if unassigned). |
| `+0x218` | `int32_t` | `m_nAnimMin` | Starting animation track / sequence index. |
| `+0x21c` | `int32_t` | `m_nAnimMax` | Ending animation track / sequence index. |
| `+0x220` | `int32_t` | `m_nAnimRate` | Frame display duration (milliseconds per frame). |
| `+0x224` | `float` | `m_fColorRate` | Palette progression rate multiplier. |
| `+0x228` | `float` | `m_fFadeInEndTime`| Normalized time $[0.0, 1.0]$ when fade-in finishes. |
| `+0x22c` | `float` | `m_fFadeOutStartTime`| Normalized time $[0.0, 1.0]$ when fade-out begins. |
| `+0x230` | `float` | `m_fLifespanMin` | Minimum particle life in seconds. |
| `+0x234` | `float` | `m_fLifespanMax` | Maximum particle life in seconds. |
| `+0x238` | `float` | `m_fRenderDelay` | Delay before rendering begins (seconds). |
| `+0x23c` | `float` | `m_fFirstFrameDelay`| Delay before advancing to subsequent frames. |
| `+0x240` | `float` | `m_fPosX` | Emitter origin X offset. |
| `+0x244` | `float` | `m_fPosY` | Emitter origin Y offset. |
| `+0x248` | `float` | `m_fSpawnRadiusMinX`| Minimum elliptical spawn radius X. |
| `+0x24c` | `float` | `m_fSpawnRadiusMinY`| Minimum elliptical spawn radius Y. |
| `+0x250` | `float` | `m_fSpawnRadiusMaxX`| Maximum elliptical spawn radius X. |
| `+0x254` | `float` | `m_fSpawnRadiusMaxY`| Maximum elliptical spawn radius Y. |
| `+0x258` | `float` | `m_fStartVelMinX` | Minimum initial velocity X. |
| `+0x25c` | `float` | `m_fStartVelMinY` | Minimum initial velocity Y. |
| `+0x260` | `float` | `m_fStartVelMaxX` | Maximum initial velocity X. |
| `+0x264` | `float` | `m_fStartVelMaxY` | Maximum initial velocity Y. |
| `+0x268` | `float` | `m_fAccX` | Constant linear acceleration X. |
| `+0x26c` | `float` | `m_fAccY` | Constant linear acceleration Y. |
| `+0x270` | `float` | `m_fRotMin` | Minimum initial orientation angle (degrees). |
| `+0x274` | `float` | `m_fRotMax` | Maximum initial orientation angle (degrees). |
| `+0x278` | `float` | `m_fRotVelMin` | Minimum rotational speed (degrees/sec). |
| `+0x27c` | `float` | `m_fRotVelMax` | Maximum rotational speed (degrees/sec). |
| `+0x280` | `float` | `m_fRotDragMin` | Minimum rotational friction/drag $[0.0, 1.0]$. |
| `+0x284` | `float` | `m_fRotDragMax` | Maximum rotational friction/drag $[0.0, 1.0]$. |
| `+0x288` | `float` | `m_fDragMin` | Minimum linear velocity drag $[0.0, 1.0]$. |
| `+0x28c` | `float` | `m_fDragMax` | Maximum linear velocity drag $[0.0, 1.0]$. |
| `+0x290` | `float` | `m_fDevDelay` | Delay before directional deviation begins. |
| `+0x294` | `float` | `m_fDevAngleMin` | Minimum directional deviation angle (degrees/sec). |
| `+0x298` | `float` | `m_fDevAngleMax` | Maximum directional deviation angle (degrees/sec). |
| `+0x29c` | `float` | `m_fEmitterVelMinX`| Minimum emitter movement velocity X. |
| `+0x2a0` | `float` | `m_fEmitterVelMinY`| Minimum emitter movement velocity Y. |
| `+0x2a4` | `float` | `m_fEmitterVelMaxX`| Maximum emitter movement velocity X. |
| `+0x2a8` | `float` | `m_fEmitterVelMaxY`| Maximum emitter movement velocity Y. |
| `+0x2ac` | `float` | `m_fEmitterAccX` | Emitter movement acceleration X. |
| `+0x2b0` | `float` | `m_fEmitterAccY` | Emitter movement acceleration Y. |
| `+0x2b4` | `float` | `m_fEmitterStartMin`| Delay before emitter becomes active (min). |
| `+0x2b8` | `float` | `m_fEmitterStartMax`| Delay before emitter becomes active (max). |
| `+0x2bc` | `float` | `m_fEmitterWarmupMin`| Warmup pre-simulation time (min). |
| `+0x2c0` | `float` | `m_fEmitterWarmupMax`| Warmup pre-simulation time (max). |
| `+0x2c4` | `float` | `m_fEmitterLifespanMin`| Emitter operational lifespan (min). |
| `+0x2c8` | `float` | `m_fEmitterLifespanMax`| Emitter operational lifespan (max). |
| `+0x2cc` | `char[128]` | `m_szPromotion` | Successor particle system name triggered on expiration. |
| `+0x34c` | `int32_t` | `m_nPromotionId`| Resource ID of promotion particle system (-1 if none). |
| `+0x350` | `char[128]` | `m_szSpline` | Spline path resource name. |
| `+0x3d0` | `int32_t` | `m_nSplineId` | Resource ID of spline curve (-1 if none). |
| `+0x3d4` | `_EmitterData_s*`| `m_pNext` | Pointer to next emitter descriptor in linked list. |

---

### 2.2 Particle Runtime Node: `_Particle_s`
- **Total Size**: 108 bytes (`0x6c`)
- **Pool**: Allocated from the global array of 10,240 nodes.

| Offset | Type | Field Name | Description |
|---|---|---|---|
| `+0x00` | `float` | `m_fLifespan` | Total particle lifespan (seconds). |
| `+0x04` | `float` | `m_fSpawnTime` | Absolute emitter time when particle was spawned. |
| `+0x08` | `float` | `m_fDieTime` | Absolute time when particle dies (`SpawnTime + Lifespan`). |
| `+0x0c` | `float` | `m_fFadeInEndTime` | Absolute time when particle reaches full opacity. |
| `+0x10` | `float` | `m_fFadeOutStartTime`| Absolute time when particle starts fading out. |
| `+0x14` | `float` | `m_fRenderDelayTime` | Absolute time before particle becomes visible. |
| `+0x18` | `float` | `m_fPosX` | Current X position in world space or emitter-local space. |
| `+0x1c` | `float` | `m_fPosY` | Current Y position in world space or emitter-local space. |
| `+0x20` | `float` | `m_fVelX` | Current linear velocity X. |
| `+0x24` | `float` | `m_fVelY` | Current linear velocity Y. |
| `+0x28` | `float` | `m_fAccX` | Current linear acceleration X (or orbit center X). |
| `+0x2c` | `float` | `m_fAccY` | Current linear acceleration Y (or orbit center Y). |
| `+0x30` | `float` | `m_fAngle` | Current rotation angle in degrees $[0, 360)$. |
| `+0x34` | `float` | `m_fRotVel` | Current rotational speed in degrees/second. |
| `+0x38` | `float` | `m_fRotDrag` | Rotational friction coefficient. |
| `+0x3c` | `float` | `m_fDrag` | Linear velocity friction coefficient. |
| `+0x40` | `float` | `m_fSpawnRadiusX` | Initial elliptical spawn radius offset X. |
| `+0x44` | `float` | `m_fSpawnRadiusY` | Initial elliptical spawn radius offset Y. |
| `+0x48` | `float` | `m_fDevAngle` | Current orbital or heading deviation angle. |
| `+0x4c` | `int32_t` | `m_nAnimTrack` | Assigned sprite animation sequence track. |
| `+0x50` | `float` | `m_fAnimTimer` | Frame duration accumulator (milliseconds). |
| `+0x54` | `float` | `m_fColorPos` | Palette coordinate rate scale factor. |
| `+0x58` | `int32_t` | `m_nFrame` | Current animation frame index. |
| `+0x5c` | `uint8_t` | `m_nR` | Current Red color channel $[0, 255]$. |
| `+0x5d` | `uint8_t` | `m_nG` | Current Green color channel $[0, 255]$. |
| `+0x5e` | `uint8_t` | `m_nB` | Current Blue color channel $[0, 255]$. |
| `+0x5f` | `uint8_t` | `m_nAlpha` | Current Alpha transparency channel $[0, 255]$. |
| `+0x60` | `uint8_t` | `m_nFlags` | Particle state flags (`0x02` = visible, `0x04` = orbit CW/CCW). |
| `+0x61` | `uint8_t[3]` | `_pad` | Alignment padding. |
| `+0x64` | `_Particle_s*` | `m_pNext` | Pointer to next particle in emitter's active list. |
| `+0x68` | `_Particle_s*` | `m_pPrev` | Pointer to previous particle in emitter's active list. |

---

### 2.3 Emitter Runtime Instance: `_EmitterInstance_s`
- **Total Size**: 88 bytes (`0x58`)
- **Location**: Allocated as an array of $N$ elements (`m_NumEmitters`) at `instance + 0x14`.

| Offset | Type | Field Name | Description |
|---|---|---|---|
| `+0x00` | `gfxSprite*` | `m_pSprite` | Cached pointer to resolved sprite object. |
| `+0x04` | `gfxPalette*` | `m_pPalette` | Cached pointer to resolved palette texture. |
| `+0x08` | `gfxSpline*` | `m_pSpline` | Cached pointer to resolved spline curve. |
| `+0x0c` | `float` | `m_fSplineProgress`| Parametric progress along spline $t \in [0.0, 1.0]$. |
| `+0x10` | `_Particle_s*` | `m_pParticles` | Head pointer of active particle doubly linked list. |
| `+0x14` | `_EmitterInstance_s*`| `m_pParentEmitter`| Pointer to parent emitter instance (if sub-emitter). |
| `+0x18` | `int32_t` | `m_nActiveParticles`| Number of active live particles in this emitter. |
| `+0x1c` | `uint8_t` | `m_bPromoted` | Set to 1 once the promotion trigger has fired. |
| `+0x1d` | `uint8_t` | `m_bStarted` | Set to 1 once `m_fEmitterStart` delay has elapsed. |
| `+0x1e` | `uint8_t[2]` | `_pad0` | Alignment padding. |
| `+0x20` | `float` | `m_fParticleRateInv`| Interval between particle spawns ($1.0 / ParticleRate$). |
| `+0x24` | `float` | `m_fLastSpawnTime` | Emitter time of previous particle spawn. |
| `+0x28` | `int32_t` | `m_nOriginX` | Root instance anchor position X. |
| `+0x2c` | `int32_t` | `m_nOriginY` | Root instance anchor position Y. |
| `+0x30` | `float` | `m_fPosX` | Integrated emitter local position X. |
| `+0x34` | `float` | `m_fPosY` | Integrated emitter local position Y. |
| `+0x38` | `float` | `m_fVelX` | Randomized emitter translational velocity X. |
| `+0x3c` | `float` | `m_fVelY` | Randomized emitter translational velocity Y. |
| `+0x40` | `float` | `m_fEmitterLifespan`| Randomized emitter lifespan duration (seconds). |
| `+0x44` | `float` | `m_fEmitterStart` | Randomized start delay (seconds). |
| `+0x48` | `float` | `m_fEmitterWarmup`| Remaining warmup pre-simulation duration. |
| `+0x4c` | `float` | `m_fTotalLifespan`| Total operational limit (`EmitterStart + EmitterLifespan`). |
| `+0x50` | `float` | `m_fTimer` | Elapsed emitter local simulation time (seconds). |
| `+0x54` | `uint8_t` | `m_bStopped` | Flag set to 1 when `stopEmitters()` is called. |
| `+0x55` | `uint8_t` | `m_bActive` | Operational state boolean (can be toggled per emitter). |
| `+0x56` | `uint8_t[2]` | `_pad1` | Alignment padding. |

---

### 2.4 Instance Container: `gfxParticleSystemInstance`
- **Total Size**: 48 bytes (`0x30`)

| Offset | Type | Field Name | Description |
|---|---|---|---|
| `+0x00` | `void*` | `_vptr` | Virtual method table. |
| `+0x04` | `gfxParticleSystemInstance*`| `m_pNextSibling` | Sibling pointer in hierarchy list. |
| `+0x08` | `_ChildInstanceNode*`| `m_pChildList` | Head of promoted child instance linked list. |
| `+0x0c` | `uint8_t` | `m_bDead` | Dead state flag. |
| `+0x10` | `gfxParticleSystem*` | `m_pTemplate` | Pointer to parent template definition. |
| `+0x14` | `_EmitterInstance_s*`| `m_pEmitters` | Array of emitter runtime state structures. |
| `+0x18` | `uint32_t` | `m_nNumEmitters` | Total number of emitters in this instance. |
| `+0x1c` | `PsysCallback` | `m_pfnCallback` | Event callback function pointer. |
| `+0x20` | `void*` | `m_pUserData` | User context pointer passed to callback. |
| `+0x24` | `int32_t` | `m_nX` | Current world position anchor X. |
| `+0x28` | `int32_t` | `m_nY` | Current world position anchor Y. |
| `+0x2c` | `uint8_t` | `m_bJustInitialized` | 1 on first frame; triggers `PE_START` callback. |

---

## 3. Flag Definitions & Bitmasks

Parsed from the `Flags = ...` declaration in `.psys` scripts into `m_nFlags` at `+0x104`:

| Flag Constant | Hex Mask | Binary Bit | Engine Behavior |
|---|---|---|---|
| `EF_NONE` | `0x00000000` | None | Default linear kinematic particle. |
| `EF_SPRITE_ANIM_LOOP` | `0x00000001` | Bit 0 | Sprite animation cycles back to frame 0 after reaching the last frame. |
| `EF_USE_COLOR_RATE` | `0x00000002` | Bit 1 | Palette sample coordinate advances at speed proportional to `ColorRate`. |
| `EF_PALETTE_LOOP` | `0x00000006` | Bits 1, 2 | Automatically sets `EF_USE_COLOR_RATE`; wraps palette index modulo 1.0. |
| `EF_SPRITE_RANDOM_FRAME` | `0x00000008` | Bit 3 | Newly spawned particles select a random starting frame within the anim track. |
| `EF_ACC_ATTRACTION` | `0x00000010` | Bit 4 | Replaces constant linear acceleration with centripetal attractor physics. |
| `EF_VEL_POSRELATIVE` | `0x00000020` | Bit 5 | Initial velocity vector is directed radially outwards from emitter center. |
| `EF_VEL_ORBIT` | `0x00000040` | Bit 6 | Overrides linear motion; locks particles to an elliptical orbit around the emitter. |
| `EF_VEL_TANGENTIAL` | `0x00000080` | Bit 7 | Initial velocity is directed perpendicular (tangential) to the radial spawn vector. |
| `EF_LIFESPAN_INFINITE` | `0x00000100` | Bit 8 | Particles do not expire or fade out until explicitly stopped or killed. |
| `EF_VEL_DEVIATION` | `0x00000200` | Bit 9 | Applies dynamic angular perturbation / wander to the velocity vector over time. |
| `EF_POS_RELATIVE` | `0x00000400` | Bit 10 | Particle positions are relative to the emitter origin (move when the emitter moves). |
| `EF_ELIFESPAN_INFINITE` | `0x00000800` | Bit 11 | Emitter never expires; runs continuously until explicitly stopped. |
| `EF_SOFTWARE` | `0x00001000` | Bit 12 | Requires software rendering pipeline. |
| `EF_HARDWARE` | `0x00002000` | Bit 13 | Requires Direct3D hardware acceleration. |
| `EF_KILL_ON_STOP` | `0x00004000` | Bit 14 | When `stopEmitters()` is invoked, immediately purges all active particles. |

---

## 4. Lifecycle & State Machine

```
  +------------------+
  |  Instance Init   | ---> Spawns StartParticles burst
  +------------------+
           |
           v
  +------------------+      Timer < EmitterStart
  | Waiting / Warmup | -----------------------------+
  +------------------+                              |
           |                                        |
           | Timer >= EmitterStart                  |
           v                                        v
  +------------------+                     [ Skip Emission ]
  | Active Emission  | <----------------------------+
  | (Rate & Physics) |
  +------------------+
           |
           | Timer >= TotalLifespan (and not EF_ELIFESPAN_INFINITE)
           v
  +------------------+      m_PromotionId != -1
  | Emitter Expired  | ------------------------> [ Spawn Child Instance ]
  +------------------+                           [ Invoke Callback PE_PROMOTION ]
           |
           | All particles dead && Child instances dead
           v
  +------------------+
  |   Instance Dead  | ---> isDead() == true ---> Reclaimed by gfxSubsystem
  +------------------+
```

### 4.1 Initialization: `gfxParticleSystemInstance::init` (Address: `0x10009c90`)
1. Allocates an array of `m_NumEmitters * 88` bytes for `m_pEmitters`.
2. Resolves all external asset handles via factories:
   - `m_pSprite` from `gfxSpriteFactory::getSprite(m_nSpriteId)`.
   - `m_pPalette` from `gfxPaletteFactory::getPalette(m_nPaletteId)`.
   - `m_pSpline` from `gfxSplineFactory::getSpline(m_nSplineId)`.
3. Resolves hierarchical group linkages (`m_szGroup`):
   - Looks up parent emitter by name using `gfxParticleSystem::getEmitterId`.
   - Links `m_pParentEmitter` to the matching parent instance.
4. Initializes random initial kinematic states:
   $$\text{Vel}_x = \text{EmitterVelMinX} + \text{rand}() \cdot (\text{EmitterVelMaxX} - \text{EmitterVelMinX})$$
   $$\text{Vel}_y = \text{EmitterVelMinY} + \text{rand}() \cdot (\text{EmitterVelMaxY} - \text{EmitterVelMinY})$$
   $$\text{Start} = \text{EmitterStartMin} + \text{rand}() \cdot (\text{EmitterStartMax} - \text{EmitterStartMin})$$
   $$\text{Lifespan} = \text{EmitterLifespanMin} + \text{rand}() \cdot (\text{EmitterLifespanMax} - \text{EmitterLifespanMin})$$
   $$\text{TotalLifespan} = \text{Start} + \text{Lifespan}$$
5. Sets `m_bJustInitialized = 1`.

### 4.2 Start Particles Burst
If `StartParticles > 0`, the emitter immediately executes a burst spawn loop on its first active frame, popping up to `StartParticles` nodes from the particle pool before continuous accumulator emission begins.

### 4.3 Emission Rate Accumulator
Emission is governed by an inverted delta accumulator:
- If `m_fParticleRate > 0`: $\Delta t_{\text{interval}} = \frac{1.0}{\text{ParticleRate}}$.
- When $(t_{\text{current}} - t_{\text{last\_spawn}}) \ge \Delta t_{\text{interval}}$ and `ActiveParticles < MaxParticles`:
  - Spawns a new particle via `spawnParticle()`.
  - Sets $t_{\text{last\_spawn}} = t_{\text{current}}$.

### 4.4 Termination & Dead State: `isDead()` (Address: `0x10009b60`)
An instance is marked dead and eligible for deletion when:
1. `m_pChildList == NULL` (no active promoted child systems remain).
2. For all $i \in [0, m\_NumEmitters - 1]$:
   - The emitter is stopped (`m_bStopped == 1`) OR
   - The emitter has exceeded its total lifespan ($(t_{\text{current}} \ge \text{TotalLifespan})$ and not `EF_ELIFESPAN_INFINITE`).
   - AND `m_pParticles == NULL` (zero active particles in flight).

### 4.5 Promotion Mechanism (Address: `0x10008f40`)
When an emitter with a valid `m_nPromotionId != -1` reaches `TotalLifespan`:
1. Instantiates the successor particle system using `gfxSubsystem::createParticleSystemInstance`.
2. Inherits current emitter world coordinates:
   $$\text{ChildX} = \text{ParentX} + \text{EmitterPosX}, \quad \text{ChildY} = \text{ParentY} + \text{EmitterPosY}$$
3. Adds the child system to `m_pChildList`.
4. Dispatches the `PE_PROMOTION` event (Code `4`) to the application callback.

### 4.6 Event Notification System: `e_ParticleEvents`
The engine dispatches events through `m_pfnCallback`:
```cpp
void ParticleCallback(void* pUserData, e_ParticleEvents event, gfxParticleSystemInstance* pInstance, int nEmitterId);
```

| Event Constant | Code | Trigger Condition |
|---|---|---|
| `PE_START` | `0` | Dispatched on the very first update frame (`m_bJustInitialized == 1`). `nEmitterId = -1`. |
| `PE_STOP` | `1` | Dispatched when all emitters are stopped. |
| `PE_DIE` | `2` | Dispatched when all particles expire. |
| `PE_BURST` | `3` | Dispatched when a burst spawn completes. |
| `PE_PROMOTION` | `4` | Dispatched when an emitter finishes its lifecycle and triggers a promotion. |

---

## 5. Physical Simulation & Numerical Integration

During `gfxParticleSystemInstance::update(float dt, int origin_x, int origin_y, ...)` (`0x1000a0c0`), the engine iterates over every active particle in the doubly linked list.

```
       Live Particle Update Loop (dt)
                     |
       +-------------+-------------+
       |                           |
       v                           v
[ EF_VEL_ORBIT ]         [ Standard Kinematics ]
- Angle += Speed * dt    - Speed = |V|
- Px = cos(A)*Rx + Ex    - Drag = Speed * DragCoeff
- Py = sin(A)*Ry + Ey    - Accel = BaseAcc + AttractorAcc
                         - V += (Accel - DragDir * Drag) * dt
                         - P += V * dt
                                   |
                                   v
                         [ EF_VEL_DEVIATION ]
                         - Heading += rand(Min, Max) * dt
                         - Rotate V by new heading
```

### 5.1 Standard Linear Kinematics (Newtonian Motion)
1. **Speed & Velocity Drag**:
   $$s = \sqrt{V_x^2 + V_y^2}$$
   $$\vec{F}_{\text{drag}} = - \frac{\vec{V}}{s} \cdot (s \cdot Drag) = - \vec{V} \cdot Drag$$
2. **Acceleration Integration**:
   $$\vec{A}_{\text{total}} = \vec{A} + \vec{F}_{\text{drag}}$$
   $$\vec{V}_{t + \Delta t} = \vec{V}_t + \vec{A}_{\text{total}} \cdot \Delta t$$
3. **Position Integration**:
   $$\vec{P}_{t + \Delta t} = \vec{P}_t + \vec{V}_{t + \Delta t} \cdot \Delta t$$

### 5.2 Attractor Physics (`EF_ACC_ATTRACTION`)
When bit `0x10` is active, particles are pulled toward the emitter center:
1. Distance vector to emitter origin:
   $$\vec{D} = (\text{EmitterPosX} + \text{OriginX}) - P_x, \quad D_y = (\text{EmitterPosY} + \text{OriginY}) - P_y$$
   $$d = \sqrt{D_x^2 + D_y^2}$$
2. Centripetal acceleration (normalized by inverse distance):
   $$A_{\text{attr\_x}} = \frac{D_x}{d} \cdot \text{AccX}, \quad A_{\text{attr\_y}} = \frac{D_y}{d} \cdot \text{AccY}$$
3. Combined with velocity drag:
   $$\vec{V}_{t + \Delta t} = \vec{V}_t + (\vec{A}_{\text{attr}} + \vec{F}_{\text{drag}}) \cdot \Delta t$$

### 5.3 Orbital Mechanics (`EF_VEL_ORBIT`)
When bit `0x40` is active, linear position integration is superseded by locked parametric orbit:
1. Angular velocity advances current orbital angle:
   $$\theta_{t + \Delta t} = \theta_t + (\text{OrbitalSpeed} \cdot \Delta t \cdot \text{Scale})$$
   $$\theta = \text{fmod}(\theta, 360^\circ)$$
2. Direct position evaluation on the ellipse:
   $$P_x = \cos(\theta) \cdot \text{SpawnRadiusX} + \text{EmitterPosX} + \text{OriginX}$$
   $$P_y = \sin(\theta) \cdot \text{SpawnRadiusY} + \text{EmitterPosY} + \text{OriginY}$$

### 5.4 Dynamic Directional Deviation (`EF_VEL_DEVIATION`)
When bit `0x0200` is active and emitter time exceeds `DevDelay`:
1. Random angular wander delta generated:
   $$\Delta \phi = (\text{DevAngleMin} + \text{rand}() \cdot (\text{DevAngleMax} - \text{DevAngleMin})) \cdot \Delta t \cdot \frac{\pi}{180^\circ}$$
2. Heading modified using `fpatan`, `fcos`, and `fsin`:
   $$\phi_{\text{current}} = \text{atan2}(V_y, V_x)$$
   $$\phi_{\text{new}} = \phi_{\text{current}} + \Delta \phi$$
   $$s = \sqrt{V_x^2 + V_y^2}$$
   $$V_x = \cos(\phi_{\text{new}}) \cdot s, \quad V_y = \sin(\phi_{\text{new}}) \cdot s$$

### 5.5 Rotational Dynamics
1. Damped rotational speed:
   $$\omega_{t + \Delta t} = \omega_t - (\omega_t \cdot \text{RotDrag})$$
2. Orientation angle integration:
   $$\alpha_{t + \Delta t} = \alpha_t + \omega_{t + \Delta t} \cdot \Delta t$$
   $$\alpha = \text{fmod}(\alpha + 360^\circ, 360^\circ)$$

---

## 6. Color, Alpha, & Animation Pipelines

### 6.1 Alpha Transparency Envelope
Alpha $\alpha \in [0, 255]$ is calculated using a piecewise-linear 3-phase envelope based on normalized lifetime:

```
 Alpha
  255 +                 +-------------------+
      |                /                     \
      |               /                       \
      |              /                         \
    0 +-------------+---------------------------+-----> Time
                 SpawnTime  FadeInEnd     FadeOutStart  DieTime
```

$$\alpha(t) = \begin{cases}
255 \cdot \left(\frac{t - \text{SpawnTime}}{\text{FadeInEndTime} - \text{SpawnTime}}\right) & \text{if } t < \text{FadeInEndTime} \\
255 & \text{if } \text{FadeInEndTime} \le t < \text{FadeOutStartTime} \\
255 \cdot \left(1.0 - \frac{t - \text{FadeOutStartTime}}{\text{DieTime} - \text{FadeOutStartTime}}\right) & \text{if } t \ge \text{FadeOutStartTime} \text{ and not } \text{EF\_LIFESPAN\_INFINITE} \\
255 & \text{if } \text{EF\_LIFESPAN\_INFINITE}
\end{cases}$$

### 6.2 Palette Sampling (`gfxPalette::getColor`, Address: `0x10003d20`)
A palette is a 256×16 bitmap texture containing horizontal color gradient strips:
1. Coordinate progression parameter $u \in [0.0, 1.0]$:
   $$u = \frac{t_{\text{elapsed}}}{\text{ColorRate} \cdot \text{Lifespan}}$$
2. If `EF_PALETTE_LOOP` is set:
   $$u = \text{fmod}(u, 1.0)$$
3. Linear interpolation between adjacent texels:
   $$x_0 = \lfloor u \cdot 255 \rfloor, \quad x_1 = \min(x_0 + 1, 255)$$
   $$\text{RGB} = \text{Lerp}(\text{Texel}[x_0], \text{Texel}[x_1], \text{frac}(u \cdot 255))$$

### 6.3 Sprite Frame Stepping
1. Accumulator updated:
   $$\text{m\_fAnimTimer} += \Delta t \cdot 1000.0$$
2. If $\text{m\_fAnimTimer} \ge \text{AnimRate}$:
   - $\text{m\_fAnimTimer} = 0$.
   - $\text{m\_nFrame}++$.
   - If $\text{m\_nFrame} \ge \text{TotalTrackFrames}$:
     - If `EF_SPRITE_ANIM_LOOP` is set: $\text{m\_nFrame} = 0$.
     - Else: $\text{m\_nFrame} = \text{TotalTrackFrames} - 1$ (clamp).

---

## 7. Rendering Pipeline: `bltPsys` (Address: `0x1000ac20`)

When rendering each particle:
1. **Visibility Test**:
   - Skips particles if emitter is stopped and `m_bActive == 0`.
   - Skips particles if current time is less than `RenderDelayTime`.
2. **Sprite Dimensions & Pivot Centering**:
   - Retrieves sprite dimensions: $W = \text{m\_pSprite->getWidth()}$, $H = \text{m\_pSprite->getHeight()}$.
   - Computes center pivot offsets: $O_x = W \cdot 0.5$, $O_y = H \cdot 0.5$.
   - Adjusts render coordinates so particles rotate around their visual center:
     $$X_{\text{screen}} = P_x + \text{EmitterOriginX} - O_x$$
     $$Y_{\text{screen}} = P_y + \text{EmitterOriginY} - O_y$$
3. **Modulated Color & Alpha**:
   $$\text{FinalAlpha} = \text{Round}\left(\alpha_{\text{particle}} \cdot \text{SystemAlphaMod}\right)$$
   $$\text{Color} = (\text{FinalAlpha} \ll 24) \mid (\text{R} \ll 16) \mid (\text{G} \ll 8) \mid \text{B}$$
4. **Drawing Invocation**:
   Invokes `gfxSprite::bltXForm` (`ebx + 0x2c`):
   ```cpp
   m_pSprite->bltXForm(
       X_screen, Y_screen,
       particle->m_nAnimTrack,
       particle->m_nFrame,
       &particle->m_nR, // RGB color pointer
       FinalAlpha,
       1.0f,            // Scale
       particle->m_fAngle
   );
   ```
5. **Hierarchical Recursion**:
   Recursively executes `bltPsys` on all active child instances in `m_pChildList`, purging child instances that report `isDead() == true`.

---

## 8. C++ Header Interface (`gfxParticleSystem.h`)

Below is the reverse-engineered C++ header file matching the `gfx2d.dll` binary layouts:

```cpp
#pragma once
#include <cstdint>

// Emitter flag bitmasks
enum e_ParticleFlags : uint32_t {
    EF_NONE                 = 0x00000000,
    EF_SPRITE_ANIM_LOOP     = 0x00000001,
    EF_USE_COLOR_RATE       = 0x00000002,
    EF_PALETTE_LOOP         = 0x00000006, // Bits 1 and 2
    EF_SPRITE_RANDOM_FRAME  = 0x00000008,
    EF_ACC_ATTRACTION       = 0x00000010,
    EF_VEL_POSRELATIVE      = 0x00000020,
    EF_VEL_ORBIT            = 0x00000040,
    EF_VEL_TANGENTIAL       = 0x00000080,
    EF_LIFESPAN_INFINITE    = 0x00000100,
    EF_VEL_DEVIATION        = 0x00000200,
    EF_POS_RELATIVE         = 0x00000400,
    EF_ELIFESPAN_INFINITE   = 0x00000800,
    EF_SOFTWARE             = 0x00001000,
    EF_HARDWARE             = 0x00002000,
    EF_KILL_ON_STOP         = 0x00004000
};

// Event callback notification codes
enum e_ParticleEvents : int32_t {
    PE_START        = 0,
    PE_STOP         = 1,
    PE_DIE          = 2,
    PE_BURST        = 3,
    PE_PROMOTION    = 4
};

// Forward declarations
class gfxSprite;
class gfxPalette;
class gfxSpline;
class gfxParticleSystem;
class gfxParticleSystemInstance;

// Callback signature
typedef void (*PsysEventCallback)(void* pUserData, e_ParticleEvents event, gfxParticleSystemInstance* pInstance, int nEmitterId);

#pragma pack(push, 1)

// Emitter configuration descriptor (984 bytes / 0x3d8)
struct _EmitterData_s {
    char        m_szName[128];              // +0x000
    char        m_szGroup[128];             // +0x080
    uint8_t     m_nType;                    // +0x100 (0=Sprite, 1=Point)
    uint8_t     _pad0[3];                   // +0x101
    uint32_t    m_nFlags;                   // +0x104 (EF_*)
    uint16_t    m_nStartParticles;          // +0x108
    uint16_t    m_nMaxParticles;            // +0x10a
    float       m_fParticleRate;            // +0x10c
    char        m_szSprite[128];            // +0x110
    int32_t     m_nSpriteId;                // +0x190
    char        m_szPalette[128];           // +0x194
    int32_t     m_nPaletteId;               // +0x214
    int32_t     m_nAnimMin;                 // +0x218
    int32_t     m_nAnimMax;                 // +0x21c
    int32_t     m_nAnimRate;                // +0x220
    float       m_fColorRate;               // +0x224
    float       m_fFadeInEndTime;           // +0x228
    float       m_fFadeOutStartTime;         // +0x22c
    float       m_fLifespanMin;             // +0x230
    float       m_fLifespanMax;             // +0x234
    float       m_fRenderDelay;             // +0x238
    float       m_fFirstFrameDelay;         // +0x23c
    float       m_fPosX;                    // +0x240
    float       m_fPosY;                    // +0x244
    float       m_fSpawnRadiusMinX;         // +0x248
    float       m_fSpawnRadiusMinY;         // +0x24c
    float       m_fSpawnRadiusMaxX;         // +0x250
    float       m_fSpawnRadiusMaxY;         // +0x254
    float       m_fStartVelMinX;            // +0x258
    float       m_fStartVelMinY;            // +0x25c
    float       m_fStartVelMaxX;            // +0x260
    float       m_fStartVelMaxY;            // +0x264
    float       m_fAccX;                    // +0x268
    float       m_fAccY;                    // +0x26c
    float       m_fRotMin;                  // +0x270
    float       m_fRotMax;                  // +0x274
    float       m_fRotVelMin;               // +0x278
    float       m_fRotVelMax;               // +0x27c
    float       m_fRotDragMin;              // +0x280
    float       m_fRotDragMax;              // +0x284
    float       m_fDragMin;                 // +0x288
    float       m_fDragMax;                 // +0x28c
    float       m_fDevDelay;                // +0x290
    float       m_fDevAngleMin;             // +0x294
    float       m_fDevAngleMax;             // +0x298
    float       m_fEmitterVelMinX;          // +0x29c
    float       m_fEmitterVelMinY;          // +0x2a0
    float       m_fEmitterVelMaxX;          // +0x2a4
    float       m_fEmitterVelMaxY;          // +0x2a8
    float       m_fEmitterAccX;             // +0x2ac
    float       m_fEmitterAccY;             // +0x2b0
    float       m_fEmitterStartMin;         // +0x2b4
    float       m_fEmitterStartMax;         // +0x2b8
    float       m_fEmitterWarmupMin;        // +0x2bc
    float       m_fEmitterWarmupMax;        // +0x2c0
    float       m_fEmitterLifespanMin;      // +0x2c4
    float       m_fEmitterLifespanMax;      // +0x2c8
    char        m_szPromotion[128];         // +0x2cc
    int32_t     m_nPromotionId;             // +0x34c
    char        m_szSpline[128];            // +0x350
    int32_t     m_nSplineId;                // +0x3d0
    _EmitterData_s* m_pNext;                // +0x3d4
};

// Live particle state structure (108 bytes / 0x6c)
struct _Particle_s {
    float       m_fLifespan;                // +0x00
    float       m_fSpawnTime;               // +0x04
    float       m_fDieTime;                 // +0x08
    float       m_fFadeInEndTime;           // +0x0c
    float       m_fFadeOutStartTime;        // +0x10
    float       m_fRenderDelayTime;         // +0x14
    float       m_fPosX;                    // +0x18
    float       m_fPosY;                    // +0x1c
    float       m_fVelX;                    // +0x20
    float       m_fVelY;                    // +0x24
    float       m_fAccX;                    // +0x28
    float       m_fAccY;                    // +0x2c
    float       m_fAngle;                   // +0x30
    float       m_fRotVel;                  // +0x34
    float       m_fRotDrag;                 // +0x38
    float       m_fDrag;                    // +0x3c
    float       m_fSpawnRadiusX;            // +0x40
    float       m_fSpawnRadiusY;            // +0x44
    float       m_fDevAngle;                // +0x48
    int32_t     m_nAnimTrack;               // +0x4c
    float       m_fAnimTimer;               // +0x50
    float       m_fColorPos;                // +0x54
    int32_t     m_nFrame;                   // +0x58
    uint8_t     m_nR;                       // +0x5c
    uint8_t     m_nG;                       // +0x5d
    uint8_t     m_nB;                       // +0x5e
    uint8_t     m_nAlpha;                   // +0x5f
    uint8_t     m_nFlags;                   // +0x60
    uint8_t     _pad[3];                    // +0x61
    _Particle_s* m_pNext;                   // +0x64
    _Particle_s* m_pPrev;                   // +0x68
};

// Emitter runtime instance state (88 bytes / 0x58)
struct _EmitterInstance_s {
    gfxSprite*  m_pSprite;                  // +0x00
    gfxPalette* m_pPalette;                 // +0x04
    gfxSpline*  m_pSpline;                  // +0x08
    float       m_fSplineProgress;          // +0x0c
    _Particle_s* m_pParticles;              // +0x10
    _EmitterInstance_s* m_pParentEmitter;   // +0x14
    int32_t     m_nActiveParticles;         // +0x18
    uint8_t     m_bPromoted;                // +0x1c
    uint8_t     m_bStarted;                 // +0x1d
    uint8_t     _pad0[2];                   // +0x1e
    float       m_fParticleRateInv;         // +0x20
    float       m_fLastSpawnTime;           // +0x24
    int32_t     m_nOriginX;                 // +0x28
    int32_t     m_nOriginY;                 // +0x2c
    float       m_fPosX;                    // +0x30
    float       m_fPosY;                    // +0x34
    float       m_fVelX;                    // +0x38
    float       m_fVelY;                    // +0x3c
    float       m_fEmitterLifespan;         // +0x40
    float       m_fEmitterStart;            // +0x44
    float       m_fEmitterWarmup;           // +0x48
    float       m_fTotalLifespan;           // +0x4c
    float       m_fTimer;                   // +0x50
    uint8_t     m_bStopped;                 // +0x54
    uint8_t     m_bActive;                  // +0x55
    uint8_t     _pad1[2];                   // +0x56
};

#pragma pack(pop)

// Template definition object (148 bytes / 0x94)
class gfxParticleSystem {
public:
    virtual ~gfxParticleSystem();

    int             load(const char* szFilename);
    void            reset();
    uint8_t         getEmitterCount() const;
    const _EmitterData_s* getEmitterData(int nIndex);
    const char*     getEmitterName(int nIndex);
    int             getEmitterId(const char* szName);
    const char*     getParticleSystemName() const;

private:
    char            m_szName[128];          // +0x04
    _EmitterData_s* m_pFirstEmitter;        // +0x84
    _EmitterData_s* m_pLastEmitter;         // +0x88
    _EmitterData_s* m_pCurrentEmitter;      // +0x8c
    uint8_t         m_nEmitterCount;        // +0x90
};

// Simulation instance object (48 bytes / 0x30)
class gfxParticleSystemInstance {
public:
    virtual ~gfxParticleSystemInstance();

    int             init(gfxParticleSystem* pTemplate,
                         void* pSpriteFactory,
                         void* pPaletteFactory,
                         void* pSplineFactory,
                         PsysEventCallback pfnCallback,
                         void* pUserData);
    void            reset();
    bool            isDead();
    void            startEmitters();
    void            stopEmitters();
    void            offsetParticles(int dx, int dy);
    void            showSpline(int nEmitterId, bool bShow);
    void            update(float dt, int origin_x, int origin_y,
                           void* pSpriteFactory,
                           void* pPaletteFactory,
                           void* pSplineFactory);
    int             bltPsys(float fAlphaMod);

    const gfxParticleSystem* getParticleSystem() const { return m_pTemplate; }

private:
    gfxParticleSystemInstance* m_pNextSibling;      // +0x04
    void*                      m_pChildList;        // +0x08
    uint8_t                    m_bDead;             // +0x0c
    gfxParticleSystem*         m_pTemplate;         // +0x10
    _EmitterInstance_s*        m_pEmitters;         // +0x14
    uint32_t                   m_nNumEmitters;      // +0x18
    PsysEventCallback          m_pfnCallback;       // +0x1c
    void*                      m_pUserData;         // +0x20
    int32_t                    m_nX;                // +0x24
    int32_t                    m_nY;                // +0x28
    uint8_t                    m_bJustInitialized;  // +0x2c
};
```
