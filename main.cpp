// SM64-Style 3D Platformer — SDL3 + OpenGL 3.3 Core
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <cmath>
#include <cstring>
#include <vector>
#include <cstdio>

// ============================================================
// 3D Math
// ============================================================
struct Vec3 {
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    Vec3 operator+(const Vec3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vec3 operator*(float s) const { return {x*s, y*s, z*s}; }
    Vec3 operator-() const { return {-x, -y, -z}; }
    float dot(const Vec3& o) const { return x*o.x + y*o.y + z*o.z; }
    Vec3 cross(const Vec3& o) const {
        return {y*o.z - z*o.y, z*o.x - x*o.z, x*o.y - y*o.x};
    }
    float length() const { return sqrtf(x*x + y*y + z*z); }
    Vec3 normalized() const {
        float l = length();
        if (l < 1e-8f) return {0,0,0};
        return {x/l, y/l, z/l};
    }
};

struct Mat4 {
    float m[16]; // column-major

    static Mat4 identity() {
        Mat4 r;
        memset(r.m, 0, sizeof(r.m));
        r.m[0]=1; r.m[5]=1; r.m[10]=1; r.m[15]=1;
        return r;
    }

    Mat4 operator*(const Mat4& b) const {
        Mat4 r;
        memset(r.m, 0, sizeof(r.m));
        for (int c = 0; c < 4; c++)
            for (int row = 0; row < 4; row++)
                for (int k = 0; k < 4; k++)
                    r.m[c*4+row] += m[k*4+row] * b.m[c*4+k];
        return r;
    }
};

static Mat4 mat4_translate(float tx, float ty, float tz) {
    Mat4 r = Mat4::identity();
    r.m[12]=tx; r.m[13]=ty; r.m[14]=tz;
    return r;
}
static Mat4 mat4_translate(Vec3 v) { return mat4_translate(v.x, v.y, v.z); }

static Mat4 mat4_rotate_y(float angle) {
    Mat4 r = Mat4::identity();
    float c = cosf(angle), s = sinf(angle);
    r.m[0]=c;  r.m[2]=s;
    r.m[8]=-s; r.m[10]=c;
    return r;
}

static Mat4 mat4_perspective(float fov_deg, float aspect, float near, float far) {
    Mat4 r;
    memset(r.m, 0, sizeof(r.m));
    float f = 1.0f / tanf(fov_deg * (float)M_PI / 360.0f);
    r.m[0] = f / aspect;
    r.m[5] = f;
    r.m[10] = (far + near) / (near - far);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * far * near) / (near - far);
    return r;
}

static Mat4 mat4_ortho(float l, float r_val, float b, float t, float n, float f) {
    Mat4 res;
    memset(res.m, 0, sizeof(res.m));
    res.m[0]  = 2.0f / (r_val - l);
    res.m[5]  = 2.0f / (t - b);
    res.m[10] = -2.0f / (f - n);
    res.m[12] = -(r_val + l) / (r_val - l);
    res.m[13] = -(t + b) / (t - b);
    res.m[14] = -(f + n) / (f - n);
    res.m[15] = 1.0f;
    return res;
}

static Mat4 mat4_look_at(Vec3 eye, Vec3 target, Vec3 up) {
    Vec3 f = (target - eye).normalized();
    Vec3 s = f.cross(up).normalized();
    Vec3 u = s.cross(f);
    Mat4 r = Mat4::identity();
    r.m[0]=s.x;  r.m[4]=s.y;  r.m[8]=s.z;
    r.m[1]=u.x;  r.m[5]=u.y;  r.m[9]=u.z;
    r.m[2]=-f.x; r.m[6]=-f.y; r.m[10]=-f.z;
    r.m[12] = -s.dot(eye);
    r.m[13] = -u.dot(eye);
    r.m[14] = f.dot(eye);
    return r;
}

// ============================================================
// Constants
// ============================================================
static const int SCREEN_W = 800;
static const int SCREEN_H = 600;
static const float MOVE_SPEED = 5.0f;
static const float TURN_SPEED = 3.0f;
static const float GRAVITY = -15.0f;
static const float JUMP_VEL = 8.0f;
static const float CAM_DIST = 6.0f;
static const float CAM_HEIGHT = 3.0f;
static const float CAM_LERP = 5.0f;
static const float FOV = 60.0f;
static const float NEAR_PLANE = 0.1f;
static const float FAR_PLANE = 100.0f;
static const float COIN_COLLECT_DIST = 1.2f;
static const float RESPAWN_Y = -10.0f;

// ============================================================
// Shaders
// ============================================================
static const char* VERT_SRC = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
uniform mat4 uMVP;
out vec3 vColor;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vColor = aColor;
}
)";

static const char* FRAG_SRC = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(vColor, 1.0);
}
)";

// ============================================================
// Geometry Generation
// ============================================================

static void push_vert(std::vector<float>& buf, float x, float y, float z,
                       float r, float g, float b) {
    buf.push_back(x); buf.push_back(y); buf.push_back(z);
    buf.push_back(r); buf.push_back(g); buf.push_back(b);
}

static std::vector<float> gen_box(Vec3 mn, Vec3 mx, Vec3 col) {
    std::vector<float> buf;
    buf.reserve(36 * 6);

    float top_f = 1.1f, side_f = 0.8f, bot_f = 0.5f;
    Vec3 ct = {col.x*top_f, col.y*top_f, col.z*top_f};
    Vec3 cs = {col.x*side_f, col.y*side_f, col.z*side_f};
    Vec3 cb = {col.x*bot_f, col.y*bot_f, col.z*bot_f};
    auto clamp01 = [](float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); };
    ct = {clamp01(ct.x), clamp01(ct.y), clamp01(ct.z)};
    cs = {clamp01(cs.x), clamp01(cs.y), clamp01(cs.z)};
    cb = {clamp01(cb.x), clamp01(cb.y), clamp01(cb.z)};

    float x0=mn.x, y0=mn.y, z0=mn.z;
    float x1=mx.x, y1=mx.y, z1=mx.z;

    // Top face (+Y)
    push_vert(buf, x0,y1,z0, ct.x,ct.y,ct.z);
    push_vert(buf, x1,y1,z0, ct.x,ct.y,ct.z);
    push_vert(buf, x1,y1,z1, ct.x,ct.y,ct.z);
    push_vert(buf, x0,y1,z0, ct.x,ct.y,ct.z);
    push_vert(buf, x1,y1,z1, ct.x,ct.y,ct.z);
    push_vert(buf, x0,y1,z1, ct.x,ct.y,ct.z);

    // Bottom face (-Y)
    push_vert(buf, x0,y0,z1, cb.x,cb.y,cb.z);
    push_vert(buf, x1,y0,z1, cb.x,cb.y,cb.z);
    push_vert(buf, x1,y0,z0, cb.x,cb.y,cb.z);
    push_vert(buf, x0,y0,z1, cb.x,cb.y,cb.z);
    push_vert(buf, x1,y0,z0, cb.x,cb.y,cb.z);
    push_vert(buf, x0,y0,z0, cb.x,cb.y,cb.z);

    // Front face (+Z)
    push_vert(buf, x0,y0,z1, cs.x,cs.y,cs.z);
    push_vert(buf, x1,y0,z1, cs.x,cs.y,cs.z);
    push_vert(buf, x1,y1,z1, cs.x,cs.y,cs.z);
    push_vert(buf, x0,y0,z1, cs.x,cs.y,cs.z);
    push_vert(buf, x1,y1,z1, cs.x,cs.y,cs.z);
    push_vert(buf, x0,y1,z1, cs.x,cs.y,cs.z);

    // Back face (-Z)
    push_vert(buf, x1,y0,z0, cs.x,cs.y,cs.z);
    push_vert(buf, x0,y0,z0, cs.x,cs.y,cs.z);
    push_vert(buf, x0,y1,z0, cs.x,cs.y,cs.z);
    push_vert(buf, x1,y0,z0, cs.x,cs.y,cs.z);
    push_vert(buf, x0,y1,z0, cs.x,cs.y,cs.z);
    push_vert(buf, x1,y1,z0, cs.x,cs.y,cs.z);

    // Right face (+X)
    Vec3 cs2 = {clamp01(col.x*0.7f), clamp01(col.y*0.7f), clamp01(col.z*0.7f)};
    push_vert(buf, x1,y0,z1, cs2.x,cs2.y,cs2.z);
    push_vert(buf, x1,y0,z0, cs2.x,cs2.y,cs2.z);
    push_vert(buf, x1,y1,z0, cs2.x,cs2.y,cs2.z);
    push_vert(buf, x1,y0,z1, cs2.x,cs2.y,cs2.z);
    push_vert(buf, x1,y1,z0, cs2.x,cs2.y,cs2.z);
    push_vert(buf, x1,y1,z1, cs2.x,cs2.y,cs2.z);

    // Left face (-X)
    push_vert(buf, x0,y0,z0, cs2.x,cs2.y,cs2.z);
    push_vert(buf, x0,y0,z1, cs2.x,cs2.y,cs2.z);
    push_vert(buf, x0,y1,z1, cs2.x,cs2.y,cs2.z);
    push_vert(buf, x0,y0,z0, cs2.x,cs2.y,cs2.z);
    push_vert(buf, x0,y1,z1, cs2.x,cs2.y,cs2.z);
    push_vert(buf, x0,y1,z0, cs2.x,cs2.y,cs2.z);

    return buf;
}

static std::vector<float> gen_octagon(float radius, float thickness, Vec3 col) {
    std::vector<float> buf;
    const int SIDES = 8;
    Vec3 bright = {col.x*1.1f < 1 ? col.x*1.1f : 1.0f,
                   col.y*1.1f < 1 ? col.y*1.1f : 1.0f,
                   col.z*1.1f < 1 ? col.z*1.1f : 1.0f};
    Vec3 dark   = {col.x*0.7f, col.y*0.7f, col.z*0.7f};

    for (int i = 0; i < SIDES; i++) {
        float a0 = (float)i / SIDES * 2.0f * (float)M_PI;
        float a1 = (float)(i+1) / SIDES * 2.0f * (float)M_PI;
        push_vert(buf, 0, 0, thickness*0.5f, bright.x, bright.y, bright.z);
        push_vert(buf, cosf(a0)*radius, sinf(a0)*radius, thickness*0.5f, col.x, col.y, col.z);
        push_vert(buf, cosf(a1)*radius, sinf(a1)*radius, thickness*0.5f, col.x, col.y, col.z);
    }
    for (int i = 0; i < SIDES; i++) {
        float a0 = (float)i / SIDES * 2.0f * (float)M_PI;
        float a1 = (float)(i+1) / SIDES * 2.0f * (float)M_PI;
        push_vert(buf, 0, 0, -thickness*0.5f, dark.x, dark.y, dark.z);
        push_vert(buf, cosf(a1)*radius, sinf(a1)*radius, -thickness*0.5f, dark.x, dark.y, dark.z);
        push_vert(buf, cosf(a0)*radius, sinf(a0)*radius, -thickness*0.5f, dark.x, dark.y, dark.z);
    }
    for (int i = 0; i < SIDES; i++) {
        float a0 = (float)i / SIDES * 2.0f * (float)M_PI;
        float a1 = (float)(i+1) / SIDES * 2.0f * (float)M_PI;
        float cx0 = cosf(a0)*radius, sy0 = sinf(a0)*radius;
        float cx1 = cosf(a1)*radius, sy1 = sinf(a1)*radius;
        push_vert(buf, cx0, sy0,  thickness*0.5f, col.x*0.6f, col.y*0.6f, col.z*0.6f);
        push_vert(buf, cx1, sy1,  thickness*0.5f, col.x*0.6f, col.y*0.6f, col.z*0.6f);
        push_vert(buf, cx1, sy1, -thickness*0.5f, col.x*0.6f, col.y*0.6f, col.z*0.6f);
        push_vert(buf, cx0, sy0,  thickness*0.5f, col.x*0.6f, col.y*0.6f, col.z*0.6f);
        push_vert(buf, cx1, sy1, -thickness*0.5f, col.x*0.6f, col.y*0.6f, col.z*0.6f);
        push_vert(buf, cx0, sy0, -thickness*0.5f, col.x*0.6f, col.y*0.6f, col.z*0.6f);
    }

    return buf;
}

// ============================================================
// HUD — 7-segment digit rendering
// ============================================================

static const bool SEGMENTS[10][7] = {
    {1,1,1,1,1,1,0}, // 0
    {0,1,1,0,0,0,0}, // 1
    {1,1,0,1,1,0,1}, // 2
    {1,1,1,1,0,0,1}, // 3
    {0,1,1,0,0,1,1}, // 4
    {1,0,1,1,0,1,1}, // 5
    {1,0,1,1,1,1,1}, // 6
    {1,1,1,0,0,0,0}, // 7
    {1,1,1,1,1,1,1}, // 8
    {1,1,1,1,0,1,1}, // 9
};

static std::vector<float> gen_hud_digit(int digit, float ox, float oy, float scale) {
    std::vector<float> buf;
    if (digit < 0 || digit > 9) return buf;

    float w = 4.0f * scale;
    float h = 2.0f * scale;
    float dw = w + 2.0f * scale;
    float dh = dw * 2.0f;
    (void)dh;

    struct SegRect { float x, y, sw, sh; };

    float sw = w, sh = h;
    float vw = h, vh = w;

    SegRect segs[7] = {
        {ox + scale, oy + 2*w + 2*scale, sw, sh},
        {ox + w + scale, oy + w + 2*scale, vw, vh},
        {ox + w + scale, oy + scale, vw, vh},
        {ox + scale, oy, sw, sh},
        {ox, oy + scale, vw, vh},
        {ox, oy + w + 2*scale, vw, vh},
        {ox + scale, oy + w + scale, sw, sh},
    };

    Vec3 col = {1.0f, 1.0f, 1.0f};

    for (int i = 0; i < 7; i++) {
        if (!SEGMENTS[digit][i]) continue;
        float sx = segs[i].x, sy = segs[i].y;
        float ex = sx + segs[i].sw, ey = sy + segs[i].sh;
        push_vert(buf, sx, sy, 0, col.x, col.y, col.z);
        push_vert(buf, ex, sy, 0, col.x, col.y, col.z);
        push_vert(buf, ex, ey, 0, col.x, col.y, col.z);
        push_vert(buf, sx, sy, 0, col.x, col.y, col.z);
        push_vert(buf, ex, ey, 0, col.x, col.y, col.z);
        push_vert(buf, sx, ey, 0, col.x, col.y, col.z);
    }
    return buf;
}

static std::vector<float> gen_hud_coin_icon(float cx, float cy, float radius) {
    std::vector<float> buf;
    const int SIDES = 8;
    Vec3 col = {1.0f, 0.85f, 0.0f};
    for (int i = 0; i < SIDES; i++) {
        float a0 = (float)i / SIDES * 2.0f * (float)M_PI;
        float a1 = (float)(i+1) / SIDES * 2.0f * (float)M_PI;
        push_vert(buf, cx, cy, 0, col.x*1.1f, col.y*1.1f, col.z);
        push_vert(buf, cx + cosf(a0)*radius, cy + sinf(a0)*radius, 0, col.x, col.y, col.z);
        push_vert(buf, cx + cosf(a1)*radius, cy + sinf(a1)*radius, 0, col.x, col.y, col.z);
    }
    return buf;
}

// ============================================================
// AABB collision
// ============================================================
static bool aabb_overlap(Vec3 pmin, Vec3 pmax, Vec3 bmin, Vec3 bmax) {
    return pmax.x > bmin.x && pmin.x < bmax.x &&
           pmax.y > bmin.y && pmin.y < bmax.y &&
           pmax.z > bmin.z && pmin.z < bmax.z;
}

// ============================================================
// Platform data struct
// ============================================================
struct PlatformData {
    Vec3 min, max;
    Vec3 color;
};

// ============================================================
// Mesh — RAII VAO/VBO wrapper
// ============================================================
class Mesh {
public:
    Mesh() : vao_(0), vbo_(0), vertex_count_(0) {}

    explicit Mesh(const std::vector<float>& data) {
        vertex_count_ = (int)data.size() / 6;
        glGenVertexArrays(1, &vao_);
        glBindVertexArray(vao_);
        glGenBuffers(1, &vbo_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(data.size() * sizeof(float)),
                     data.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
    }

    ~Mesh() {
        if (vbo_ != 0) glDeleteBuffers(1, &vbo_);
        if (vao_ != 0) glDeleteVertexArrays(1, &vao_);
    }

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    Mesh(Mesh&& o) noexcept : vao_(o.vao_), vbo_(o.vbo_), vertex_count_(o.vertex_count_) {
        o.vao_ = 0; o.vbo_ = 0; o.vertex_count_ = 0;
    }

    Mesh& operator=(Mesh&& o) noexcept {
        if (this != &o) {
            if (vbo_ != 0) glDeleteBuffers(1, &vbo_);
            if (vao_ != 0) glDeleteVertexArrays(1, &vao_);
            vao_ = o.vao_; vbo_ = o.vbo_; vertex_count_ = o.vertex_count_;
            o.vao_ = 0; o.vbo_ = 0; o.vertex_count_ = 0;
        }
        return *this;
    }

    void draw() const {
        glBindVertexArray(vao_);
        glDrawArrays(GL_TRIANGLES, 0, vertex_count_);
    }

    bool valid() const { return vao_ != 0; }

private:
    GLuint vao_, vbo_;
    int vertex_count_;
};

// ============================================================
// ShaderProgram — wraps GL program + uniforms
// ============================================================
static GLuint compile_shader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, 512, nullptr, log);
        SDL_Log("Shader compile error: %s", log);
    }
    return s;
}

class ShaderProgram {
public:
    ShaderProgram() : program_(0), mvp_loc_(-1) {}

    ShaderProgram(const char* vs_src, const char* fs_src) {
        GLuint vs = compile_shader(GL_VERTEX_SHADER, vs_src);
        GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fs_src);
        program_ = glCreateProgram();
        glAttachShader(program_, vs);
        glAttachShader(program_, fs);
        glLinkProgram(program_);
        GLint ok;
        glGetProgramiv(program_, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[512];
            glGetProgramInfoLog(program_, 512, nullptr, log);
            SDL_Log("Program link error: %s", log);
        }
        glDeleteShader(vs);
        glDeleteShader(fs);
        mvp_loc_ = glGetUniformLocation(program_, "uMVP");
    }

    void use() const { glUseProgram(program_); }
    void set_mvp(const Mat4& mvp) const { glUniformMatrix4fv(mvp_loc_, 1, GL_FALSE, mvp.m); }

private:
    GLuint program_;
    GLint mvp_loc_;
};

// ============================================================
// Camera — smooth third-person follow
// ============================================================
class Camera {
public:
    Camera() : pos_({0, 4, 6}), yaw_(0) {}

    void follow(Vec3 target_pos, float target_yaw, float dt) {
        float yaw_diff = target_yaw - yaw_;
        while (yaw_diff > (float)M_PI)  yaw_diff -= 2.0f * (float)M_PI;
        while (yaw_diff < -(float)M_PI) yaw_diff += 2.0f * (float)M_PI;
        yaw_ += yaw_diff * CAM_LERP * dt;

        Vec3 cam_forward = {sinf(yaw_), 0, cosf(yaw_)};
        pos_ = target_pos - cam_forward * CAM_DIST + Vec3(0, CAM_HEIGHT, 0);
    }

    Mat4 view_matrix(Vec3 target_pos) const {
        Vec3 cam_target = target_pos + Vec3(0, 0.75f, 0);
        return mat4_look_at(pos_, cam_target, {0, 1, 0});
    }

private:
    Vec3 pos_;
    float yaw_;
};

// ============================================================
// Player — physics, input, collision, rendering
// ============================================================
class Player {
public:
    Player() : pos_({0, 1, 0}), vel_({0, 0, 0}), yaw_(0), on_ground_(false),
               mesh_(gen_box({-HALF_W, 0, -HALF_W}, {HALF_W, HEIGHT, HALF_W}, {0.9f, 0.15f, 0.15f})) {}

    void handle_input(float move, float turn, bool jump, float dt) {
        yaw_ += turn * TURN_SPEED * dt;
        Vec3 forward = {sinf(yaw_), 0, cosf(yaw_)};
        vel_.x = forward.x * move * MOVE_SPEED;
        vel_.z = forward.z * move * MOVE_SPEED;

        if (jump && on_ground_) {
            vel_.y = JUMP_VEL;
            on_ground_ = false;
        }
    }

    void update_physics(const PlatformData* platforms, int count, float dt) {
        vel_.y += GRAVITY * dt;
        Vec3 new_pos = pos_ + vel_ * dt;

        Vec3 pmin = {new_pos.x - HALF_W, new_pos.y, new_pos.z - HALF_W};
        Vec3 pmax = {new_pos.x + HALF_W, new_pos.y + HEIGHT, new_pos.z + HALF_W};

        on_ground_ = false;

        for (int i = 0; i < count; i++) {
            const PlatformData& pl = platforms[i];
            if (!aabb_overlap(pmin, pmax, pl.min, pl.max)) continue;

            float overlaps[6] = {
                pmax.x - pl.min.x,
                pl.max.x - pmin.x,
                pmax.y - pl.min.y,
                pl.max.y - pmin.y,
                pmax.z - pl.min.z,
                pl.max.z - pmin.z,
            };

            int min_axis = 0;
            float min_overlap = overlaps[0];
            for (int j = 1; j < 6; j++) {
                if (overlaps[j] < min_overlap) {
                    min_overlap = overlaps[j];
                    min_axis = j;
                }
            }

            switch (min_axis) {
                case 0: new_pos.x = pl.min.x - HALF_W;  vel_.x = 0; break;
                case 1: new_pos.x = pl.max.x + HALF_W;  vel_.x = 0; break;
                case 2: new_pos.y = pl.min.y - HEIGHT;   vel_.y = 0; break;
                case 3: new_pos.y = pl.max.y;            vel_.y = 0; on_ground_ = true; break;
                case 4: new_pos.z = pl.min.z - HALF_W;   vel_.z = 0; break;
                case 5: new_pos.z = pl.max.z + HALF_W;   vel_.z = 0; break;
            }

            pmin = {new_pos.x - HALF_W, new_pos.y, new_pos.z - HALF_W};
            pmax = {new_pos.x + HALF_W, new_pos.y + HEIGHT, new_pos.z + HALF_W};
        }

        pos_ = new_pos;

        if (pos_.y < RESPAWN_Y) {
            pos_ = {0, 1, 0};
            vel_ = {0, 0, 0};
            on_ground_ = false;
        }
    }

    void draw(const ShaderProgram& shader, const Mat4& vp) const {
        Mat4 model = mat4_translate(pos_) * mat4_rotate_y(yaw_);
        shader.set_mvp(vp * model);
        mesh_.draw();
    }

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

// ============================================================
// Coin — spinning collectible
// ============================================================
class Coin {
public:
    Coin() : pos_({0,0,0}), spin_angle_(0), collected_(false) {}
    Coin(Vec3 pos) : pos_(pos), spin_angle_(0), collected_(false) {}

    void update(float dt) { spin_angle_ += 3.0f * dt; }

    bool try_collect(Vec3 player_pos) {
        if (collected_) return false;
        Vec3 diff = player_pos - pos_;
        diff.y += 0.75f;
        if (diff.length() < COIN_COLLECT_DIST) {
            collected_ = true;
            return true;
        }
        return false;
    }

    void draw(const ShaderProgram& shader, const Mat4& vp, const Mesh& shared_mesh) const {
        if (collected_) return;
        Mat4 model = mat4_translate(pos_ + Vec3(0, 0.4f, 0)) * mat4_rotate_y(spin_angle_);
        shader.set_mvp(vp * model);
        shared_mesh.draw();
    }

    bool collected() const { return collected_; }

private:
    Vec3 pos_;
    float spin_angle_;
    bool collected_;
};

// ============================================================
// Level — owns platforms, coins, shared coin mesh
// ============================================================
class Level {
public:
    Level() {
        PlatformData platform_defs[] = {
            {{-15, -0.5f, -15}, {15, 0, 15}, {0.2f, 0.7f, 0.2f}},
            {{3, 0, 3},    {7, 1.5f, 7},   {0.6f, 0.4f, 0.2f}},
            {{-8, 0, -3},  {-4, 2.5f, 1},  {0.5f, 0.5f, 0.5f}},
            {{-3, 0, 8},   {1, 1.0f, 12},  {0.6f, 0.4f, 0.2f}},
            {{8, 0, -8},   {12, 3.5f, -4}, {0.5f, 0.5f, 0.5f}},
            {{-10, 0, -10},{-6, 4.0f, -7}, {0.6f, 0.4f, 0.2f}},
            {{5, 0, -2},   {9, 2.0f, 2},   {0.5f, 0.5f, 0.5f}},
        };

        int num_plats = sizeof(platform_defs) / sizeof(platform_defs[0]);
        for (int i = 0; i < num_plats; i++) {
            platforms_.push_back(platform_defs[i]);
            platform_meshes_.push_back(Mesh(gen_box(platform_defs[i].min, platform_defs[i].max, platform_defs[i].color)));
        }

        Vec3 coin_positions[] = {
            {5, 2.5f, 5}, {-6, 3.5f, -1}, {-1, 2.0f, 10}, {10, 4.5f, -6},
            {-8, 5.0f, -8.5f}, {7, 3.0f, 0}, {0, 1.0f, 0}, {-12, 1.0f, 12},
        };
        int num_coins = sizeof(coin_positions) / sizeof(coin_positions[0]);
        for (int i = 0; i < num_coins; i++) {
            coins_.push_back(Coin(coin_positions[i]));
        }
        total_coins_ = num_coins;
        coins_collected_ = 0;

        coin_mesh_ = Mesh(gen_octagon(0.4f, 0.1f, {1.0f, 0.85f, 0.0f}));
    }

    void update(float dt, Vec3 player_pos) {
        for (auto& c : coins_) {
            c.update(dt);
            if (c.try_collect(player_pos)) {
                coins_collected_++;
            }
        }
    }

    void draw(const ShaderProgram& shader, const Mat4& vp) const {
        for (size_t i = 0; i < platforms_.size(); i++) {
            shader.set_mvp(vp);
            platform_meshes_[i].draw();
        }
        for (const auto& c : coins_) {
            c.draw(shader, vp, coin_mesh_);
        }
    }

    const PlatformData* platform_data() const { return platforms_.data(); }
    int platform_count() const { return (int)platforms_.size(); }
    int coins_collected() const { return coins_collected_; }
    int total_coins() const { return total_coins_; }

private:
    std::vector<PlatformData> platforms_;
    std::vector<Mesh> platform_meshes_;
    std::vector<Coin> coins_;
    Mesh coin_mesh_;
    int coins_collected_;
    int total_coins_;
};

// ============================================================
// HUD — cached coin counter display
// ============================================================
class HUD {
public:
    HUD(int total_coins) : cached_collected_(-1), cached_win_h_(-1), total_coins_(total_coins) {}

    void build_static(int win_h) {
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

    void update(int coins_collected, int win_h) {
        if (win_h != cached_win_h_) {
            build_static(win_h);
            cached_win_h_ = win_h;
            cached_collected_ = -1; // force rebuild of collected digit
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

    void draw(const ShaderProgram& shader, int win_w, int win_h) const {
        glDisable(GL_DEPTH_TEST);
        Mat4 ortho = mat4_ortho(0, (float)win_w, 0, (float)win_h, -1, 1);
        shader.set_mvp(ortho);

        if (icon_mesh_.valid()) icon_mesh_.draw();
        if (collected_mesh_.valid()) collected_mesh_.draw();
        if (slash_mesh_.valid()) slash_mesh_.draw();
        if (total_mesh_.valid()) total_mesh_.draw();

        glEnable(GL_DEPTH_TEST);
    }

private:
    Mesh icon_mesh_;
    Mesh collected_mesh_;
    Mesh slash_mesh_;
    Mesh total_mesh_;
    int cached_collected_;
    int cached_win_h_;
    int total_coins_;
};

// ============================================================
// Main
// ============================================================
int main(int /*argc*/, char* /*argv*/[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window* window = SDL_CreateWindow("SM64 Platformer", SCREEN_W, SCREEN_H,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);
    if (!window) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext gl_ctx = SDL_GL_CreateContext(window);
    if (!gl_ctx) {
        SDL_Log("GL context creation failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_GL_SetSwapInterval(1);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClearColor(0.4f, 0.6f, 1.0f, 1.0f);

    ShaderProgram shader(VERT_SRC, FRAG_SRC);
    Level level;
    Player player;
    Camera camera;
    HUD hud(level.total_coins());

    Uint64 last_time = SDL_GetTicksNS();
    bool running = true;

    while (running) {
        Uint64 now = SDL_GetTicksNS();
        float dt = (float)(now - last_time) / 1e9f;
        last_time = now;
        if (dt > 0.05f) dt = 0.05f;

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_EVENT_QUIT) running = false;
            if (ev.type == SDL_EVENT_KEY_DOWN && ev.key.key == SDLK_ESCAPE) running = false;
        }

        const bool* keys = SDL_GetKeyboardState(nullptr);
        float move_input = 0, turn_input = 0;
        if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP])    move_input += 1;
        if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN])  move_input -= 1;
        if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT])  turn_input += 1;
        if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) turn_input -= 1;
        bool jump_pressed = keys[SDL_SCANCODE_SPACE];

        player.handle_input(move_input, turn_input, jump_pressed, dt);
        player.update_physics(level.platform_data(), level.platform_count(), dt);
        level.update(dt, player.pos());
        camera.follow(player.pos(), player.yaw(), dt);

        int win_w, win_h;
        SDL_GetWindowSizeInPixels(window, &win_w, &win_h);
        hud.update(level.coins_collected(), win_h);

        glViewport(0, 0, win_w, win_h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();
        Mat4 view = camera.view_matrix(player.pos());
        Mat4 proj = mat4_perspective(FOV, (float)win_w / (float)win_h, NEAR_PLANE, FAR_PLANE);
        Mat4 vp = proj * view;

        level.draw(shader, vp);
        player.draw(shader, vp);
        hud.draw(shader, win_w, win_h);

        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DestroyContext(gl_ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
