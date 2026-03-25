#ifndef GEOMETRY_H
#define GEOMETRY_H
#include "math.h"
#include <cmath>
#include <vector>

inline void push_vert(std::vector<float> &buf, float x, float y, float z,
                      float r, float g, float b) {
  buf.push_back(x);
  buf.push_back(y);
  buf.push_back(z);
  buf.push_back(r);
  buf.push_back(g);
  buf.push_back(b);
}

inline std::vector<float> gen_box(Vec3 mn, Vec3 mx, Vec3 col) {
  std::vector<float> buf;
  buf.reserve(36 * 6);

  float top_f = 1.1f, side_f = 0.8f, bot_f = 0.5f;
  Vec3 ct = {col.x * top_f, col.y * top_f, col.z * top_f};
  Vec3 cs = {col.x * side_f, col.y * side_f, col.z * side_f};
  Vec3 cb = {col.x * bot_f, col.y * bot_f, col.z * bot_f};
  auto clamp01 = [](float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); };
  ct = {clamp01(ct.x), clamp01(ct.y), clamp01(ct.z)};
  cs = {clamp01(cs.x), clamp01(cs.y), clamp01(cs.z)};
  cb = {clamp01(cb.x), clamp01(cb.y), clamp01(cb.z)};

  float x0 = mn.x, y0 = mn.y, z0 = mn.z;
  float x1 = mx.x, y1 = mx.y, z1 = mx.z;

  // Top face (+Y)
  push_vert(buf, x0, y1, z0, ct.x, ct.y, ct.z);
  push_vert(buf, x1, y1, z0, ct.x, ct.y, ct.z);
  push_vert(buf, x1, y1, z1, ct.x, ct.y, ct.z);
  push_vert(buf, x0, y1, z0, ct.x, ct.y, ct.z);
  push_vert(buf, x1, y1, z1, ct.x, ct.y, ct.z);
  push_vert(buf, x0, y1, z1, ct.x, ct.y, ct.z);

  // Bottom face (-Y)
  push_vert(buf, x0, y0, z1, cb.x, cb.y, cb.z);
  push_vert(buf, x1, y0, z1, cb.x, cb.y, cb.z);
  push_vert(buf, x1, y0, z0, cb.x, cb.y, cb.z);
  push_vert(buf, x0, y0, z1, cb.x, cb.y, cb.z);
  push_vert(buf, x1, y0, z0, cb.x, cb.y, cb.z);
  push_vert(buf, x0, y0, z0, cb.x, cb.y, cb.z);

  // Front face (+Z)
  push_vert(buf, x0, y0, z1, cs.x, cs.y, cs.z);
  push_vert(buf, x1, y0, z1, cs.x, cs.y, cs.z);
  push_vert(buf, x1, y1, z1, cs.x, cs.y, cs.z);
  push_vert(buf, x0, y0, z1, cs.x, cs.y, cs.z);
  push_vert(buf, x1, y1, z1, cs.x, cs.y, cs.z);
  push_vert(buf, x0, y1, z1, cs.x, cs.y, cs.z);

  // Back face (-Z)
  push_vert(buf, x1, y0, z0, cs.x, cs.y, cs.z);
  push_vert(buf, x0, y0, z0, cs.x, cs.y, cs.z);
  push_vert(buf, x0, y1, z0, cs.x, cs.y, cs.z);
  push_vert(buf, x1, y0, z0, cs.x, cs.y, cs.z);
  push_vert(buf, x0, y1, z0, cs.x, cs.y, cs.z);
  push_vert(buf, x1, y1, z0, cs.x, cs.y, cs.z);

  // Right face (+X)
  Vec3 cs2 = {clamp01(col.x * 0.7f), clamp01(col.y * 0.7f),
              clamp01(col.z * 0.7f)};
  push_vert(buf, x1, y0, z1, cs2.x, cs2.y, cs2.z);
  push_vert(buf, x1, y0, z0, cs2.x, cs2.y, cs2.z);
  push_vert(buf, x1, y1, z0, cs2.x, cs2.y, cs2.z);
  push_vert(buf, x1, y0, z1, cs2.x, cs2.y, cs2.z);
  push_vert(buf, x1, y1, z0, cs2.x, cs2.y, cs2.z);
  push_vert(buf, x1, y1, z1, cs2.x, cs2.y, cs2.z);

  // Left face (-X)
  push_vert(buf, x0, y0, z0, cs2.x, cs2.y, cs2.z);
  push_vert(buf, x0, y0, z1, cs2.x, cs2.y, cs2.z);
  push_vert(buf, x0, y1, z1, cs2.x, cs2.y, cs2.z);
  push_vert(buf, x0, y0, z0, cs2.x, cs2.y, cs2.z);
  push_vert(buf, x0, y1, z1, cs2.x, cs2.y, cs2.z);
  push_vert(buf, x0, y1, z0, cs2.x, cs2.y, cs2.z);

  return buf;
}

inline std::vector<float> gen_octagon(float radius, float thickness, Vec3 col) {
  std::vector<float> buf;
  const int SIDES = 8;
  Vec3 bright = {col.x * 1.1f < 1 ? col.x * 1.1f : 1.0f,
                 col.y * 1.1f < 1 ? col.y * 1.1f : 1.0f,
                 col.z * 1.1f < 1 ? col.z * 1.1f : 1.0f};
  Vec3 dark = {col.x * 0.7f, col.y * 0.7f, col.z * 0.7f};

  for (int i = 0; i < SIDES; i++) {
    float a0 = (float)i / SIDES * 2.0f * (float)M_PI;
    float a1 = (float)(i + 1) / SIDES * 2.0f * (float)M_PI;
    push_vert(buf, 0, 0, thickness * 0.5f, bright.x, bright.y, bright.z);
    push_vert(buf, cosf(a0) * radius, sinf(a0) * radius, thickness * 0.5f,
              col.x, col.y, col.z);
    push_vert(buf, cosf(a1) * radius, sinf(a1) * radius, thickness * 0.5f,
              col.x, col.y, col.z);
  }
  for (int i = 0; i < SIDES; i++) {
    float a0 = (float)i / SIDES * 2.0f * (float)M_PI;
    float a1 = (float)(i + 1) / SIDES * 2.0f * (float)M_PI;
    push_vert(buf, 0, 0, -thickness * 0.5f, dark.x, dark.y, dark.z);
    push_vert(buf, cosf(a1) * radius, sinf(a1) * radius, -thickness * 0.5f,
              dark.x, dark.y, dark.z);
    push_vert(buf, cosf(a0) * radius, sinf(a0) * radius, -thickness * 0.5f,
              dark.x, dark.y, dark.z);
  }
  for (int i = 0; i < SIDES; i++) {
    float a0 = (float)i / SIDES * 2.0f * (float)M_PI;
    float a1 = (float)(i + 1) / SIDES * 2.0f * (float)M_PI;
    float cx0 = cosf(a0) * radius, sy0 = sinf(a0) * radius;
    float cx1 = cosf(a1) * radius, sy1 = sinf(a1) * radius;
    push_vert(buf, cx0, sy0, thickness * 0.5f, col.x * 0.6f, col.y * 0.6f,
              col.z * 0.6f);
    push_vert(buf, cx1, sy1, thickness * 0.5f, col.x * 0.6f, col.y * 0.6f,
              col.z * 0.6f);
    push_vert(buf, cx1, sy1, -thickness * 0.5f, col.x * 0.6f, col.y * 0.6f,
              col.z * 0.6f);
    push_vert(buf, cx0, sy0, thickness * 0.5f, col.x * 0.6f, col.y * 0.6f,
              col.z * 0.6f);
    push_vert(buf, cx1, sy1, -thickness * 0.5f, col.x * 0.6f, col.y * 0.6f,
              col.z * 0.6f);
    push_vert(buf, cx0, sy0, -thickness * 0.5f, col.x * 0.6f, col.y * 0.6f,
              col.z * 0.6f);
  }

  return buf;
}

inline constexpr bool SEGMENTS[10][7] = {
    {1, 1, 1, 1, 1, 1, 0}, // 0
    {0, 1, 1, 0, 0, 0, 0}, // 1
    {1, 1, 0, 1, 1, 0, 1}, // 2
    {1, 1, 1, 1, 0, 0, 1}, // 3
    {0, 1, 1, 0, 0, 1, 1}, // 4
    {1, 0, 1, 1, 0, 1, 1}, // 5
    {1, 0, 1, 1, 1, 1, 1}, // 6
    {1, 1, 1, 0, 0, 0, 0}, // 7
    {1, 1, 1, 1, 1, 1, 1}, // 8
    {1, 1, 1, 1, 0, 1, 1}, // 9
};

inline std::vector<float> gen_hud_digit(int digit, float ox, float oy,
                                        float scale) {
  std::vector<float> buf;
  if (digit < 0 || digit > 9)
    return buf;

  float w = 4.0f * scale;
  float h = 2.0f * scale;
  float dw = w + 2.0f * scale;
  float dh = dw * 2.0f;
  (void)dh;

  struct SegRect {
    float x, y, sw, sh;
  };

  float sw = w, sh = h;
  float vw = h, vh = w;

  SegRect segs[7] = {
      {ox + scale, oy + 2 * w + 2 * scale, sw, sh},
      {ox + w + scale, oy + w + 2 * scale, vw, vh},
      {ox + w + scale, oy + scale, vw, vh},
      {ox + scale, oy, sw, sh},
      {ox, oy + scale, vw, vh},
      {ox, oy + w + 2 * scale, vw, vh},
      {ox + scale, oy + w + scale, sw, sh},
  };

  Vec3 col = {1.0f, 1.0f, 1.0f};

  for (int i = 0; i < 7; i++) {
    if (!SEGMENTS[digit][i])
      continue;
    float sx = segs[i].x, sy = segs[i].y;
    float ex = sx + segs[i].sw, ey = sy + segs[i].sh;
    push_vert(buf, sx, sy, 0, col.x, col.y, col.z);
    push_vert(buf, ex, sy, 0, col.x, col.y, col.z);
    push_vert(buf, ex, ey, 0, col.x, col.y, col.z);
    push_vert(buf, sx, sy, 0, col.x, col.y, col.z);
    push_vert(buf, ex, ey, 0, col.x, col.y, col.z);
    push_vert(buf, sx, ey, 0, col.x, col.y, col.z);
  }
  return buf;
}

inline std::vector<float> gen_hud_coin_icon(float cx, float cy, float radius) {
  std::vector<float> buf;
  const int SIDES = 8;
  Vec3 col = {1.0f, 0.85f, 0.0f};
  for (int i = 0; i < SIDES; i++) {
    float a0 = (float)i / SIDES * 2.0f * (float)M_PI;
    float a1 = (float)(i + 1) / SIDES * 2.0f * (float)M_PI;
    push_vert(buf, cx, cy, 0, col.x * 1.1f, col.y * 1.1f, col.z);
    push_vert(buf, cx + cosf(a0) * radius, cy + sinf(a0) * radius, 0, col.x,
              col.y, col.z);
    push_vert(buf, cx + cosf(a1) * radius, cy + sinf(a1) * radius, 0, col.x,
              col.y, col.z);
  }
  return buf;
}

inline std::vector<float> gen_star(float radius, float thickness, Vec3 col) {
  std::vector<float> buf;
  float inner_radius = radius * 0.4f;

  auto clamp01 = [](float v) {
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
  };
  Vec3 bright = {clamp01(col.x * 1.2f), clamp01(col.y * 1.2f),
                 clamp01(col.z * 1.2f)};
  Vec3 dark = {col.x * 0.5f, col.y * 0.5f, col.z * 0.5f};
  Vec3 side = {col.x * 0.7f, col.y * 0.7f, col.z * 0.7f};

  // Build 10 vertices of the star outline (alternating outer/inner)
  // Star is in the XY plane (stands upright), Z is thickness
  float sx[10], sy[10];
  for (int i = 0; i < 10; i++) {
    float angle = (float)i / 10.0f * 2.0f * (float)M_PI - (float)M_PI / 2.0f;
    float r = (i % 2 == 0) ? radius : inner_radius;
    sx[i] = cosf(angle) * r;
    sy[i] = sinf(angle) * r;
  }

  float ht = thickness * 0.5f;

  // Front face (+Z)
  for (int i = 0; i < 10; i++) {
    int j = (i + 1) % 10;
    push_vert(buf, 0, 0, ht, bright.x, bright.y, bright.z);
    push_vert(buf, sx[i], sy[i], ht, col.x, col.y, col.z);
    push_vert(buf, sx[j], sy[j], ht, col.x, col.y, col.z);
  }

  // Back face (-Z, reversed winding)
  for (int i = 0; i < 10; i++) {
    int j = (i + 1) % 10;
    push_vert(buf, 0, 0, -ht, dark.x, dark.y, dark.z);
    push_vert(buf, sx[j], sy[j], -ht, dark.x, dark.y, dark.z);
    push_vert(buf, sx[i], sy[i], -ht, dark.x, dark.y, dark.z);
  }

  // Edge faces (connecting front and back)
  for (int i = 0; i < 10; i++) {
    int j = (i + 1) % 10;
    push_vert(buf, sx[i], sy[i], ht, side.x, side.y, side.z);
    push_vert(buf, sx[j], sy[j], ht, side.x, side.y, side.z);
    push_vert(buf, sx[j], sy[j], -ht, side.x, side.y, side.z);
    push_vert(buf, sx[i], sy[i], ht, side.x, side.y, side.z);
    push_vert(buf, sx[j], sy[j], -ht, side.x, side.y, side.z);
    push_vert(buf, sx[i], sy[i], -ht, side.x, side.y, side.z);
  }

  return buf;
}

#endif // GEOMETRY_H
