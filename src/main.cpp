#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>

#include "constants.h"
#include "renderer.h"
#include "camera.h"
#include "player.h"
#include "level.h"
#include "hud.h"

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

    ShaderProgram shader = ShaderProgram::create_default();
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

        // 60 FPS cap
        Uint64 frame_end = SDL_GetTicksNS();
        Uint64 frame_time = frame_end - now;
        if (frame_time < FRAME_TIME_NS) {
            SDL_DelayNS(FRAME_TIME_NS - frame_time);
        }
    }

    SDL_GL_DestroyContext(gl_ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
