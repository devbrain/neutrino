# Neutrino Demoscene Gallery: The Bas van Gaalen Collection

> **An Educational C++20 Reconstruction of Classic 1990s MS-DOS Demoscene Graphics using the `euler` Math Library and the Neutrino Game Engine.**

---

## 1. Historical Context: The Golden Age of PC Demos (1994–1995)

In 1994, personal computing was undergoing a revolution. The IBM PC compatible—historically seen as a dull office spreadsheet machine—became a hotbed of real-time computer graphics, fueled by the demoscene culture that migrated from Commodore 64 and Commodore Amiga to MS-DOS.

One of the most prolific and influential figures of this era was Dutch demo coder **Bas van Gaalen** (Fidonet node `2:285/213.8`, *Brain Made Productions*). In 1994–1995, he released **GFXFX2**, a landmark public-domain archive of over 100 graphics routines written in Borland Turbo Pascal and 16-bit x86 inline assembly. His collection demystified cutting-edge techniques seen in iconic demos like Future Crew's *Second Reality* (1993) and Nova's *Catch That Cow*, teaching a whole generation of programmers how to push 386 and 486 PCs to their absolute limits.

### The Hardware Reality of MS-DOS VGA

In 1994, coders had no 3D hardware acceleration, no programmable shaders, and no floating-point units on standard CPUs (the 386 and 486SX lacked a hardware FPU). Every pixel had to be calculated on the CPU and written into video RAM before the monitor's electron beam drew the next frame:

- **VGA Mode 13h ($320 \times 200$ pixels, 8-bit indexed):** Linear memory mapped at physical address `$A000:0000` (64,000 bytes).
- **256-Color DAC Palette:** The video card DAC (Digital-to-Analog Converter) mapped each 8-bit pixel value ($0..255$) to a 6-bit RGB triple ($0..63$). Hardware color cycling and palette fading were done by rewriting ports `0x3C8` / `0x3C9` rather than redrawing pixels.
- **70 Hz CRT Refresh & Vertical Blanking:** Programs polled I/O port `0x3DA` (bit 3) to wait for the CRT electron beam to travel from the bottom of the tube back to the top (`vretrace`), avoiding screen tearing.
- **Fixed-Point Math & Lookup Tables:** Trigonometry (`sin`, `cos`) was precomputed into 256-element or 360-element integer lookup tables (`sintab`, `ptab`) to bypass slow math operations.

---

## 2. The Pedagogical Mission: Modernizing with `euler` and Neutrino

While the 1994 routines were triumphs of optimization, their underlying mathematical brilliance was frequently obscured by 16-bit register allocations (`dx`, `ax`, `di`, `cx`), segmented memory addresses (`mem[seg:ofs]`), and hardcoded table lookups.

The **Neutrino Demoscene Gallery** ports these routines into expressive, modern **C++20** with two foundational goals:
1. **Mathematical Clarity via `euler`**: Reveal the clean geometry and algebraic formulas underlying each effect.
2. **Engine Architecture via Neutrino**: Demonstrate how retro software-rendering techniques and modern hardware-accelerated vector drawing live harmoniously inside a modern engine.

### How `euler` Transforms Demoscene Math

The [`euler`](https://github.com/devbrain/euler) mathematics library is deeply integrated into every scene:

| 1994 DOS Technique | Why It Was Done That Way | Modern `euler` Equivalent | Educational Benefit |
| :--- | :--- | :--- | :--- |
| `stab[angle mod 255]` | Trig tables avoided 386 FPU penalties. | `euler::degree<float>`, `euler::radian<float>` | **Type safety**: Angle arithmetic is explicit. Radians and degrees can never be accidentally mixed or misused. |
| Ad-hoc integers `x, y, z` | Grouping data in structs had compiler overhead. | `euler::vec2<float>`, `euler::vec3<float>` | Clean vector algebra (`+`, `-`, `*`, `dot`, `cross`, `normalize`). |
| Manual Euler rotation matrices | Trig lookups multiplied across 3 Euler angles. | `euler::quaternion<float>` | **No Gimbal Lock**: Quaternions provide smooth, artifact-free 3D spherical rotations with SLERP interpolation. |
| Nested polar loops | Manual conversion: $x = r \cos \theta$, $y = r \sin \theta$. | `euler::dda::make_polar_curve` & coord transforms | Unifies mathematical definition of polar curves with raster iterators. |

### How Neutrino Powers the Presentation

- **`vga_canvas` (Faithful Software Framebuffer):**
  A modern, high-performance C++ implementation of the Mode 13h display model. It maintains a $320 \times 200$ 8-bit index buffer alongside a 256-color DAC palette. It supports genuine palette cycling and color arithmetic (`add`, `blend`, `dim`), then uploads the buffer to a streaming texture for single-draw-call presentation with crisp pixel scaling.
- **Modern Vector Primitives:**
  For 3D wireframe scenes, stars, and UI gizmos, the gallery uses Neutrino's sub-pixel anti-aliased primitives (`draw_line_aa`, `draw_circle_fill`), elevating vintage demoscene visuals to silky smooth modern displays.
- **Deterministic Fixed-Timestep Loop:**
  Neutrino's `fixed_update(dt, in)` provides frame-rate-independent simulation, ensuring smooth physics and rotations regardless of whether your monitor runs at 60 Hz, 144 Hz, or 240 Hz.

---

## 3. Catalog of Featured Effects

### 1. Comanche Voxel Space (`VOXEL.PAS`)
- **Author:** Jeroen Bouwens & Bas van Gaalen (1994)
- **Math Principle:** Midpoint displacement fractal plasma landscape raymarched forward from screen bottom to horizon using an occlusion horizon buffer (`oldy[320]`).
- **Equation:** Height scaling with perspective divide: $y_{\text{screen}} = y_0 - \frac{H(u, v) \cdot S_v}{256}$.
- **Modern Twist:** Procedural plasma generated into a continuous heightfield; interactive camera altitude, roll, and pitch controls.

### 2. The Second Reality Wormhole (`WORMHOLE.PAS`)
- **Author:** Bas van Gaalen (1994), homage to Future Crew (1993)
- **Math Principle:** Polar coordinate tunnel rendering. Concentric circles with radius $r \in [10, 220]$ centered on dual harmonic Lissajous orbits:
  $$x_c(t) = A_x \cos(\omega_x t), \quad y_c(t) = A_y \sin(\omega_y t)$$
- **Modern Twist:** Expressed through `euler::radian<float>` and polar angle sweeps, with smooth anti-aliased depth shading.

### 3. Translucent Shaded Bobs (`SHADEBOB.PAS`)
- **Author:** Bas van Gaalen (1994)
- **Math Principle:** Additive pixel accumulation. Instead of overwriting screen pixels, translucent stamp kernels add color values to the target buffer:
  $$P(x, y) \leftarrow (P(x, y) + K(u, v)) \pmod{64}$$
  Moving along dual sine paths, the bobs weave glowing, translucent ribbons that accumulate over time.
- **Modern Twist:** Implemented on both `vga_canvas` (authentic 8-bit palette accumulation) and modern floating-point additive blending.

### 4. 3D Rotating Polyhedra & Sphere (`ROTATE1.PAS` & `ROT3.PAS`)
- **Author:** Bas van Gaalen (1994)
- **Math Principle:** 3D coordinates rotated in space and projected to 2D screen coordinates using perspective projection:
  $$x' = x_c + \frac{x \cdot D}{z + z_0}, \quad y' = y_c + \frac{y \cdot D}{z + z_0}$$
- **Modern Twist:** Rotated using `euler::quaternion<float>::from_axis_angle()`, avoiding Euler angle gimbal lock, and rasterized with crisp anti-aliased lines (`draw_line_aa`).

### 5. Multi-Frequency Sine Plasma (`PLASMA1.PAS`)
- **Author:** Bas van Gaalen (1994)
- **Math Principle:** 2D wave superposition of independent sinusoidal frequency fields:
  $$C(x, y) = \sin(k_1 x + \phi_1) + \sin(k_2 y + \phi_2) + \sin(k_3(x + y) + \phi_3)$$
  Coupled with a continuous rotating color palette to produce psychedelic motion with zero pixel redraws.
- **Modern Twist:** High-precision evaluation combined with real-time palette DAC rotation.

### 6. Spherical Refraction Lens (`LENS.PAS`)
- **Author:** Bas van Gaalen (1994)
- **Math Principle:** 2.5D optical refraction simulation. A precomputed hemispherical displacement vector field:
  $$\Delta \vec{p} = \vec{p} \cdot \left(1 - \frac{z}{\text{radius}}\right), \quad z = \sqrt{R^2 - x^2 - y^2}$$
  sweeps across the background, bending pixels behind it.
- **Modern Twist:** Interactive mouse/keyboard movement over dynamic procedural backdrops.

### 7. Warp Perspective Starfield (`STARS.PAS`)
- **Author:** Bas van Gaalen (1994)
- **Math Principle:** 3D star coordinates with exponential speed acceleration, perspective projection, and depth-cued brightness based on $1 / z$.
- **Modern Twist:** Rendered using `euler::vec3<float>` with velocity vector streaks.

### 8. Convection Fire (`3D_FCUBE.PAS`)
- **Author:** Bas van Gaalen & Jare (1994)
- **Math Principle:** Upward thermal cellular automata convection. Pixel heat at $(x, y)$ samples a weighted 4-neighborhood from rows $(y+1)$ and $(y+2)$ displaced by wind, decayed by cooling:
  $$\text{heat}(x, y) = \max\left(0, \frac{h(x-1, y+1) + h(x, y+1) + h(x+1, y+1) + h(x, y+2)}{4} - \text{decay}\right)$$
- **Modern Twist:** Real-time parameterization of wind deflection, cooling rates, ignition bursts, and multiple emitter modes (floor inferno, center pyre, oscillating torches).

### 9. Rainbow Copper Bars (`COPPER5.PAS`)
- **Author:** Bas van Gaalen (1994)
- **Math Principle:** Multi-harmonic oscillating raster bars with additive color blending. Multiple bars oscillate according to independent sinusoidal trajectories:
  $$y_i(t) = y_{\text{mid}} + A_i \sin(\omega_i t + \phi_i)$$
- **Modern Twist:** Cylindrical 3D specular highlight profiles, perspective floor mirror reflection with distance damping, and 3 selectable color themes (Metallic Copper, Synthwave Neon, Rainbow Spectrum).

### 10. DYPP Sinus Scroller (`SCR_DYPP.PAS`)
- **Author:** Bas van Gaalen (1994)
- **Math Principle:** Different-Y-Pixel-Position (DYPP) column modulation. Each vertical column $x$ of a horizontal text strip is displaced vertically by compound sine waves:
  $$y(x, t) = y_{\text{mid}} + A_1 \sin(k_1 x + \omega_1 t) + A_2 \cos(k_2 x + \omega_2 t)$$
- **Modern Twist:** Dynamic 8x8 BIOS font glyph rasterization, 24-pixel tall extruded ribbon with specular crest shading, drop shadow, and horizontal starfield drift.

### 11. Realtime Julia Fractal (`FRACTAL3.PAS` & `FRACZOOM.PAS`)
- **Author:** Bas van Gaalen (1994)
- **Math Principle:** Iterated complex quadratic mapping:
  $$z_{n+1} = z_n^2 + c, \quad z, c \in \mathbb{C}$$
  A point escapes to infinity if $|z_n|^2 = \text{norm}(z_n) > 4$.
- **Modern Twist:** Evaluated in **real time at 60+ FPS** using `euler::complex<float>`, with dynamic orbit parameter morphing $c(t)$, exponential zoom, interactive panning, classic presets (Douady's Rabbit, Frost Dendrite, Dragon, San Marco), and smooth VGA DAC palette cycling.

### 12. Sinusoidal Mesh Warp (`SINMAP.PAS`)
- **Author:** Bas van Gaalen (1994)
- **Math Principle:** 2D sinusoidal coordinate displacement field:
  $$x' = x + A_x \sin(k_y y + \omega t) + 0.35 A_x \cos(k_x x - \omega t)$$
  $$y' = y + A_y \cos(k_x x - \omega t) + 0.30 A_y \sin(k_y y + \omega t)$$
- **Modern Twist:** Real-time inverse mesh mapping with dynamic surface lighting / silk specular sheen derived from the wave slope derivative. Features 3 procedural patterns: authentic 1994 Dutch Flag, demoscene checkerboard, and concentric ripple bullseyes.

### 13. 3D Checkerboard Plane (`3DCHKBRD.PAS`)
- **Author:** Bas van Gaalen & Sean Palmer (1994)
- **Math Principle:** 3D parametric surface mesh with multi-axis rotation and dynamic surface wave displacement:
  $$\vec{v}' = q \cdot (u, v, z(u, v, t)) \cdot q^{-1}, \quad z(u, v, t) = A \sin(\omega_1 u + t) \cos(\omega_2 v + t)$$
- **Modern Twist:** Quaternions via `euler::quaternion<float>`, two-sided Lambertian diffuse lighting with face normals via `euler::cross()`, Painter's Algorithm depth sorting, and multiple wave deformation modes (flat plane, ripple wave, hyperbolic saddle).

### 14. Phosphor Lissajous Trails (`FADEPLOT.PAS`)
- **Author:** Bas van Gaalen (1994)
- **Math Principle:** High-harmonic 3D parametric space curves with in-place framebuffer decay:
  $$x(t) = A_x \sin(\omega_x t + \phi_x), \quad y(t) = A_y \sin(\omega_y t), \quad z(t) = A_z \cos(\omega_z t)$$
  $$\text{pixel}(x, y, t) \leftarrow \max(0, \text{pixel}(x, y, t-1) - \text{decay})$$
- **Modern Twist:** Evaluated with `euler::vec3<float>` and `euler::quaternion<float>`, rendering 3D topological knots (Trefoil knot, 3:4:5 Lissajous ribbon, Torus knot, Figure-8) with simulated CRT phosphor persistence and authentic P1 Green, P3 Amber, and Cyan CRT phosphor palettes.

### 15. 3D Translucent Glass Polyhedra (`3D_TRANS.PAS` & `3D_HOLE.PAS`)
- **Author:** Bas van Gaalen (1994)
- **Math Principle:** Additive translucent polygon rasterization simulating tinted glass:
  $$I(x, y) \leftarrow \min(255, I_{\text{cur}}(x, y) + I_{\text{poly}})$$
  Faces are depth-sorted back-to-front using the Painter's Algorithm. Orientation is tracked with `euler::quaternion<float>`, and face normals are computed via `euler::cross()`.
- **Modern Twist:** Dynamic Fresnel edge brightening, multiple 3D glass geometries (Cube, Octahedron diamond, Hollow hexagonal torus), and switchable color tint themes (Emerald Cyan, Amethyst Ruby, Amber Gold).

### 16. 3D Sine Wave Dot Matrix Field (`DOTS1.PAS` & `DOTS2.PAS`)
- **Author:** Bas van Gaalen (1994)
- **Math Principle:** 850-particle vector dot matrix field modulated by multi-harmonic sinusoidal waves:
  $$\vec{P}_i(t) = q \cdot (x(i, t), y(i, t), z(i, t)) \cdot q^{-1}, \quad x, y, z = \sum A_k \sin(\omega_k i \pm t)$$
- **Modern Twist:** Multi-axis quaternion rotations, depth-cued $1/z$ perspective point magnification and halo scattering, simulated CRT phosphor decay persistence, and multiple field topologies (Undulating Ribbon, Torus Vortex, 3D Cube Grid).

### 17. Bouncing Physics Spheres (`BOUNCE.PAS`)
- **Author:** Bas van Gaalen (1994)
- **Math Principle:** Kinematic bouncing physics with gravity integration, velocity damping, and coefficient of restitution $e$:
  $$y(t) = y_0 + v t - \frac{1}{2} g t^2, \quad v' = -e \cdot v$$
  Volume-preserving squash & stretch deformation upon floor impact: $r_x \cdot r_y = R^2$.
- **Modern Twist:** Perspective elliptical ground shadow projection scaling with altitude, 3D spherical Lambertian diffuse shading with Blinn-Phong specular highlight, multiple interactive spheres, and variable gravity/restitution.

### 18. Electric Field Lines & Equipotential Contours (`FIELD.PAS`)
- **Author:** Bas van Gaalen (1994)
- **Math Principle:** Coulomb's law vector field numerical integration and scalar potential equipotential heatmap:
  $$\vec{E}(\vec{r}) = \sum q_i \frac{\vec{r} - \vec{r}_i}{|\vec{r} - \vec{r}_i|^3}, \quad V(\vec{r}) = \sum \frac{q_i}{|\vec{r} - \vec{r}_i|}$$
- **Modern Twist:** Real-time Runge-Kutta/Euler field line integration emerging from positive charges and terminating on negative charges, animated orbital charge dynamics, and scalar equipotential contour heatmap overlay.

### 19. Spiral & Twister (`SPIRAL.PAS` & `SCR_SPRL.PAS`)
- **Author:** Bas van Gaalen (1994–1995)
- **Math Principle:** 3D Gouraud-shaded twisting column, multi-arm logarithmic polar spiral vortex ($r = |z|, \theta = \arg(z)$ via `euler::complex<float>`), and Amiga-style sine spiral ribbon scroller:
  $$\theta(y, t) = \omega t + y \cdot k_t + A \sin(k_w y)$$
- **Modern Twist:** Scanline-by-scanline 3D twister with cylindrical lighting, multi-frequency sine wobbles, polar spiral vortex, and authentic Amiga 4-face copper color palette.

### 20. Radar Scope Sweep (`SWEEP.PAS`)
- **Author:** Bas van Gaalen (1994)
- **Math Principle:** 360-degree rotating polar beam with Bresenham line sweep, analog CRT phosphor persistence decay, and dynamic radar targets:
  $$x = x_0 + R \cos(\omega t), \quad y = y_0 + R \sin(\omega t), \quad I_{k+1} = \max(0, I_k - \delta)$$
- **Modern Twist:** Rotating beam with soft edge trails, concentric range rings, azimuth crosshairs, dynamic drifting target blips with heading vectors and illuminated phosphor decay trails, and multiple phosphor modes (P31 Green, P20 Amber, Sonar Blue).

### 21. 3D Vectorballs (`BOPS.PAS`)
- **Author:** Bas van Gaalen (1994)
- **Math Principle:** 3D vectorball structures (rotating cube, double helix, torus trefoil knot) rotated with `euler::quaternion<float>` and projected with perspective:
  $$\vec{P}'_i = q \cdot \vec{P}_i \cdot q^{-1}, \quad x_s = \frac{x D}{z + z_0}, \quad r_s = \frac{R D}{z + z_0}$$
- **Modern Twist:** Painter's Algorithm depth sorting for occlusion, perspective $1/z$ radius scaling, 3D spherical Lambertian diffuse + specular highlights, and switchable structures.

### 22. 2D Wave Equation Fluid Caustics (`water.cpp` / `symwater`)
- **Author:** Johan Gardhage & Tran / Renaissance (1990s Demoresearch)
- **Math Principle:** Numerical integration of 2D discrete wave equation finite difference approximation:
  $$h_{t+1}(x, y) = \left(\frac{h_t(x-1, y) + h_t(x+1, y) + h_t(x, y-1) + h_t(x, y+1)}{2} - h_{t-1}(x, y)\right) \cdot d$$
  Surface normal gradient $\nabla h = (\partial h / \partial x, \partial h / \partial y)$, and Snell's law optical ray-bending $(x', y') = (x, y) - r \nabla h$.
- **Modern Twist:** Procedural mosaic pool tiles with dynamic caustic refraction, interactive rain droplets, viscosity damping controls, and multiple fluid themes (Azure Pool, Emerald Lagoon, Molten Lava).

### 23. 2D Scalar Potential Field Metaballs (`metaballs.cpp` / `blobs.cpp`)
- **Author:** Classic demoscene effect (Johan Gardhage & Jeroen Bouwens)
- **Math Principle:** Inverse-square scalar potential field summation:
  $$V(x, y) = \sum_{i=1}^N \frac{R_i^2}{(x - x_i)^2 + (y - y_i)^2 + \epsilon}$$
- **Modern Twist:** Continuous iridescent chrome gradients, solid threshold blobs with specular cores, equipotential contour rings, and multi-frequency harmonic orbital trajectories.

### 24. Realtime Phong Bump Mapping (`bump.cpp` / `bump2d`)
- **Author:** Classic demoscene effect (Gardhage / Turpeau)
- **Math Principle:** Surface normal gradient displacement and Phong specular illumination:
  $$\vec{N} = \left(-\frac{\partial h}{\partial x}, -\frac{\partial h}{\partial y}, 1\right), \quad I = \vec{N} \cdot \vec{L} + (\vec{N} \cdot \vec{H})^\alpha$$
- **Modern Twist:** Fast spherical normal lightmap lookup table, interactive spotlight tracking, and 3 procedural heightmaps (Emblem, Circuit Board, Maze Labyrinth).

### 25. Infinite Plane Texture Rotozoomer (`rotozoom.cpp`)
- **Author:** Classic demoscene effect
- **Math Principle:** Affine 2D coordinate transformation:
  $$\begin{pmatrix} u \\ v \end{pmatrix} = \frac{1}{s(t)} \begin{pmatrix} \cos \theta(t) & -\sin \theta(t) \\ \sin \theta(t) & \cos \theta(t) \end{pmatrix} \begin{pmatrix} x - x_c \\ y - y_c \end{pmatrix}$$
- **Modern Twist:** Constant scanline differential stepping $(\Delta u_x, \Delta v_x)$, 3 procedural textures (Checkerboard, Plasma, Concentric rings), and interactive manual zoom and rotation.

### 26. 3D Textured Cylinder Rototunnel (`rototunnel.cpp`)
- **Author:** Classic demoscene effect
- **Math Principle:** Polar-to-Cartesian cylinder coordinate mapping:
  $$u = \frac{\text{atan2}(y, x)}{\pi} \cdot 128 + u_0(t), \quad v = \frac{D}{\sqrt{x^2 + y^2}} + v_0(t)$$
- **Modern Twist:** Lissajous camera sway displacement, exponential distance fog attenuation, and 3 seamless procedural textures (Stone Brick, Cyber Grid, Star Tunnel).

### 27. Environment-Mapped Chrome Polyhedra (`environcube.cpp`)
- **Author:** Classic demoscene effect
- **Math Principle:** Spherical environment reflection mapping:
  $$\vec{R} = 2(\vec{N} \cdot \vec{V})\vec{N} - \vec{V}, \quad (u, v) = \left(R_x \cdot 115 + 128, R_y \cdot 115 + 128\right)$$
- **Modern Twist:** 3D quaternion rotation via `euler::quaternion<float>`, sub-scanline Gouraud-interpolated triangle rasterization, Painter's algorithm depth sorting, and multiple geometric solids (Chrome Cube, Diamond Octahedron, Hexagonal Ring).

### 28. 3D Strange Attractors (`attractors.c`)
- **Author:** Pierre-Jean Turpeau & E. Lorenz (oldskewlish)
- **Math Principle:** Non-linear chaotic differential systems integrated numerically:
  Lorenz ($\dot{x}=\sigma(y-x), \dot{y}=x(\rho-z)-y, \dot{z}=xy-\beta z$), Pickover 3D, Rössler, and Aizawa attractors.
- **Modern Twist:** 3D quaternion orbit rotation, depth-cued perspective projection, CRT phosphor persistence attenuation decay, and 3x3 anti-aliased luminous point splatting.

### 29. 3D Gravitational Well & Accretion Vortex (`ROT7.PAS`)
- **Author:** Bas van Gaalen (1994)
- **Math Principle:** Warped gravitational potential funnel:
  $$z = -\frac{M}{\sqrt{x^2 + y^2} + \epsilon}, \quad X_p = \frac{X_c Z - X Z_c}{Z - Z_c}$$
- **Modern Twist:** Authentic 169-point discrete grid from `ROT7.PAS`, 3D wireframe spacetime mesh, Keplerian accretion disk particles swirling into the vortex ($v \propto 1/\sqrt{r}$), and authentic 1994 VGA DAC depth palette.

### 30. Compound Sine Wavy Ribbon (`WAVY.PAS`)
- **Author:** Bas van Gaalen (1995)
- **Math Principle:** Multi-harmonic Fourier trigonometric synthesis:
  $$y(x) = A_1 \sin(x + \phi_1) + A_2 \cos(2x + \phi_2) + A_3 \sin(2x + \phi_3) + y_0$$
- **Modern Twist:** Authentic 1994 single oscillating wave, 3D undulating silk ribbon with vertical scanline gradient rasterization, CRT phosphor trail decay, and real-time Fourier harmonic decomposition.

### 31. 100 Additive Transparent Sprites (`SPRITES2.PAS`)
- **Author:** Bas van Gaalen & David Dahl (1995)
- **Math Principle:** Hardware-free additive transparency on 256-color VGA via bit-mask channel combining:
  $$P \leftarrow P_{\text{bg}} \mid S_{\text{rgb}}, \quad \text{Red} (8) \mid \text{Green} (16) \to \text{Yellow}, \quad \text{Red} (8) \mid \text{Blue} (32) \to \text{Magenta}, \quad \dots$$
- **Modern Twist:** 100 concurrent Lissajous figure-8 trajectories, translucent bit 7 (128) backdrop overlay with authentic 1995 message, swirling gravitational whirlpool, and customizable swarm formations.

### 32. Blinking / Twinkling Stars (`STARS2.PAS`)
- **Author:** Bas van Gaalen (1995)
- **Math Principle:** Asynchronous stellar scintillation with 20-phase luminance envelopes and dual $5 \times 5$ diffraction flare bitmasks:
  $$P(x, y) = \text{bitmask}[\text{phase} > 10, \Delta x, \Delta y] + \text{col} + \text{phase}$$
  Stars transition through 6 spectral color classes (Red, Green, Blue, Yellow, Cyan, Diamond White) using a multi-band symmetric VGA DAC ramp.
- **Modern Twist:** Constellation graph synthesis dynamically connecting neighboring stellar peaks with Bresenham lines, natural shooting star meteor streaks, and variable star density (<kbd>+</kbd>/<kbd>-</kbd>).

### 33. 3D Point-Cloud Particle Morphing (`dotmorph.cpp`)
- **Author:** Classic demoscene effect (oldskewlish / Gardhage)
- **Math Principle:** 4,096 3D particles smoothly morphing across geometrical manifolds via quintic Hermite polynomial interpolation ($s(t) = 10t^3 - 15t^4 + 6t^5$):
  $$\vec{P}(t) = (1 - s(t)) \cdot \vec{P}_{\text{src}} + s(t) \cdot \vec{P}_{\text{dst}}$$
  Continuous 3D transformation via `euler::quaternion<float>` with hoisted $3 \times 3$ matrix rotation:
  $$\begin{pmatrix} x' \\ y' \\ z' \end{pmatrix} = \mathbf{M} \begin{pmatrix} x \\ y \\ z \end{pmatrix}, \quad x_s = x_c + \frac{x' D}{z' + Z_0}$$
- **Modern Twist:** Seamless morphing between 5 geometric manifolds (Sphere, Torus, Cube, Double Helix, Trefoil Knot), depth-cued color indexing, CRT phosphor persistence decay, and interactive manual morph controls.

### 34. Optical Moiré Interference Patterns (`moire.c`)
- **Author:** Classic demoscene effect (Turpeau / oldskewlish)
- **Math Principle:** Physical optical interference generated by overlapping periodic geometric lattices:
  Concentric circular wave interference:
  $$I(x, y) = \sin(k \cdot d_1(x, y) - \omega_1 t) + \sin(k \cdot d_2(x, y) - \omega_2 t) + \sin(k \cdot d_3(x, y) - \omega_3 t)$$
  Bitwise XOR fringe lattice:
  $$I_{\text{xor}}(x, y) = \left(\lfloor d_1 / s \rfloor \oplus \lfloor d_2 / s \rfloor\right) \bmod 2$$
- **Modern Twist:** Real-time dual and triple Lissajous moving focal centers, switchable algorithms (Continuous Sine LUT, Flat XOR Rings, Linear Radial Bands, Starburst Rays), and 3 vibrant demoscene palettes.

### 35. Harmonic Spring Line Dance (`linedance.cpp` / `linedance3.cpp`)
- **Author:** Classic demoscene effect
- **Math Principle:** 180-node harmonic coupled spring chain with 4-way kaleidoscopic mirror symmetry:
  $$x_i(t) = x_c + A \sin(\omega_x t + i \delta_x), \quad y_i(t) = y_c + B \cos(\omega_y t + i \delta_y)$$
  Mirrored 4-way across horizontal and vertical axes: $(x, y)$, $(W - x, y)$, $(x, H - y)$, $(W - x, H - y)$.
- **Modern Twist:** Bresenham line connectivity between consecutive nodes, CRT phosphor decay persistence, phase velocity modulation, and interactive spring damping and node density adjustment.

### 36. Bitwise XOR Distance Field Patterns (`xorcircles.cpp`)
- **Author:** Classic demoscene effect
- **Math Principle:** Discrete modular arithmetic and bitwise non-linear mappings:
  Concentric XOR orbital circles:
  $$C(x, y) = \left(\sqrt{(x - x_1)^2 + (y - y_1)^2} \oplus \sqrt{(x - x_2)^2 + (y - y_2)^2}\right) \bmod 256$$
  Classic Sierpinski fractal carpet ($x \oplus y$) and bitwise cellular automata mandalas ($(x \cdot y) \oplus (x + y)$).
- **Modern Twist:** Orbiting focal centers driven by Lissajous trajectories, 4 mathematical XOR formulas, palette cycling animations, and zoom scaling controls.

### 37. Perspective Water Surface Reflection (`reflet.c` / `new_york.c`)
- **Author:** Pierre-Jean Turpeau (oldskewlish)
- **Math Principle:** Ray-bent specular water mirror with compound harmonic wave displacement:
  $$y_s = H_{\text{horizon}} - (y - H_{\text{horizon}}), \quad \Delta x = A \sin(\omega_1 y + \phi_1), \quad \Delta y = B \cos(\omega_2 y + \phi_2)$$
  $$\text{reflected\_pixel}(x, y) = \text{skyline}(x + \Delta x, y_s + \Delta y) \mid 0x80$$
- **Modern Twist:** Dual-bank VGA palette mapping ($0..127$ for crisp procedural skyline, $128..255$ for deep water reflection with exponential distance falloff), 3 procedural skylines (Metropolis, Alpine Sunset, Cosmic Citadel), and interactive ripple amplitude adjustment.

### 38. 3D Bouncing & Rotating Sphere (`ROT10.PAS`)
- **Author:** Bas van Gaalen (1994)
- **Math Principle:** 3D spherical point cloud & latitude/longitude wireframe rings with authentic 1994 parabolic gravity bounce table `ptab[256]`:
  $$X_p = \text{obj}_x + \frac{-x \cdot D}{z - D}, \quad Y_p = 50 + \text{ptab}[\text{pc}] + \frac{-y \cdot D}{z - D}$$
- **Modern Twist:** Wall-to-wall horizontal bounding kinematics with floor bounce, 3D quaternion rotation via `euler::quaternion<float>` with hoisted matrix transforms, depth-cued perspective projection, shadow projection on ground plane, and 3 color themes.

### 39. Mosaic Pixelation & Decimation (`PIXELATE.PAS`)
- **Author:** Bas van Gaalen (1994) & Pierre-Jean Turpeau
- **Math Principle:** Macroblock decimation and decimation dissolve transitions:
  $$c = \text{scene}[(j \cdot z) \cdot W + i \cdot z], \quad \text{block}[y, x] = c \quad (z = 1 \dots 64)$$
- **Modern Twist:** Seamless demoscene dissolve transitions between 4 procedural high-detail scenes (Copper Logo, Polar Tunnel, 3D Checkered Orb, Cosmic Nebula), authentic 1994 top-left block sampling vs center vs area-averaged sampling, dynamic breathing mosaic mode, and discrete power-of-2 decimation ($z = 2, 4, 8, 16, 32, 64$).

### 40. Stretch & Wobble Scroller (`STRSCR.PAS` & `EFFECT4.PAS`)
- **Author:** Bas van Gaalen (1994)
- **Math Principle:** Non-linear scanline expansion using trigonometric differential table `diffsin[64]` and horizontal sine wobble wave displacement `stab[256]`:
  $$\text{offset} = \text{diffsin}[(y + \text{idx}_1 + \text{idx}_2) \ \& \ 63], \quad \text{pos} = \left(\text{add} + \text{repy} + \text{stab}[(\text{idx}_2 + \text{add} + x) \ \& \ 255]\right) \cdot 320$$
  Rubber-band accordion scanline pulling down the screen from `EFFECT4.PAS`.
- **Modern Twist:** Vertical continuous text feed using 8x8 BIOS font glyph rasterization, horizontal pixel doubling ($160 \to 320$ Mode 13h width), copper bar background with CRT phosphor persistence, accordion rubber-band stretcher, and interactive wobble amplitude controls.

### 41. 3D Glenz Vector Polyhedron (`glenzcube.cpp` / Red Sector)
- **Author:** Classic Amiga Megademo (Red Sector) & Johan Gardhage
- **Math Principle:** Additive translucent polygon rasterization:
  $$\vec{P}' = \mathbf{M} \cdot \vec{P}, \quad \text{fb}[y, x] = \min(255, \text{fb}[y, x] + \text{facet\_color})$$
  Both front and back faces are sorted and drawn; overlapping facets create luminous internal seams and gem-like refraction.
- **Modern Twist:** 3D quaternion rotation via `euler::quaternion<float>`, 3 switchable polyhedra (Glenz Cube, Diamond Octahedron, Stella Octangula), bright wireframe edge seams, and 4 crystal color themes.

### 42. 3D Texture-Mapped Cube (`3D_TMAP2.PAS` / `texturecube.cpp`)
- **Author:** Jeroen Bouwens & Bas van Gaalen (1994)
- **Math Principle:** Real-time affine $(u, v)$ scanline texture mapping across 12 triangles forming a 3D solid cube:
  $$u(x) = u_L + (x - x_L) \cdot \Delta u, \quad v(x) = v_L + (x - x_L) \cdot \Delta v, \quad c = \text{tex}[u \ \& \ 63, v \ \& \ 63] \cdot (\vec{N} \cdot \vec{L})$$
- **Modern Twist:** 2D screen cross-product backface culling (`(x1-x0)(y2-y0) - (y1-y0)(x2-x0)`), Lambertian directional lighting, and 3 procedural 64x64 textures (Gold Checkerboard, Cyber Grid, Mandala).

### 43. 3D Fractal Landscape Flight Simulator (`SCAPE.PAS` / `SCAPE2.PAS`)
- **Author:** Bas van Gaalen (1994)
- **Math Principle:** Recursive diamond-square midpoint displacement 2D fractal heightmap generation with perspective column projection:
  $$y_s = y_h - \frac{(H(u, v) - z_{\text{cam}}) \cdot S_z}{d}, \quad \text{draw\_column}(y_s, \text{old\_y}, \text{col})$$
- **Modern Twist:** Full 6-DOF flight camera (pitch, yaw, altitude, speed), altitude zoning (deep water, sand beach, emerald valleys, rocky ridges, snow glaciers), and atmospheric distance fog.

### 44. 3D Waving Cloth & Dot Flag (`dotflag.cpp`)
- **Author:** Johan Gardhage
- **Math Principle:** 4,000-node particle lattice simulating cloth aerodynamics with compound traveling waves:
  $$z(u, v, t) = u^{0.75} \cdot \left[A \sin(k_1 u - \omega_1 t) + B \cos(k_2 v - \omega_2 t)\right]$$
- **Modern Twist:** Specular wind crest highlighting, perspective 3D depth, metallic flagpole finial, and 4 flag designs (Swedish Flag, Jolly Roger, Demoscene Banner, Holographic Pride).

### 45. 3D Concentric Ring Dot Tunnel (`dottunnel.cpp`)
- **Author:** Johan Gardhage
- **Math Principle:** Endless forward camera flight through 48 concentric circular rings of glowing dots:
  $$x_s = x_c + \frac{R \cos \theta + x_{\text{sway}}}{z_i}, \quad y_s = y_c + \frac{R \sin \theta + y_{\text{sway}}}{z_i}$$
- **Modern Twist:** Dynamic compound Lissajous camera sway, continuous ring recycling, spiral twist toggle, and depth-cued particle splatting.

### 46. 2D Mesh Texture Distortion (`distort.cpp` / `distortlogo.cpp`)
- **Author:** Johan Gardhage
- **Math Principle:** Full-screen 2D non-linear displacement coordinate mapping:
  $$(x', y') = (x + \Delta x(x, y, t), y + \Delta y(x, y, t)), \quad \text{fb}[y, x] = \text{src}[y', x']$$
- **Modern Twist:** 4 distortion engines (Liquid Rubber Sheet, Underwater Flag, Polar Vortex, CRT TV Barrel Wobble) with rich procedural demoscene logos and horizons.

### 47. Procedural UV Deformations (`uvmapgen.h` / Inigo Quilez)
- **Author:** Iñigo Quílez & Pierre-Jean Turpeau
- **Math Principle:** Canonical procedural UV-space coordinate transformations:
  - Dual Symmetric Planes: $u = 0.5 x / |y|, v = 0.5 / |y|$
  - Swirl Tunnel: $u = 0.85 / (r + 0.2 \cos(6a)), v = 3a / \pi$
  - Concentric Waves: $v = r + r \sin(10r)$
  - TV CRT Barrel Warp: $x(1 + 0.32 y^2), y(1 + 0.42 x^2)$
- **Modern Twist:** Procedural stone brick, checkerboard, and cyber circuit textures, distance fog attenuation, and flight panning controls.

---

## 4. Controls & Interactive HUD

| Key | Action |
| :--- | :--- |
| <kbd>Tab</kbd> / <kbd>PageDown</kbd> | Cycle next demoscene effect (1 to 47) |
| <kbd>Backspace</kbd> / <kbd>PageUp</kbd> | Cycle previous demoscene effect |
| <kbd>1</kbd> – <kbd>9</kbd>, <kbd>0</kbd>, <kbd>-</kbd>, <kbd>=</kbd>, <kbd>[</kbd>, <kbd>]</kbd>, <kbd>\</kbd>, <kbd>;</kbd>, <kbd>'</kbd>, <kbd>,</kbd>, <kbd>.</kbd>, <kbd>/</kbd> | Direct jump to effect 1 through 20 |
| <kbd>F1</kbd> | Toggle educational HUD (shows math formulas, 1994 notes, live FPS) |
| <kbd>Up</kbd> / <kbd>Down</kbd> / <kbd>Left</kbd> / <kbd>Right</kbd> | Interactive effect controls (pitch/speed/warp/pan/freq/rotation) |
| <kbd>W</kbd> / <kbd>S</kbd> | Speed throttle or 3D camera zoom in / out |
| <kbd>Space</kbd> | Primary effect action (geometry toggle / palette cycle / morph / spawn) |
| <kbd>C</kbd> | Color theme switch (in supported effects) |
| <kbd>R</kbd> | Reset effect state / camera to defaults |

---

## 5. Building & Running

### Prerequisites
- C++20 compatible compiler (GCC 12+, Clang 15+, or Apple Clang)
- CMake 3.24+
- Ninja build system (recommended)

### Build Commands

```bash
# Configure debug or release build
cmake -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug

# Compile the demoscene gallery executable
cmake --build cmake-build-debug --target neutrino_demo_demoscene

# Run the gallery
./cmake-build-debug/bin/neutrino_demo_demoscene
```

---

## Credits & Acknowledgments

- **Original Pascal/ASM Source Codes:** Bas van Gaalen (`2:285/213.8`), Jeroen Bouwens, Sven van Heel, and the 1990s demoscene community.
- **Modern C++20 Engine:** The Neutrino Game Engine team.
- **Mathematical Foundations:** The `euler` mathematical library.
