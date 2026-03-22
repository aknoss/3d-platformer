#include "player.h"
#include "obj_loader.h"
#include <cmath>

#include "character_obj.h"
#include "character_tex.h"

bool aabb_overlap(Vec3 pmin, Vec3 pmax, Vec3 bmin, Vec3 bmax) {
  return pmax.x > bmin.x && pmin.x < bmax.x && pmax.y > bmin.y &&
         pmin.y < bmax.y && pmax.z > bmin.z && pmin.z < bmax.z;
}

Player::Player()
    : pos_({0, 1, 0}), vel_({0, 0, 0}), yaw_(0), on_ground_(false) {
  ObjModel model =
      load_obj_from_memory(assets_character_obj, assets_character_obj_len);

  // Pack into pos(3) + normal(3) + uv(2) buffer
  std::vector<float> buf;
  buf.reserve(model.vertices.size() * 8);
  for (const auto &v : model.vertices) {
    buf.push_back(v.x);
    buf.push_back(v.y);
    buf.push_back(v.z);
    buf.push_back(v.nx);
    buf.push_back(v.ny);
    buf.push_back(v.nz);
    buf.push_back(v.u);
    buf.push_back(v.v);
  }

  GLuint tex = load_texture_from_memory(assets_texture_character_png,
                                        assets_texture_character_png_len);
  mesh_ = TexturedMesh(buf, tex);
  tex_shader_ = ShaderProgram::create_textured();
}

void Player::handle_input(float move_x, float move_z, float cam_yaw,
                          bool jump) {
  // Build camera-relative direction vectors
  Vec3 cam_fwd = {sinf(cam_yaw), 0, cosf(cam_yaw)};
  Vec3 cam_right = {cam_fwd.z, 0, -cam_fwd.x};

  // Compute world-space movement from input
  Vec3 wish_dir = cam_fwd * move_z + cam_right * move_x;
  float len = sqrtf(wish_dir.x * wish_dir.x + wish_dir.z * wish_dir.z);

  if (on_ground_) {
    if (len > 0.001f) {
      wish_dir.x /= len;
      wish_dir.z /= len;
      vel_.x = wish_dir.x * MOVE_SPEED;
      vel_.z = wish_dir.z * MOVE_SPEED;
      yaw_ = atan2f(-wish_dir.x, wish_dir.z);
    } else {
      vel_.x = 0;
      vel_.z = 0;
    }
  }
  // No user movement on air

  if (jump && on_ground_) {
    vel_.y = JUMP_VEL;
    on_ground_ = false;
  }
}

void Player::update_physics(const PlatformData *platforms, int count,
                            float dt) {
  vel_.y += GRAVITY * dt;
  Vec3 new_pos = pos_ + vel_ * dt;

  Vec3 pmin = {new_pos.x - HALF_W, new_pos.y, new_pos.z - HALF_W};
  Vec3 pmax = {new_pos.x + HALF_W, new_pos.y + HEIGHT, new_pos.z + HALF_W};

  on_ground_ = false;

  for (int i = 0; i < count; i++) {
    const PlatformData &pl = platforms[i];
    if (!aabb_overlap(pmin, pmax, pl.min, pl.max))
      continue;

    float overlaps[6] = {
        pmax.x - pl.min.x, pl.max.x - pmin.x, pmax.y - pl.min.y,
        pl.max.y - pmin.y, pmax.z - pl.min.z, pl.max.z - pmin.z,
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
    case 0:
      new_pos.x = pl.min.x - HALF_W;
      vel_.x = 0;
      break;
    case 1:
      new_pos.x = pl.max.x + HALF_W;
      vel_.x = 0;
      break;
    case 2:
      new_pos.y = pl.min.y - HEIGHT;
      vel_.y = 0;
      break;
    case 3:
      new_pos.y = pl.max.y;
      vel_.y = 0;
      on_ground_ = true;
      break;
    case 4:
      new_pos.z = pl.min.z - HALF_W;
      vel_.z = 0;
      break;
    case 5:
      new_pos.z = pl.max.z + HALF_W;
      vel_.z = 0;
      break;
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

void Player::draw(const ShaderProgram & /*shader*/, const Mat4 &vp) const {
  Mat4 model =
      mat4_translate(pos_) * mat4_rotate_y(yaw_) * mat4_scale(MODEL_SCALE);
  tex_shader_.use();
  tex_shader_.set_mvp(vp * model);
  mesh_.draw();
}
