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
    : pos_({0, 1, 0}), vel_({0, 0, 0}), spawn_pos_({0, 1, 0}), yaw_(0),
      on_ground_(false), moving_(false), anim_state_(AnimState::Idle),
      anim_time_(0), current_clip_(nullptr) {
  GlbLoadResult loaded =
      load_glb_with_animations(assets_character_glb, assets_character_glb_len);

  animations_ = std::move(loaded.animations);

  // Build skeleton from GLB node rest poses
  for (const auto &rp : loaded.parts.node_rest_poses) {
    SkeletonNode node;
    node.name = rp.name;
    node.translation = rp.translation;
    node.rotation = rp.rotation;
    node.scale = rp.scale;
    node.parent = -1;
    skeleton_.push_back(node);
  }
  // Resolve parent indices
  for (size_t i = 0; i < skeleton_.size(); i++) {
    const std::string &pname = loaded.parts.node_rest_poses[i].parent_name;
    if (!pname.empty()) {
      for (size_t j = 0; j < skeleton_.size(); j++) {
        if (skeleton_[j].name == pname) {
          skeleton_[i].parent = (int)j;
          break;
        }
      }
    }
  }

  // Build mesh parts and link to skeleton nodes
  for (auto &glb_part : loaded.parts.parts) {
    std::vector<float> buf;
    buf.reserve(glb_part.vertices.size() * 8);
    for (const auto &v : glb_part.vertices) {
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

    int node_idx = -1;
    for (size_t i = 0; i < skeleton_.size(); i++) {
      if (skeleton_[i].name == glb_part.name) {
        node_idx = (int)i;
        break;
      }
    }
    parts_.push_back({glb_part.name, TexturedMesh(buf, tex), node_idx});
  }

  tex_shader_ = ShaderProgram::create_textured();
  set_animation(AnimState::Idle);
}

void Player::set_animation(AnimState state) {
  if (state == anim_state_ && current_clip_)
    return;

  anim_state_ = state;
  anim_time_ = 0;

  const char *clip_name = "static";
  switch (state) {
  case AnimState::Idle:
    clip_name = "idle";
    break;
  case AnimState::Walk:
    clip_name = "walk";
    break;
  case AnimState::Jump:
    clip_name = "static";
    break;
  case AnimState::Fall:
    clip_name = "static";
    break;
  }

  current_clip_ = animations_.find_clip(clip_name);
}

Mat4 Player::compute_node_world(int node_index) const {
  if (node_index < 0)
    return Mat4::identity();

  const SkeletonNode &node = skeleton_[node_index];
  Vec3 trans = node.translation;
  Quat rot = node.rotation;
  Vec3 scl = node.scale;

  if (current_clip_) {
    const NodeChannels *ch = current_clip_->find_node(node.name);
    if (ch) {
      NodePose pose = sample_node(*ch, anim_time_);
      if (!ch->trans_times.empty())
        trans = pose.translation;
      if (!ch->rot_times.empty())
        rot = pose.rotation;
      if (!ch->scale_times.empty())
        scl = pose.scale;
    }
  }

  Mat4 local = mat4_trs(trans, rot, scl);
  return compute_node_world(node.parent) * local;
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

void Player::update_physics(PlatformData *platforms, int count, float dt) {
  vel_.y += GRAVITY * dt;
  Vec3 new_pos = pos_ + vel_ * dt;

  Vec3 pmin = {new_pos.x - HALF_W, new_pos.y, new_pos.z - HALF_W};
  Vec3 pmax = {new_pos.x + HALF_W, new_pos.y + HEIGHT, new_pos.z + HALF_W};

  on_ground_ = false;

  for (int i = 0; i < count; i++) {
    PlatformData &pl = platforms[i];
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

    // Push pushable platforms on X/Z axes instead of blocking
    if (pl.pushable && (min_axis <= 1 || min_axis >= 4)) {
      float push = PUSH_SPEED * dt;
      switch (min_axis) {
      case 0: // player coming from -X side → push platform in +X
        pl.min.x += push;
        pl.max.x += push;
        new_pos.x = pl.min.x - HALF_W;
        break;
      case 1: // player coming from +X side → push platform in -X
        pl.min.x -= push;
        pl.max.x -= push;
        new_pos.x = pl.max.x + HALF_W;
        break;
      case 4: // player coming from -Z side → push platform in +Z
        pl.min.z += push;
        pl.max.z += push;
        new_pos.z = pl.min.z - HALF_W;
        break;
      case 5: // player coming from +Z side → push platform in -Z
        pl.min.z -= push;
        pl.max.z -= push;
        new_pos.z = pl.max.z + HALF_W;
        break;
      }
    } else {
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
    }

    pmin = {new_pos.x - HALF_W, new_pos.y, new_pos.z - HALF_W};
    pmax = {new_pos.x + HALF_W, new_pos.y + HEIGHT, new_pos.z + HALF_W};
  }

  pos_ = new_pos;

  // Animation state machine
  if (!on_ground_ && vel_.y > 0)
    set_animation(AnimState::Jump);
  else if (!on_ground_ && vel_.y <= 0)
    set_animation(AnimState::Fall);
  else if (on_ground_ && moving_)
    set_animation(AnimState::Walk);
  else if (on_ground_)
    set_animation(AnimState::Idle);

  // Advance animation time
  if (current_clip_ && current_clip_->duration > 0) {
    anim_time_ += dt;
    // Loop for walk/idle, clamp for jump
    if (anim_state_ == AnimState::Jump) {
      if (anim_time_ > current_clip_->duration)
        anim_time_ = current_clip_->duration;
    } else {
      anim_time_ = fmodf(anim_time_, current_clip_->duration);
    }
  }

  if (pos_.y < RESPAWN_Y) {
    respawn();
  }
}

void Player::draw(const ShaderProgram & /*shader*/, const Mat4 &vp) const {
  Mat4 base =
      mat4_translate(pos_) * mat4_rotate_y(yaw_) * mat4_scale(MODEL_SCALE);

  tex_shader_.use();

  for (const auto &part : parts_) {
    Mat4 mvp = vp * base * compute_node_world(part.node_index);
    tex_shader_.set_mvp(mvp);
    part.mesh.draw();
  }
}

void Player::respawn() {
  pos_ = spawn_pos_;
  vel_ = {0, 0, 0};
  on_ground_ = false;
  set_animation(AnimState::Fall);
}
