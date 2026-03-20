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
// Game Structs
// ============================================================
struct Player {
    Vec3 pos;
    Vec3 vel;
    float yaw;
    bool on_ground;
};

struct Coin {
    Vec3 pos;
    float spin_angle;
    bool collected;
};

struct Platform {
    Vec3 min, max;
    Vec3 color;
};

struct Camera {
    Vec3 pos;
    float yaw;
};

struct Mesh {
    GLuint vao, vbo;
    int vertex_count;
};

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

// Push a vertex (pos + color) into the buffer
static void push_vert(std::vector<float>& buf, float x, float y, float z,
                       float r, float g, float b) {
    buf.push_back(x); buf.push_back(y); buf.push_back(z);
    buf.push_back(r); buf.push_back(g); buf.push_back(b);
}

// Generate a box with face-based shading (top bright, sides medium, bottom dark)
static std::vector<float> gen_box(Vec3 mn, Vec3 mx, Vec3 col) {
    std::vector<float> buf;
    buf.reserve(36 * 6);

    float top_f = 1.1f, side_f = 0.8f, bot_f = 0.5f;
    Vec3 ct = {col.x*top_f, col.y*top_f, col.z*top_f};
    Vec3 cs = {col.x*side_f, col.y*side_f, col.z*side_f};
    Vec3 cb = {col.x*bot_f, col.y*bot_f, col.z*bot_f};
    // clamp
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

    // Right face (+X) — slightly different shade
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

// Generate an octagonal coin (8-triangle fan, both sides)
static std::vector<float> gen_octagon(float radius, float thickness, Vec3 col) {
    std::vector<float> buf;
    const int SIDES = 8;
    Vec3 bright = {col.x*1.1f < 1 ? col.x*1.1f : 1.0f,
                   col.y*1.1f < 1 ? col.y*1.1f : 1.0f,
                   col.z*1.1f < 1 ? col.z*1.1f : 1.0f};
    Vec3 dark   = {col.x*0.7f, col.y*0.7f, col.z*0.7f};

    // Front face
    for (int i = 0; i < SIDES; i++) {
        float a0 = (float)i / SIDES * 2.0f * (float)M_PI;
        float a1 = (float)(i+1) / SIDES * 2.0f * (float)M_PI;
        push_vert(buf, 0, 0, thickness*0.5f, bright.x, bright.y, bright.z);
        push_vert(buf, cosf(a0)*radius, sinf(a0)*radius, thickness*0.5f, col.x, col.y, col.z);
        push_vert(buf, cosf(a1)*radius, sinf(a1)*radius, thickness*0.5f, col.x, col.y, col.z);
    }
    // Back face
    for (int i = 0; i < SIDES; i++) {
        float a0 = (float)i / SIDES * 2.0f * (float)M_PI;
        float a1 = (float)(i+1) / SIDES * 2.0f * (float)M_PI;
        push_vert(buf, 0, 0, -thickness*0.5f, dark.x, dark.y, dark.z);
        push_vert(buf, cosf(a1)*radius, sinf(a1)*radius, -thickness*0.5f, dark.x, dark.y, dark.z);
        push_vert(buf, cosf(a0)*radius, sinf(a0)*radius, -thickness*0.5f, dark.x, dark.y, dark.z);
    }
    // Edge ring
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
// Mesh upload/draw
// ============================================================
static Mesh upload_mesh(const std::vector<float>& data) {
    Mesh m;
    m.vertex_count = (int)data.size() / 6;
    glGenVertexArrays(1, &m.vao);
    glBindVertexArray(m.vao);
    glGenBuffers(1, &m.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(data.size() * sizeof(float)),
                 data.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    return m;
}

static void draw_mesh(const Mesh& m) {
    glBindVertexArray(m.vao);
    glDrawArrays(GL_TRIANGLES, 0, m.vertex_count);
}

// ============================================================
// Shader compilation
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

static GLuint create_program(const char* vs_src, const char* fs_src) {
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vs_src);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fs_src);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(prog, 512, nullptr, log);
        SDL_Log("Program link error: %s", log);
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// ============================================================
// HUD — 7-segment digit rendering
// ============================================================

// Segment layout for 7-segment display:
//  _0_
// |5  |1
//  _6_
// |4  |2
//  _3_
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

    float w = 4.0f * scale;  // segment width
    float h = 2.0f * scale;  // segment thickness
    float dw = w + 2.0f * scale; // digit total width
    float dh = dw * 2.0f;        // digit total height
    (void)dh;

    struct SegRect { float x, y, sw, sh; };

    // Define each segment as a rectangle
    float sw = w, sh = h;
    float vw = h, vh = w; // vertical segments are rotated

    SegRect segs[7] = {
        {ox + scale, oy + 2*w + 2*scale, sw, sh},   // 0 top
        {ox + w + scale, oy + w + 2*scale, vw, vh},  // 1 top-right
        {ox + w + scale, oy + scale, vw, vh},         // 2 bot-right
        {ox + scale, oy, sw, sh},                      // 3 bottom
        {ox, oy + scale, vw, vh},                      // 4 bot-left
        {ox, oy + w + 2*scale, vw, vh},               // 5 top-left
        {ox + scale, oy + w + scale, sw, sh},         // 6 middle
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

// Generate a small coin icon for HUD
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
// Level Data
// ============================================================
static Platform platforms[] = {
    // Ground
    {{-15, -0.5f, -15}, {15, 0, 15}, {0.2f, 0.7f, 0.2f}},
    // Raised platforms
    {{3, 0, 3},    {7, 1.5f, 7},   {0.6f, 0.4f, 0.2f}},
    {{-8, 0, -3},  {-4, 2.5f, 1},  {0.5f, 0.5f, 0.5f}},
    {{-3, 0, 8},   {1, 1.0f, 12},  {0.6f, 0.4f, 0.2f}},
    {{8, 0, -8},   {12, 3.5f, -4}, {0.5f, 0.5f, 0.5f}},
    {{-10, 0, -10},{-6, 4.0f, -7}, {0.6f, 0.4f, 0.2f}},
    {{5, 0, -2},   {9, 2.0f, 2},   {0.5f, 0.5f, 0.5f}},
};
static const int NUM_PLATFORMS = sizeof(platforms) / sizeof(platforms[0]);

static Coin coins[] = {
    {{5, 2.5f, 5}, 0, false},
    {{-6, 3.5f, -1}, 0, false},
    {{-1, 2.0f, 10}, 0, false},
    {{10, 4.5f, -6}, 0, false},
    {{-8, 5.0f, -8.5f}, 0, false},
    {{7, 3.0f, 0}, 0, false},
    {{0, 1.0f, 0}, 0, false},
    {{-12, 1.0f, 12}, 0, false},
};
static const int NUM_COINS = sizeof(coins) / sizeof(coins[0]);

// ============================================================
// AABB collision
// ============================================================
static bool aabb_overlap(Vec3 pmin, Vec3 pmax, Vec3 bmin, Vec3 bmax) {
    return pmax.x > bmin.x && pmin.x < bmax.x &&
           pmax.y > bmin.y && pmin.y < bmax.y &&
           pmax.z > bmin.z && pmin.z < bmax.z;
}

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
    SDL_GL_SetSwapInterval(1); // vsync

    // Compile shaders
    GLuint program = create_program(VERT_SRC, FRAG_SRC);
    GLint mvp_loc = glGetUniformLocation(program, "uMVP");

    // Generate platform meshes
    Mesh platform_meshes[NUM_PLATFORMS];
    for (int i = 0; i < NUM_PLATFORMS; i++) {
        auto data = gen_box(platforms[i].min, platforms[i].max, platforms[i].color);
        platform_meshes[i] = upload_mesh(data);
    }

    // Player mesh
    auto player_data = gen_box({-0.5f, 0, -0.5f}, {0.5f, 1.5f, 0.5f}, {0.9f, 0.15f, 0.15f});
    Mesh player_mesh = upload_mesh(player_data);

    // Coin mesh
    auto coin_data = gen_octagon(0.4f, 0.1f, {1.0f, 0.85f, 0.0f});
    Mesh coin_mesh = upload_mesh(coin_data);

    // Init game state
    Player player = {{0, 1, 0}, {0,0,0}, 0, false};
    Camera cam = {{0, 4, 6}, 0};
    int coins_collected = 0;

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClearColor(0.4f, 0.6f, 1.0f, 1.0f);

    Uint64 last_time = SDL_GetTicksNS();
    bool running = true;

    while (running) {
        // Delta time
        Uint64 now = SDL_GetTicksNS();
        float dt = (float)(now - last_time) / 1e9f;
        last_time = now;
        if (dt > 0.05f) dt = 0.05f;

        // Events
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_EVENT_QUIT) running = false;
            if (ev.type == SDL_EVENT_KEY_DOWN && ev.key.key == SDLK_ESCAPE) running = false;
        }

        // Input
        const bool* keys = SDL_GetKeyboardState(nullptr);
        float move_input = 0, turn_input = 0;
        if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP])    move_input += 1;
        if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN])  move_input -= 1;
        if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT])  turn_input += 1;
        if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) turn_input -= 1;
        bool jump_pressed = keys[SDL_SCANCODE_SPACE];

        // Player movement (tank controls)
        player.yaw += turn_input * TURN_SPEED * dt;
        Vec3 forward = {sinf(player.yaw), 0, cosf(player.yaw)};
        player.vel.x = forward.x * move_input * MOVE_SPEED;
        player.vel.z = forward.z * move_input * MOVE_SPEED;

        // Jump
        if (jump_pressed && player.on_ground) {
            player.vel.y = JUMP_VEL;
            player.on_ground = false;
        }

        // Gravity
        player.vel.y += GRAVITY * dt;

        // Move + collide
        Vec3 new_pos = player.pos + player.vel * dt;

        // Player AABB (0.5 half-width, 1.5 height)
        Vec3 pmin = {new_pos.x - 0.5f, new_pos.y, new_pos.z - 0.5f};
        Vec3 pmax = {new_pos.x + 0.5f, new_pos.y + 1.5f, new_pos.z + 0.5f};

        player.on_ground = false;

        for (int i = 0; i < NUM_PLATFORMS; i++) {
            Platform& pl = platforms[i];
            if (!aabb_overlap(pmin, pmax, pl.min, pl.max)) continue;

            // Determine push-out direction (smallest overlap axis)
            float overlaps[6] = {
                pmax.x - pl.min.x, // push -x
                pl.max.x - pmin.x, // push +x
                pmax.y - pl.min.y, // push -y
                pl.max.y - pmin.y, // push +y
                pmax.z - pl.min.z, // push -z
                pl.max.z - pmin.z, // push +z
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
                case 0: new_pos.x = pl.min.x - 0.5f; player.vel.x = 0; break;
                case 1: new_pos.x = pl.max.x + 0.5f; player.vel.x = 0; break;
                case 2: new_pos.y = pl.min.y - 1.5f; player.vel.y = 0; break;
                case 3: new_pos.y = pl.max.y;         player.vel.y = 0; player.on_ground = true; break;
                case 4: new_pos.z = pl.min.z - 0.5f; player.vel.z = 0; break;
                case 5: new_pos.z = pl.max.z + 0.5f; player.vel.z = 0; break;
            }

            // Recompute AABB after push
            pmin = {new_pos.x - 0.5f, new_pos.y, new_pos.z - 0.5f};
            pmax = {new_pos.x + 0.5f, new_pos.y + 1.5f, new_pos.z + 0.5f};
        }

        player.pos = new_pos;

        // Respawn
        if (player.pos.y < RESPAWN_Y) {
            player.pos = {0, 1, 0};
            player.vel = {0, 0, 0};
            player.on_ground = false;
        }

        // Coin collection
        for (int i = 0; i < NUM_COINS; i++) {
            if (coins[i].collected) continue;
            Vec3 diff = player.pos - coins[i].pos;
            diff.y += 0.75f; // compare to player center
            if (diff.length() < COIN_COLLECT_DIST) {
                coins[i].collected = true;
                coins_collected++;
            }
        }

        // Coin spin
        for (int i = 0; i < NUM_COINS; i++) {
            coins[i].spin_angle += 3.0f * dt;
        }

        // Camera
        // Smoothly lerp yaw toward player yaw
        float yaw_diff = player.yaw - cam.yaw;
        // Normalize to [-pi, pi]
        while (yaw_diff > (float)M_PI)  yaw_diff -= 2.0f * (float)M_PI;
        while (yaw_diff < -(float)M_PI) yaw_diff += 2.0f * (float)M_PI;
        cam.yaw += yaw_diff * CAM_LERP * dt;

        Vec3 cam_forward = {sinf(cam.yaw), 0, cosf(cam.yaw)};
        cam.pos = player.pos - cam_forward * CAM_DIST + Vec3(0, CAM_HEIGHT, 0);
        Vec3 cam_target = player.pos + Vec3(0, 0.75f, 0);

        // ---- Rendering ----
        int win_w, win_h;
        SDL_GetWindowSizeInPixels(window, &win_w, &win_h);
        glViewport(0, 0, win_w, win_h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(program);

        Mat4 view = mat4_look_at(cam.pos, cam_target, {0,1,0});
        Mat4 proj = mat4_perspective(FOV, (float)win_w / (float)win_h, NEAR_PLANE, FAR_PLANE);
        Mat4 vp = proj * view;

        // Draw platforms
        for (int i = 0; i < NUM_PLATFORMS; i++) {
            Mat4 model = Mat4::identity();
            Mat4 mvp = vp * model;
            glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, mvp.m);
            draw_mesh(platform_meshes[i]);
        }

        // Draw player
        {
            Mat4 model = mat4_translate(player.pos) * mat4_rotate_y(player.yaw);
            Mat4 mvp = vp * model;
            glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, mvp.m);
            draw_mesh(player_mesh);
        }

        // Draw coins
        for (int i = 0; i < NUM_COINS; i++) {
            if (coins[i].collected) continue;
            Mat4 model = mat4_translate(coins[i].pos + Vec3(0, 0.4f, 0))
                       * mat4_rotate_y(coins[i].spin_angle);
            Mat4 mvp = vp * model;
            glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, mvp.m);
            draw_mesh(coin_mesh);
        }

        // ---- HUD ----
        glDisable(GL_DEPTH_TEST);
        Mat4 ortho = mat4_ortho(0, (float)win_w, 0, (float)win_h, -1, 1);

        // Coin icon
        {
            auto icon_data = gen_hud_coin_icon(30, (float)win_h - 30, 12);
            Mesh icon = upload_mesh(icon_data);
            glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, ortho.m);
            draw_mesh(icon);
            // We leak these VBOs — acceptable for a simple game
            // In production, we'd cache or delete them
        }

        // Digit: coins_collected
        {
            auto d1 = gen_hud_digit(coins_collected % 10, 50, (float)win_h - 50, 2.5f);
            if (!d1.empty()) {
                Mesh dm = upload_mesh(d1);
                glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, ortho.m);
                draw_mesh(dm);
            }
        }

        // Slash
        {
            std::vector<float> slash;
            float sx = 80, sy = (float)win_h - 45;
            push_vert(slash, sx, sy, 0, 1,1,1);
            push_vert(slash, sx+3, sy, 0, 1,1,1);
            push_vert(slash, sx+10, sy+20, 0, 1,1,1);
            push_vert(slash, sx+3, sy, 0, 1,1,1);
            push_vert(slash, sx+10, sy+20, 0, 1,1,1);
            push_vert(slash, sx+7, sy+20, 0, 1,1,1);
            Mesh sm = upload_mesh(slash);
            glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, ortho.m);
            draw_mesh(sm);
        }

        // Total coins
        {
            auto d2 = gen_hud_digit(NUM_COINS % 10, 95, (float)win_h - 50, 2.5f);
            if (!d2.empty()) {
                Mesh dm = upload_mesh(d2);
                glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, ortho.m);
                draw_mesh(dm);
            }
        }

        glEnable(GL_DEPTH_TEST);

        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    SDL_GL_DestroyContext(gl_ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
