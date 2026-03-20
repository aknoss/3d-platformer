# SM64-Style 3D Platformer

## Build

```bash
make          # builds ./platformer
make clean    # removes binary
```

Requires: SDL3 (system-wide via pkg-config), OpenGL, g++

## Architecture

Single-file C++ game (`main.cpp`, ~600 lines). SDL3 for windowing/input, OpenGL 3.3 Core for rendering. No external dependencies beyond SDL3 and system GL.

**Key sections in main.cpp:**
- GL types/constants and manual function loading (no GLAD/GLEW)
- `Vec3`/`Mat4` math structs
- Game structs: `Player`, `Coin`, `Platform`, `Camera`, `Mesh`
- Geometry generators: `gen_box()`, `gen_octagon()`, `gen_hud_digit()`
- Shader compilation and mesh upload/draw helpers
- Level data (hardcoded platforms and coins)
- Main game loop with physics, collision, rendering

## Controls

- W/Up: Move forward
- S/Down: Move backward
- A/Left: Turn left
- D/Right: Turn right
- Space: Jump
- Escape: Quit

## Conventions

- All geometry is flat-shaded colored vertices (no textures)
- Fake directional lighting baked into vertex colors (top bright, sides medium, bottom dark)
- Tank controls (turn + forward/back, no strafing)
- AABB collision with axis-aligned push-out resolution
