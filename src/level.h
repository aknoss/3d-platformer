#ifndef LEVEL_H
#define LEVEL_H
#include "constants.h"
#include "geometry.h"
#include "math.h"
#include "player.h"
#include "renderer.h"
#include <vector>

class Coin {
public:
  Coin() : pos_({0, 0, 0}), spin_angle_(0), collected_(false) {}
  Coin(Vec3 pos) : pos_(pos), spin_angle_(0), collected_(false) {}

  void update(float dt) { spin_angle_ += 3.0f * dt; }

  bool try_collect(Vec3 player_pos) {
    if (collected_)
      return false;
    Vec3 diff = player_pos - pos_;
    diff.y += 0.75f;
    if (diff.length() < COIN_COLLECT_DIST) {
      collected_ = true;
      return true;
    }
    return false;
  }

  void draw(const ShaderProgram &shader, const Mat4 &vp,
            const Mesh &shared_mesh) const {
    if (collected_)
      return;
    Mat4 model =
        mat4_translate(pos_ + Vec3(0, 0.4f, 0)) * mat4_rotate_y(spin_angle_);
    shader.set_mvp(vp * model);
    shared_mesh.draw();
  }

  bool collected() const { return collected_; }

private:
  Vec3 pos_;
  float spin_angle_;
  bool collected_;
};

class Level {
public:
  Level();

  void update(float dt, Vec3 player_pos);
  void draw(const ShaderProgram &shader, const Mat4 &vp) const;

  PlatformData *platform_data() { return platforms_.data(); }
  int platform_count() const { return (int)platforms_.size(); }
  int coins_collected() const { return coins_collected_; }
  int total_coins() const { return total_coins_; }

private:
  std::vector<PlatformData> platforms_;
  std::vector<Mesh> platform_meshes_;
  std::vector<Vec3> platform_origins_; // original min for tracking push offset
  std::vector<Coin> coins_;
  Mesh coin_mesh_;
  int coins_collected_;
  int total_coins_;
};

#endif // LEVEL_H
