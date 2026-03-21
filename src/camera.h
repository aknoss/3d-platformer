#ifndef CAMERA_H
#define CAMERA_H
#include "math.h"
#include "constants.h"
#include <cmath>

class Camera {
public:
    Camera() : pos_({0, 4, 6}), yaw_(0) {}

    void follow(Vec3 target_pos, float target_yaw, float dt) {
        float yaw_diff = target_yaw - yaw_;
        while (yaw_diff > (float)M_PI)  yaw_diff -= 2.0f * (float)M_PI;
        while (yaw_diff < -(float)M_PI) yaw_diff += 2.0f * (float)M_PI;
        yaw_ += yaw_diff * CAM_LERP * dt;

        Vec3 cam_forward = {sinf(yaw_), 0, cosf(yaw_)};
        pos_ = target_pos - cam_forward * CAM_DIST + Vec3(0, CAM_HEIGHT, 0);
    }

    Mat4 view_matrix(Vec3 target_pos) const {
        Vec3 cam_target = target_pos + Vec3(0, 0.75f, 0);
        return mat4_look_at(pos_, cam_target, {0, 1, 0});
    }

private:
    Vec3 pos_;
    float yaw_;
};

#endif // CAMERA_H
