#ifndef BOUNCE_NX_SDL_H
#define BOUNCE_NX_SDL_H
#include <SDL2/SDL.h>
SDL_Renderer *graphics_nx_renderer(void);
void graphics_nx_set_game_view(int gameplay);
int graphics_nx_ready(void);
void nx_log(const char *format, ...);
#endif
