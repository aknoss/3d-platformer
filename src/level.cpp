#include "level.h"
#include <cmath>

Level::Level()
    : coins_collected_(0), total_coins_(0), star_pos_({0, 0, 0}), star_spin_(0),
      star_collected_(true) {}

Level::Level(const LevelData &data)
    : coins_collected_(0), total_coins_(0), star_spin_(0),
      star_collected_(false) {
  for (const auto &p : data.platforms) {
    platforms_.push_back(p);
    platform_meshes_.push_back(Mesh(gen_box(p.min, p.max, p.color)));
    platform_origins_.push_back(p.min);
  }

  for (const auto &c : data.coins) {
    coins_.push_back(Coin(c));
  }
  total_coins_ = (int)data.coins.size();

  coin_mesh_ = Mesh(gen_octagon(0.4f, 0.1f, {1.0f, 0.85f, 0.0f}));

  star_pos_ = data.star_pos;
  star_mesh_ = Mesh(gen_star(0.6f, 0.15f, {1.0f, 0.9f, 0.1f}));
}

void Level::update(float dt, Vec3 player_pos) {
  for (auto &c : coins_) {
    c.update(dt);
    if (c.try_collect(player_pos)) {
      coins_collected_++;
    }
  }

  if (!star_collected_) {
    star_spin_ += 2.0f * dt;
    Vec3 diff = player_pos - star_pos_;
    diff.y += 0.75f; // offset to player center
    if (diff.length() < 2.0f) {
      star_collected_ = true;
    }
  }
}

void Level::draw(const ShaderProgram &shader, const Mat4 &vp) const {
  for (size_t i = 0; i < platforms_.size(); i++) {
    Vec3 offset = platforms_[i].min - platform_origins_[i];
    Mat4 mvp = vp * mat4_translate(offset);
    shader.set_mvp(mvp);
    platform_meshes_[i].draw();
  }
  for (const auto &c : coins_) {
    c.draw(shader, vp, coin_mesh_);
  }

  if (!star_collected_) {
    float bob = sinf(star_spin_ * 1.5f) * 0.3f;
    Mat4 model = mat4_translate(star_pos_ + Vec3(0, 0.8f + bob, 0)) *
                 mat4_rotate_y(star_spin_);
    shader.set_mvp(vp * model);
    star_mesh_.draw();
  }
}
