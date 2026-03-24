#include "level.h"

Level::Level() {

  PlatformData platform_defs[] = {
      {{-15, -0.5f, -15}, {15, 0, 15}, {0.2f, 0.7f, 0.2f}, false},
      {{3, 0, 3}, {7, 1.5f, 7}, {0.6f, 0.4f, 0.2f}, true},
      {{-8, 0, -3}, {-4, 2.5f, 1}, {0.5f, 0.5f, 0.5f}, true},
      {{-3, 0, 8}, {1, 1.0f, 12}, {0.6f, 0.4f, 0.2f}, true},
      {{8, 0, -8}, {12, 3.5f, -4}, {0.5f, 0.5f, 0.5f}, true},
      {{-10, 0, -10}, {-6, 4.0f, -7}, {0.6f, 0.4f, 0.2f}, true},
      {{5, 0, -2}, {9, 2.0f, 2}, {0.5f, 0.5f, 0.5f}, true},
  };

  int num_plats = sizeof(platform_defs) / sizeof(platform_defs[0]);
  for (int i = 0; i < num_plats; i++) {
    platforms_.push_back(platform_defs[i]);
    platform_meshes_.push_back(Mesh(gen_box(
        platform_defs[i].min, platform_defs[i].max, platform_defs[i].color)));
    platform_origins_.push_back(platform_defs[i].min);
  }

  Vec3 coin_positions[] = {
      {5, 2.5f, 5},      {-6, 3.5f, -1}, {-1, 2.0f, 10}, {10, 4.5f, -6},
      {-8, 5.0f, -8.5f}, {7, 3.0f, 0},   {0, 1.0f, 0},   {-12, 1.0f, 12},
  };
  int num_coins = sizeof(coin_positions) / sizeof(coin_positions[0]);
  for (int i = 0; i < num_coins; i++) {
    coins_.push_back(Coin(coin_positions[i]));
  }
  total_coins_ = num_coins;
  coins_collected_ = 0;

  coin_mesh_ = Mesh(gen_octagon(0.4f, 0.1f, {1.0f, 0.85f, 0.0f}));
}

void Level::update(float dt, Vec3 player_pos) {
  for (auto &c : coins_) {
    c.update(dt);
    if (c.try_collect(player_pos)) {
      coins_collected_++;
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
}
