#ifndef MATH_H
#define MATH_H
#include <cmath>
#include <cstring>

struct Vec3 {
  float x, y, z;
  Vec3() : x(0), y(0), z(0) {}
  Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
  Vec3 operator+(const Vec3 &o) const { return {x + o.x, y + o.y, z + o.z}; }
  Vec3 operator-(const Vec3 &o) const { return {x - o.x, y - o.y, z - o.z}; }
  Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
  Vec3 operator-() const { return {-x, -y, -z}; }
  float dot(const Vec3 &o) const { return x * o.x + y * o.y + z * o.z; }
  Vec3 cross(const Vec3 &o) const {
    return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x};
  }
  float length() const { return sqrtf(x * x + y * y + z * z); }
  Vec3 normalized() const {
    float l = length();
    if (l < 1e-8f)
      return {0, 0, 0};
    return {x / l, y / l, z / l};
  }
};

struct Mat4 {
  float m[16]; // column-major

  static Mat4 identity() {
    Mat4 r;
    memset(r.m, 0, sizeof(r.m));
    r.m[0] = 1;
    r.m[5] = 1;
    r.m[10] = 1;
    r.m[15] = 1;
    return r;
  }

  Mat4 operator*(const Mat4 &b) const {
    Mat4 r;
    memset(r.m, 0, sizeof(r.m));
    for (int c = 0; c < 4; c++)
      for (int row = 0; row < 4; row++)
        for (int k = 0; k < 4; k++)
          r.m[c * 4 + row] += m[k * 4 + row] * b.m[c * 4 + k];
    return r;
  }
};

inline Mat4 mat4_translate(float tx, float ty, float tz) {
  Mat4 r = Mat4::identity();
  r.m[12] = tx;
  r.m[13] = ty;
  r.m[14] = tz;
  return r;
}
inline Mat4 mat4_translate(Vec3 v) { return mat4_translate(v.x, v.y, v.z); }

inline Mat4 mat4_scale(float s) {
  Mat4 r = Mat4::identity();
  r.m[0] = s;
  r.m[5] = s;
  r.m[10] = s;
  return r;
}

inline Mat4 mat4_rotate_y(float angle) {
  Mat4 r = Mat4::identity();
  float c = cosf(angle), s = sinf(angle);
  r.m[0] = c;
  r.m[2] = s;
  r.m[8] = -s;
  r.m[10] = c;
  return r;
}

inline Mat4 mat4_perspective(float fov_deg, float aspect, float near,
                             float far) {
  Mat4 r;
  memset(r.m, 0, sizeof(r.m));
  float f = 1.0f / tanf(fov_deg * (float)M_PI / 360.0f);
  r.m[0] = f / aspect;
  r.m[5] = f;
  r.m[10] = (far + near) / (near - far);
  r.m[11] = -1.0f;
  r.m[14] = (2.0f * far * near) / (near - far);
  return r;
}

inline Mat4 mat4_ortho(float l, float r_val, float b, float t, float n,
                       float f) {
  Mat4 res;
  memset(res.m, 0, sizeof(res.m));
  res.m[0] = 2.0f / (r_val - l);
  res.m[5] = 2.0f / (t - b);
  res.m[10] = -2.0f / (f - n);
  res.m[12] = -(r_val + l) / (r_val - l);
  res.m[13] = -(t + b) / (t - b);
  res.m[14] = -(f + n) / (f - n);
  res.m[15] = 1.0f;
  return res;
}

inline Mat4 mat4_look_at(Vec3 eye, Vec3 target, Vec3 up) {
  Vec3 f = (target - eye).normalized();
  Vec3 s = f.cross(up).normalized();
  Vec3 u = s.cross(f);
  Mat4 r = Mat4::identity();
  r.m[0] = s.x;
  r.m[4] = s.y;
  r.m[8] = s.z;
  r.m[1] = u.x;
  r.m[5] = u.y;
  r.m[9] = u.z;
  r.m[2] = -f.x;
  r.m[6] = -f.y;
  r.m[10] = -f.z;
  r.m[12] = -s.dot(eye);
  r.m[13] = -u.dot(eye);
  r.m[14] = f.dot(eye);
  return r;
}

#endif // MATH_H
