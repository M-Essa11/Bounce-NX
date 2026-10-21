#include "png.h"
#include "nx_sdl.h"
#include "types.h"
#include <SDL2/SDL_image.h>
#include <stdlib.h>
#include <string.h>

texture_t *png_load_texture_vram(const char *path) {
    FILE *f = util_open_file(path, "rb");
    if (!f) { nx_log("Missing image: %s", path); return NULL; }
    /* Close the FILE ourselves; the SDL RWops owns only its wrapper. */
    SDL_RWops *rw = SDL_RWFromFP(f, SDL_FALSE);
    SDL_Surface *surface = rw ? IMG_Load_RW(rw, 1) : NULL;
    fclose(f);
    if (!surface) { nx_log("Image %s: %s", path, IMG_GetError()); return NULL; }
    texture_t *t = calloc(1, sizeof(*t));
    if (t) {
        t->data = SDL_CreateTextureFromSurface(graphics_nx_renderer(), surface);
        t->width = t->actual_width = surface->w;
        t->height = t->actual_height = surface->h;
        t->source_scale = 1;
        if (t->data) {
            SDL_SetTextureBlendMode(t->data, SDL_BLENDMODE_BLEND);
            SDL_SetTextureScaleMode(t->data,
                strncmp(path, "icons/nx/", 9) == 0 ? SDL_ScaleModeLinear : SDL_ScaleModeNearest);
        }
        else { free(t); t = NULL; }
    }
    SDL_FreeSurface(surface);
    return t;
}
sprite_rect_t png_create_sprite_rect(texture_t *t, int x, int y, int w, int h) {
    int scale = t && t->source_scale > 0 ? t->source_scale : 1;
    return (sprite_rect_t){x * scale, y * scale, w * scale, h * scale};
}
void png_draw_sprite(texture_t *t, sprite_rect_t *s, int x, int y, int w, int h) {
    if (!t || !t->data || !s) return;
    SDL_Rect src = {s->x, s->y, s->w, s->h}, dest = {x, y, w, h};
    SDL_RenderCopy(graphics_nx_renderer(), t->data, &src, &dest);
}
void png_free_texture(texture_t *t) {
    if (!t) return;
    if (t->data) SDL_DestroyTexture(t->data);
    free(t);
}
void png_draw_sprite_uv4(texture_t *t, int utl, int vtl, int utr, int vtr,
    int ubl, int vbl, int ubr, int vbr, int x, int y, int w, int h) {
    if (!t || !t->data || t->width <= 0 || t->height <= 0 || w <= 0 || h <= 0) return;
    SDL_Color white = {255, 255, 255, 255};
    const float iw = 1.0f / t->width, ih = 1.0f / t->height;
    SDL_Vertex vertices[4] = {
        {{x, y}, white, {utl * iw, vtl * ih}},
        {{x+w, y}, white, {utr * iw, vtr * ih}},
        {{x, y+h}, white, {ubl * iw, vbl * ih}},
        {{x+w, y+h}, white, {ubr * iw, vbr * ih}}
    };
    const int indices[6] = {0, 1, 2, 2, 1, 3};
    SDL_RenderGeometry(graphics_nx_renderer(), t->data, vertices, 4, indices, 6);
}
void png_draw_sprite_transform(texture_t *t, sprite_rect_t *s, int x, int y, int w, int h,
    png_transform_t transform) {
    if (!t || !s) return;
    int u[4] = {s->x, s->x+s->w, s->x, s->x+s->w};
    int v[4] = {s->y, s->y, s->y+s->h, s->y+s->h};
    int idx[4] = {0, 1, 2, 3}, rotation = 0, fx = 0, fy = 0;
    switch (transform) {
        case PNG_TRANSFORM_ROT_90: rotation = 1; break;
        case PNG_TRANSFORM_ROT_180: rotation = 2; break;
        case PNG_TRANSFORM_ROT_270: rotation = 3; break;
        case PNG_TRANSFORM_FLIP_X: fx = 1; break;
        case PNG_TRANSFORM_FLIP_Y: fy = 1; break;
        case PNG_TRANSFORM_ROT_270_FLIP_X: rotation = 3; fx = 1; break;
        case PNG_TRANSFORM_ROT_270_FLIP_Y: rotation = 3; fy = 1; break;
        case PNG_TRANSFORM_ROT_270_FLIP_XY: rotation = 3; fx = fy = 1; break;
        default: break;
    }
    while (rotation--) {
        int a = idx[0], b = idx[1], c = idx[2], d = idx[3];
        idx[0] = b; idx[1] = d; idx[2] = a; idx[3] = c;
    }
    if (fx) { int a = idx[0], c = idx[2]; idx[0] = idx[1]; idx[1] = a; idx[2] = idx[3]; idx[3] = c; }
    if (fy) { int a = idx[0], b = idx[1]; idx[0] = idx[2]; idx[1] = idx[3]; idx[2] = a; idx[3] = b; }
    png_draw_sprite_uv4(t, u[idx[0]], v[idx[0]], u[idx[1]], v[idx[1]],
        u[idx[2]], v[idx[2]], u[idx[3]], v[idx[3]], x, y, w, h);
}
