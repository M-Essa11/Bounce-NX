#include "game.h"
#include "graphics.h"
#include "input.h"
#include "sound.h"
#include "nx_sdl.h"
#include <stdarg.h>
#include <sys/stat.h>
#ifdef __SWITCH__
#include <switch.h>
#endif

#define PHYSICS_DT_MS 30

void nx_log(const char *format, ...) {
#ifdef __SWITCH__
    FILE *f = fopen("sdmc:/switch/bounce-nx/bounce-nx.log", "a");
#else
    FILE *f = fopen("bounce-nx.log", "a");
#endif
    if (!f) return;
    va_list args; va_start(args, format); vfprintf(f, format, args); va_end(args);
    fputc('\n', f); fclose(f);
}
FILE *util_open_file(const char *path, const char *mode) {
    if (!path || !mode) return NULL;
#ifdef __SWITCH__
    char full_path[512];
    int n = snprintf(full_path, sizeof(full_path), "romfs:/%s", path);
    if (n < 0 || n >= (int)sizeof(full_path)) return NULL;
    return fopen(full_path, mode);
#else
    return fopen(path, mode);
#endif
}
static void checkpoint(void) {
    if (g_game.state == STATE_GAME) {
        save_set_progress(g_game.selected_level, g_game.score, g_game.numLives, 1);
    }
    save_flush();
}
int main(int argc, char **argv) {
    (void)argc; (void)argv;
#ifdef __SWITCH__
    mkdir("sdmc:/switch", 0777);
    mkdir("sdmc:/switch/bounce-nx", 0777);
    Result rc = romfsInit();
    if (R_FAILED(rc)) { nx_log("RomFS init failed: %08x", rc); return 1; }
    appletSetFocusHandlingMode(AppletFocusHandlingMode_NoSuspend);
#endif
    nx_log("Bounce NX 0.1.2 starting");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) {
        nx_log("SDL init: %s", SDL_GetError());
#ifdef __SWITCH__
        romfsExit();
#endif
        return 1;
    }
    graphics_init();
    if (!graphics_nx_ready()) {
        graphics_shutdown(); SDL_Quit();
#ifdef __SWITCH__
        romfsExit();
#endif
        return 1;
    }
    input_init();
    int audio_result = sound_init();
    nx_log("Audio init: %d%s", audio_result, audio_result == 1 ? " (ready)" : " (disabled)");
    save_init();
    game_init();
    Uint64 previous_time = SDL_GetTicks64();
    int accumulator = 0, running = 1, focused = 1;
    GameState previous_state = g_game.state;
    while (running && g_game.state != STATE_EXIT) {
#ifdef __SWITCH__
        if (!appletMainLoop()) break;
        int now_focused = appletGetFocusState() == AppletFocusState_InFocus;
        if (focused && !now_focused) {
            checkpoint();
            if (g_game.state == STATE_GAME) g_game.state = STATE_MENU;
        }
        focused = now_focused;
#endif
        SDL_Event event;
        while (SDL_PollEvent(&event)) if (event.type == SDL_QUIT) running = 0;
        if (!running) break;
        if (!focused) { previous_time = SDL_GetTicks64(); accumulator = 0; SDL_Delay(20); continue; }
        input_update();
        if (input_pressed(PSP_CTRL_SELECT)) break;
        Uint64 now = SDL_GetTicks64();
        Uint64 delta = now - previous_time;
        previous_time = now;
        if (delta > 200) delta = 200;
        if (g_game.state != previous_state) {
            accumulator = 0;
            input_reset_edges();
            previous_state = g_game.state;
        }
        const game_state_handler_t *handler = game_get_state_handler(g_game.state);
        if (!handler) break;
        if (handler->tick_mode == GAME_TICK_FIXED) {
            if (input_pressed(PSP_CTRL_START)) {
                checkpoint();
                g_game.saved_game_state = SAVED_GAME_IN_PROGRESS;
                g_game.state = STATE_MENU;
                accumulator = 0;
            } else {
                accumulator += (int)delta;
                while (accumulator >= PHYSICS_DT_MS) {
                    GameState before = g_game.state;
                    game_state_update();
                    accumulator -= PHYSICS_DT_MS;
                    if (g_game.state != before) { accumulator = 0; break; }
                }
            }
        } else {
            accumulator = 0;
            game_state_update();
        }
        graphics_nx_set_game_view(g_game.state == STATE_GAME);
        graphics_start_frame();
        game_state_render();
        graphics_end_frame();
        SDL_Delay(1);
    }
    checkpoint();
    game_shutdown(); save_shutdown(); sound_shutdown(); graphics_shutdown();
    SDL_Quit();
#ifdef __SWITCH__
    romfsExit();
#endif
    return 0;
}
