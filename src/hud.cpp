#include "hud.h"

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>

void HUD::build_static(int win_h) {
    icon_mesh_ = Mesh(gen_hud_coin_icon(30, (float)win_h - 30, 12));

    std::vector<float> slash;
    float sx = 80, sy = (float)win_h - 45;
    push_vert(slash, sx, sy, 0, 1,1,1);
    push_vert(slash, sx+3, sy, 0, 1,1,1);
    push_vert(slash, sx+10, sy+20, 0, 1,1,1);
    push_vert(slash, sx+3, sy, 0, 1,1,1);
    push_vert(slash, sx+10, sy+20, 0, 1,1,1);
    push_vert(slash, sx+7, sy+20, 0, 1,1,1);
    slash_mesh_ = Mesh(slash);

    auto d2 = gen_hud_digit(total_coins_ % 10, 95, (float)win_h - 50, 2.5f);
    if (!d2.empty()) {
        total_mesh_ = Mesh(d2);
    }
}

void HUD::update(int coins_collected, int win_h) {
    if (win_h != cached_win_h_) {
        build_static(win_h);
        cached_win_h_ = win_h;
        cached_collected_ = -1;
    }
    if (coins_collected != cached_collected_) {
        auto d1 = gen_hud_digit(coins_collected % 10, 50, (float)win_h - 50, 2.5f);
        if (!d1.empty()) {
            collected_mesh_ = Mesh(d1);
        } else {
            collected_mesh_ = Mesh();
        }
        cached_collected_ = coins_collected;
    }
}

void HUD::draw(const ShaderProgram& shader, int win_w, int win_h) const {
    glDisable(GL_DEPTH_TEST);
    Mat4 ortho = mat4_ortho(0, (float)win_w, 0, (float)win_h, -1, 1);
    shader.set_mvp(ortho);

    if (icon_mesh_.valid()) icon_mesh_.draw();
    if (collected_mesh_.valid()) collected_mesh_.draw();
    if (slash_mesh_.valid()) slash_mesh_.draw();
    if (total_mesh_.valid()) total_mesh_.draw();

    glEnable(GL_DEPTH_TEST);
}
