#ifndef PLAYER_H
#define PLAYER_H
#include "constants.h"
#include "math.h"
#include "renderer.h"
#include <string>
#include <vector>

struct PlatformData {
  Vec3 min, max;
  Vec3 color;
};

bool aabb_overlap(Vec3 pmin, Vec3 pmax, Vec3 bmin, Vec3 bmax);

struct BodyPart {
  std::string name;
  TexturedMesh mesh;
  Vec3 pivot; // joint position in model space (where to rotate around)
};

class Player {
public:
  Player();

  void handle_input(float move_x, float move_z, float cam_yaw, bool jump);
  void update_physics(const PlatformData *platforms, int count, float dt);
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
  float walk_timer_;

  std::vector<BodyPart> parts_;
  ShaderProgram tex_shader_;

  int find_part(const char *name) const;
};

#endif // PLAYER_H
