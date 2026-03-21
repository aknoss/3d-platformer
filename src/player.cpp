#include "player.h"
#include <cmath>

bool aabb_overlap(Vec3 pmin, Vec3 pmax, Vec3 bmin, Vec3 bmax) {
    return pmax.x > bmin.x && pmin.x < bmax.x &&
           pmax.y > bmin.y && pmin.y < bmax.y &&
           pmax.z > bmin.z && pmin.z < bmax.z;
}

Player::Player() : pos_({0, 1, 0}), vel_({0, 0, 0}), yaw_(0), on_ground_(false),
           mesh_(gen_box({-HALF_W, 0, -HALF_W}, {HALF_W, HEIGHT, HALF_W}, {0.9f, 0.15f, 0.15f})) {}

void Player::handle_input(float move, float turn, bool jump, float dt) {
    yaw_ += turn * TURN_SPEED * dt;
    Vec3 forward = {sinf(yaw_), 0, cosf(yaw_)};
    vel_.x = forward.x * move * MOVE_SPEED;
    vel_.z = forward.z * move * MOVE_SPEED;

    if (jump && on_ground_) {
        vel_.y = JUMP_VEL;
        on_ground_ = false;
    }
}

void Player::update_physics(const PlatformData* platforms, int count, float dt) {
    vel_.y += GRAVITY * dt;
    Vec3 new_pos = pos_ + vel_ * dt;

    Vec3 pmin = {new_pos.x - HALF_W, new_pos.y, new_pos.z - HALF_W};
    Vec3 pmax = {new_pos.x + HALF_W, new_pos.y + HEIGHT, new_pos.z + HALF_W};

    on_ground_ = false;

    for (int i = 0; i < count; i++) {
        const PlatformData& pl = platforms[i];
        if (!aabb_overlap(pmin, pmax, pl.min, pl.max)) continue;

        float overlaps[6] = {
            pmax.x - pl.min.x,
            pl.max.x - pmin.x,
            pmax.y - pl.min.y,
            pl.max.y - pmin.y,
            pmax.z - pl.min.z,
            pl.max.z - pmin.z,
        };

        int min_axis = 0;
        float min_overlap = overlaps[0];
        for (int j = 1; j < 6; j++) {
            if (overlaps[j] < min_overlap) {
                min_overlap = overlaps[j];
                min_axis = j;
            }
        }

        switch (min_axis) {
            case 0: new_pos.x = pl.min.x - HALF_W;  vel_.x = 0; break;
            case 1: new_pos.x = pl.max.x + HALF_W;  vel_.x = 0; break;
            case 2: new_pos.y = pl.min.y - HEIGHT;   vel_.y = 0; break;
            case 3: new_pos.y = pl.max.y;            vel_.y = 0; on_ground_ = true; break;
            case 4: new_pos.z = pl.min.z - HALF_W;   vel_.z = 0; break;
            case 5: new_pos.z = pl.max.z + HALF_W;   vel_.z = 0; break;
        }

        pmin = {new_pos.x - HALF_W, new_pos.y, new_pos.z - HALF_W};
        pmax = {new_pos.x + HALF_W, new_pos.y + HEIGHT, new_pos.z + HALF_W};
    }

    pos_ = new_pos;

    if (pos_.y < RESPAWN_Y) {
        pos_ = {0, 1, 0};
        vel_ = {0, 0, 0};
        on_ground_ = false;
    }
}

void Player::draw(const ShaderProgram& shader, const Mat4& vp) const {
    Mat4 model = mat4_translate(pos_) * mat4_rotate_y(yaw_);
    shader.set_mvp(vp * model);
    mesh_.draw();
}
