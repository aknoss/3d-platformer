#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cstdint>

inline constexpr int SCREEN_W = 800;
inline constexpr int SCREEN_H = 600;
inline constexpr float MOVE_SPEED = 5.0f;
inline constexpr float TURN_SPEED = 3.0f;
inline constexpr float GRAVITY = -15.0f;
inline constexpr float JUMP_VEL = 8.0f;
inline constexpr float CAM_DIST = 6.0f;
inline constexpr float CAM_HEIGHT = 3.0f;
inline constexpr float CAM_LERP = 5.0f;
inline constexpr float FOV = 60.0f;
inline constexpr float NEAR_PLANE = 0.1f;
inline constexpr float FAR_PLANE = 100.0f;
inline constexpr float MOUSE_SENS = 0.003f;
inline constexpr float COIN_COLLECT_DIST = 1.2f;
inline constexpr float RESPAWN_Y = -10.0f;
// ~16.67ms per frame (60 FPS cap)
inline constexpr uint64_t FRAME_TIME_NS = 1000000000ULL / 60; 

#endif // CONSTANTS_H
