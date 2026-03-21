#ifndef HUD_H
#define HUD_H
#include "math.h"
#include "geometry.h"
#include "renderer.h"

class HUD {
public:
    HUD(int total_coins) : cached_collected_(-1), cached_win_h_(-1), total_coins_(total_coins) {}

    void update(int coins_collected, int win_h);
    void draw(const ShaderProgram& shader, int win_w, int win_h) const;

private:
    void build_static(int win_h);

    Mesh icon_mesh_;
    Mesh collected_mesh_;
    Mesh slash_mesh_;
    Mesh total_mesh_;
    int cached_collected_;
    int cached_win_h_;
    int total_coins_;
};

#endif // HUD_H
