#ifndef PLAYER_H
#define PLAYER_H
#include "math.h"
#include "constants.h"
#include "geometry.h"
#include "renderer.h"

struct PlatformData {
    Vec3 min, max;
    Vec3 color;
};

bool aabb_overlap(Vec3 pmin, Vec3 pmax, Vec3 bmin, Vec3 bmax);

class Player {
public:
    Player();

    void handle_input(float move, float turn, bool jump, float dt);
    void update_physics(const PlatformData* platforms, int count, float dt);
    void draw(const ShaderProgram& shader, const Mat4& vp) const;

    Vec3 pos() const { return pos_; }
    float yaw() const { return yaw_; }

private:
    static constexpr float HALF_W = 0.5f;
    static constexpr float HEIGHT = 1.5f;

    Vec3 pos_, vel_;
    float yaw_;
    bool on_ground_;
    Mesh mesh_;
};

#endif // PLAYER_H
