#ifndef CAMERA_H
#define CAMERA_H
#include "constants.h"
#include "math.h"
#include <cmath>

class Camera {
public:
  Camera() : pos_({0, 4, 6}), yaw_(0) {}

  void rotate(float mouse_dx) { yaw_ -= mouse_dx * MOUSE_SENS; }

  void follow(Vec3 target_pos) {
    Vec3 cam_forward = {sinf(yaw_), 0, cosf(yaw_)};
    pos_ = target_pos - cam_forward * CAM_DIST + Vec3(0, CAM_HEIGHT, 0);
  }

  Mat4 view_matrix(Vec3 target_pos) const {
    Vec3 cam_target = target_pos + Vec3(0, 0.75f, 0);
    return mat4_look_at(pos_, cam_target, {0, 1, 0});
  }

  float yaw() const { return yaw_; }

private:
  Vec3 pos_;
  float yaw_;
};

#endif // CAMERA_H
