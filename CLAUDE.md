# SM64-Style 3D Platformer

## Build

```bash
make          # builds ./game
make clean    # removes binary and .o files
```

Requires: SDL3 (system-wide via pkg-config), OpenGL, g++

## Architecture

Multi-file C++ game. SDL3 for windowing/input, OpenGL 3.3 Core for rendering. No external dependencies beyond SDL3 and system GL.

**File layout:**

| File | Contents |
|------|----------|
| `src/math.h` | Vec3, Mat4, mat4_* helpers (header-only) |
| `src/constants.h` | All game constants as `inline constexpr` (header-only) |
| `src/geometry.h` | push_vert, gen_box, gen_octagon, gen_hud_*, SEGMENTS (header-only) |
| `src/renderer.h/cpp` | Mesh (RAII VAO/VBO), ShaderProgram (RAII, factory `create_default()`) |
| `src/camera.h` | Camera — smooth third-person follow (header-only) |
| `src/player.h/cpp` | PlatformData, aabb_overlap, Player |
| `src/level.h/cpp` | Coin, Level — owns platforms and coins |
| `src/hud.h/cpp` | HUD — cached coin counter display |
| `src/main.cpp` | SDL init, GL context, game loop |

All headers use both `#pragma once` and `#ifndef`/`#define`/`#endif` guards.

**GL isolation:** Only `renderer.cpp`, `hud.cpp`, and `main.cpp` include GL headers. Shader sources and `compile_shader()` are private to `renderer.cpp`.

## Controls

- W/Up: Move forward (camera-relative)
- S/Down: Move backward (camera-relative)
- A/Left: Strafe left (camera-relative)
- D/Right: Strafe right (camera-relative)
- Mouse: Rotate camera
- Space: Jump
- Escape: Quit

## Conventions

- All geometry is flat-shaded colored vertices (no textures)
- Fake directional lighting baked into vertex colors (top bright, sides medium, bottom dark)
- Camera-relative movement with mouse look; player faces movement direction
- AABB collision with axis-aligned push-out resolution
