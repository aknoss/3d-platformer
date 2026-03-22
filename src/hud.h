#ifndef HUD_H
#define HUD_H
#include "geometry.h"
#include "math.h"
#include "renderer.h"

class HUD {
public:
  HUD() : cached_collected_(-1), cached_win_h_(-1) {}

  void update(int coins_collected, int win_h);
  void draw(const ShaderProgram &shader, int win_w, int win_h) const;

private:
  void build_static(int win_h);

  Mesh icon_mesh_;
  Mesh collected_mesh_;
  int cached_collected_;
  int cached_win_h_;
};

#endif // HUD_H
