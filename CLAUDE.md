# SM64-Style 3D Platformer

## Build

```bash
make          # builds ./game
make clean    # removes binary and .o files
```

Requires: SDL3 (system-wide via pkg-config), OpenGL, g++

## Architecture

Multi-file C++ game. SDL3 for windowing/input, OpenGL 3.3 Core for rendering. Uses cgltf (vendored, header-only) for GLB model loading.

**File layout:**

| File                 | Contents                                                              |
| -------------------- | --------------------------------------------------------------------- |
| `src/math.h`         | Vec3, Mat4, mat4\_\* helpers (header-only)                            |
| `src/constants.h`    | All game constants as `inline constexpr` (header-only)                |
| `src/geometry.h`     | push*vert, gen_box, gen_octagon, gen_hud*\*, SEGMENTS (header-only)   |
| `src/renderer.h/cpp` | Mesh (RAII VAO/VBO), ShaderProgram (RAII, factory `create_default()`) |
| `src/camera.h`       | Camera — smooth third-person follow (header-only)                     |
| `src/player.h/cpp`   | PlatformData, aabb_overlap, Player                                    |
| `src/level.h/cpp`    | Coin, Level — owns platforms and coins                                |
| `src/hud.h/cpp`      | HUD — cached coin counter display                                     |
| `src/glb_loader.h`   | GLB/glTF model loading via cgltf (header-only)                        |
| `src/main.cpp`       | SDL init, GL context, game loop                                       |

All headers use `#ifndef`/`#define`/`#endif` guards.

**GL isolation:** Only `renderer.cpp`, `hud.cpp`, and `main.cpp` include GL headers. Shader sources and `compile_shader()` are private to `renderer.cpp`.

## Controls

- W/Up: Move forward (camera-relative)
- S/Down: Move backward (camera-relative)
- A/Left: Strafe left (camera-relative)
- D/Right: Strafe right (camera-relative)
- Mouse: Rotate camera
- Space: Jump
- Escape: Quit

## Formatting

Run `clang-format` after any code changes:

```bash
clang-format -i src/*.cpp src/*.h
```

## Conventions

- Player model loaded from GLB (assets/character.glb, embedded at compile time via xxd)
- All geometry is flat-shaded colored vertices (no textures)
- Fake directional lighting baked into vertex colors (top bright, sides medium, bottom dark)
- Camera-relative movement with mouse look; player faces movement direction
- AABB collision with axis-aligned push-out resolution
