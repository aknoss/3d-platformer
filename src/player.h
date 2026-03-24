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
};

// Rest-pose TRS for a node in the hierarchy
struct NodeRest {
  Vec3 translation;
  Quat rotation;
  Vec3 scale;
  NodeRest() : scale(1, 1, 1) {}
};

class Player {
public:
  Player();

  void handle_input(float move_x, float move_z, float cam_yaw, bool jump);
  void update_physics(PlatformData *platforms, int count, float dt);
  void draw(const ShaderProgram &shader, const Mat4 &vp) const;

  Vec3 pos() const { return pos_; }
  float yaw() const { return yaw_; }

  ShaderProgram &textured_shader() { return tex_shader_; }

private:
  static constexpr float HALF_W = 0.5f;
  static constexpr float HEIGHT = 1.5f;
  static constexpr float MODEL_SCALE = 1.5f / 2.7f;

  Vec3 pos_, vel_;
  float yaw_;
  bool on_ground_;
  bool moving_;

  // Animation
  AnimState anim_state_;
  float anim_time_;
  AnimSet animations_;
  const AnimClip *current_clip_;

  // Body parts (mesh nodes only)
  std::vector<BodyPart> parts_;
  ShaderProgram tex_shader_;

  // Rest-pose TRS lookup by node name (all nodes including non-mesh)
  // Indexed parallel to known names: root, leg-left, leg-right, torso,
  // arm-left, arm-right, head
  NodeRest rest_root_, rest_torso_;
  // Per-part rest poses (indexed parallel to parts_)
  std::vector<NodeRest> part_rest_;
  // Per-part parent chain type
  std::vector<int> part_parent_type_; // 0=root child, 1=torso child

  Mat4 compute_node_trs(const std::string &name, const AnimClip *clip,
                        float t) const;
  void set_animation(AnimState state);
};

#endif // PLAYER_H
