#ifndef PLAYER_H
#define PLAYER_H
#include "animation.h"
#include "constants.h"
#include "math.h"
#include "renderer.h"
#include <string>
#include <vector>

struct PlatformData {
  Vec3 min, max;
  Vec3 color;
  bool pushable = false;
};

bool aabb_overlap(Vec3 pmin, Vec3 pmax, Vec3 bmin, Vec3 bmax);

enum class AnimState { Idle, Walk, Jump, Fall };

struct BodyPart {
  std::string name;
  TexturedMesh mesh;
  int node_index; // index into skeleton_
};

struct SkeletonNode {
  std::string name;
  Vec3 translation;
  Quat rotation;
  Vec3 scale;
  int parent; // -1 = root
};

class Player {
public:
  Player();

  void handle_input(float move_x, float move_z, float cam_yaw, bool jump);
  void update_physics(PlatformData *platforms, int count, float dt);
  void draw(const ShaderProgram &shader, const Mat4 &vp) const;

  Vec3 pos() const { return pos_; }
  float yaw() const { return yaw_; }
  void set_spawn(Vec3 s) { spawn_pos_ = s; }
  void respawn();

  ShaderProgram &textured_shader() { return tex_shader_; }

private:
  static constexpr float HALF_W = 0.5f;
  static constexpr float HEIGHT = 1.5f;
  static constexpr float MODEL_SCALE = 1.5f / 2.7f;

  Vec3 pos_, vel_;
  Vec3 spawn_pos_;
  float yaw_;
  bool on_ground_;
  bool moving_;

  // Animation
  AnimState anim_state_;
  float anim_time_;
  AnimSet animations_;
  const AnimClip *current_clip_;

  // Skeleton and mesh parts
  std::vector<SkeletonNode> skeleton_;
  std::vector<BodyPart> parts_;
  ShaderProgram tex_shader_;

  Mat4 compute_node_world(int node_index) const;
  void set_animation(AnimState state);
};

#endif // PLAYER_H
