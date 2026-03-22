#include "player.h"
#include "glb_loader.h"
#include <cmath>

#include "character_glb.h"
#include "character_tex.h"

bool aabb_overlap(Vec3 pmin, Vec3 pmax, Vec3 bmin, Vec3 bmax) {
  return pmax.x > bmin.x && pmin.x < bmax.x && pmax.y > bmin.y &&
         pmin.y < bmax.y && pmax.z > bmin.z && pmin.z < bmax.z;
}

Player::Player()
    : pos_({0, 1, 0}), vel_({0, 0, 0}), yaw_(0), on_ground_(false),
      moving_(false), walk_timer_(0) {
  GlbPartsModel model = load_glb_parts_from_memory(assets_character_glb,
                                                   assets_character_glb_len);

  for (auto &part : model.parts) {
    std::vector<float> buf;
    buf.reserve(part.vertices.size() * 8);
    for (const auto &v : part.vertices) {
      buf.push_back(v.x);
      buf.push_back(v.y);
      buf.push_back(v.z);
      buf.push_back(v.nx);
      buf.push_back(v.ny);
      buf.push_back(v.nz);
      buf.push_back(v.u);
      buf.push_back(v.v);
    }

    // Each TexturedMesh owns its texture, so load a separate copy per part
    GLuint tex = load_texture_from_memory(assets_texture_character_png,
                                          assets_texture_character_png_len);
    parts_.push_back({part.name,
                      TexturedMesh(buf, tex),
                      {part.pivot_x, part.pivot_y, part.pivot_z}});
  }

  tex_shader_ = ShaderProgram::create_textured();
}

int Player::find_part(const char *name) const {
  for (int i = 0; i < (int)parts_.size(); i++) {
    if (parts_[i].name == name)
      return i;
  }
  return -1;
}

void Player::handle_input(float move_x, float move_z, float cam_yaw,
                          bool jump) {
  Vec3 cam_fwd = {sinf(cam_yaw), 0, cosf(cam_yaw)};
  Vec3 cam_right = {cam_fwd.z, 0, -cam_fwd.x};

  Vec3 wish_dir = cam_fwd * move_z + cam_right * move_x;
  float len = sqrtf(wish_dir.x * wish_dir.x + wish_dir.z * wish_dir.z);

  if (on_ground_) {
    if (len > 0.001f) {
      wish_dir.x /= len;
      wish_dir.z /= len;
      vel_.x = wish_dir.x * MOVE_SPEED;
      vel_.z = wish_dir.z * MOVE_SPEED;
      yaw_ = atan2f(-wish_dir.x, wish_dir.z);
      moving_ = true;
    } else {
      vel_.x = 0;
      vel_.z = 0;
      moving_ = false;
    }
  }

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

  // Advance walk animation timer
  if (on_ground_ && moving_) {
    walk_timer_ += dt * 10.0f; // walk cycle speed
  } else if (on_ground_) {
    // Smoothly return to idle
    walk_timer_ = 0;
  }

  if (pos_.y < RESPAWN_Y) {
    pos_ = {0, 1, 0};
    vel_ = {0, 0, 0};
    on_ground_ = false;
    walk_timer_ = 0;
  }
}

void Player::draw(const ShaderProgram & /*shader*/, const Mat4 &vp) const {
  // Base transform: translate to world pos, rotate to face direction, scale
  Mat4 base =
      mat4_translate(pos_) * mat4_rotate_y(yaw_) * mat4_scale(MODEL_SCALE);

  // Animation angles
  float leg_swing = 0;
  float arm_swing = 0;

  if (!on_ground_) {
    // Jump pose: legs tucked forward, arms up
    leg_swing = -0.4f;
    arm_swing = -0.6f;
  } else if (moving_) {
    // Walk cycle: sinusoidal swing
    leg_swing = sinf(walk_timer_) * 0.6f;
    arm_swing = sinf(walk_timer_) * 0.5f;
  }

  tex_shader_.use();

  for (int i = 0; i < (int)parts_.size(); i++) {
    const BodyPart &part = parts_[i];
    Mat4 part_transform = Mat4::identity();

    if (part.name == "leg-left") {
      // Pivot at hip, swing forward/backward
      part_transform = mat4_translate(part.pivot) * mat4_rotate_x(leg_swing) *
                       mat4_translate(-part.pivot);
    } else if (part.name == "leg-right") {
      // Opposite phase from left leg
      part_transform = mat4_translate(part.pivot) * mat4_rotate_x(-leg_swing) *
                       mat4_translate(-part.pivot);
    } else if (part.name == "arm-left") {
      // Arms swing opposite to legs
      part_transform = mat4_translate(part.pivot) * mat4_rotate_x(-arm_swing) *
                       mat4_translate(-part.pivot);
    } else if (part.name == "arm-right") {
      part_transform = mat4_translate(part.pivot) * mat4_rotate_x(arm_swing) *
                       mat4_translate(-part.pivot);
    }
    // head and torso: no animation rotation (identity)

    Mat4 mvp = vp * base * part_transform;
    tex_shader_.set_mvp(mvp);
    part.mesh.draw();
  }
}
