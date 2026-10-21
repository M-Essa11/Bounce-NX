#include "graphics.h"
#include "types.h"
#include "cbmf.h"
#include "nx_sdl.h"
#include <SDL2/SDL_image.h>
#include <stdlib.h>
#include <string.h>

#define FONT_COUNT 5
static const char *font_paths[FONT_COUNT] = {
    "fonts/0012-0002.cbmf", "fonts/0015-0064.cbmf", "fonts/0019-0128.cbmf",
    "fonts/0026-0002.cbmf", "fonts/0027-0032.cbmf"
};
static const int heights[FONT_COUNT] = {9, 12, 16, 23, 24};
static CbmfFont fonts[FONT_COUNT];
static void *font_data[FONT_COUNT];
typedef struct GlyphCache {
    unsigned int codepoint;
    int font;
    CbmfGlyphView glyph;
    SDL_Texture *texture;
    struct GlyphCache *next;
} GlyphCache;
static GlyphCache *glyphs;
static SDL_Window *window;
static SDL_Renderer *renderer;
static int texturing, ready;
static texture_t *bound_texture;

SDL_Renderer *graphics_nx_renderer(void) { return renderer; }
int graphics_nx_ready(void) { return ready; }
void graphics_nx_set_game_view(int gameplay) {
    SDL_RenderSetLogicalSize(renderer, gameplay ? 320 : SCREEN_WIDTH,
        gameplay ? 180 : SCREEN_HEIGHT);
}
static void set_color(u32 color) {
    SDL_SetRenderDrawColor(renderer, color & 255, (color >> 8) & 255,
        (color >> 16) & 255, color >> 24);
}
static int font_index(int height) {
    for (int i = 0; i < FONT_COUNT; ++i) if (height == heights[i]) return i;
    return 0;
}
static unsigned int next_codepoint(const unsigned char **cursor) {
    const unsigned char *p = *cursor;
    unsigned int cp = *p++;
    int extra = 0;
    if (cp >= 0xF0 && cp <= 0xF4) { cp &= 7; extra = 3; }
    else if (cp >= 0xE0 && cp <= 0xEF) { cp &= 15; extra = 2; }
    else if (cp >= 0xC2 && cp <= 0xDF) { cp &= 31; extra = 1; }
    else if (cp >= 128) cp = '?';
    while (extra--) {
        if ((*p & 0xC0) != 0x80) { cp = '?'; break; }
        cp = (cp << 6) | (*p++ & 63);
    }
    *cursor = p;
    return cp;
}
static GlyphCache *get_glyph(int index, unsigned int cp) {
    for (GlyphCache *g = glyphs; g; g = g->next)
        if (g->font == index && g->codepoint == cp) return g;
    CbmfGlyphView view;
    if (cbmf_get_glyph(&fonts[index], cp, &view) != CBMF_OK) return NULL;
    GlyphCache *g = calloc(1, sizeof(*g));
    if (!g) return NULL;
    g->font = index; g->codepoint = cp; g->glyph = view;
    if (view.width && view.height) {
        SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, view.width,
            view.height, 32, SDL_PIXELFORMAT_RGBA32);
        if (surface) {
            SDL_LockSurface(surface);
            for (int y = 0; y < view.height; ++y) {
                Uint32 *row = (Uint32 *)((Uint8 *)surface->pixels + y * surface->pitch);
                for (int x = 0; x < view.width; ++x) {
                    Uint8 packed = view.bitmap[y * view.row_bytes + (x >> 1)];
                    Uint8 coverage = (x & 1) ? packed >> 4 : packed & 15;
                    /* CBMF stores palette indices: 0 transparent, 1 opaque white.
                     * Index 1 is not 1/15 opacity. Match the original PSP CLUT. */
                    row[x] = SDL_MapRGBA(surface->format, 255, 255, 255, coverage ? 255 : 0);
                }
            }
            SDL_UnlockSurface(surface);
            g->texture = SDL_CreateTextureFromSurface(renderer, surface);
            if (g->texture) SDL_SetTextureBlendMode(g->texture, SDL_BLENDMODE_BLEND);
            SDL_FreeSurface(surface);
        }
    }
    g->next = glyphs; glyphs = g;
    return g;
}
void graphics_init(void) {
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    window = SDL_CreateWindow("Bounce NX", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720, SDL_WINDOW_SHOWN);
    if (!window) { nx_log("Window: %s", SDL_GetError()); return; }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) { nx_log("Renderer: %s", SDL_GetError()); return; }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    graphics_nx_set_game_view(0);
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) { nx_log("PNG: %s", IMG_GetError()); return; }
    for (int i = 0; i < FONT_COUNT; ++i) {
        FILE *f = util_open_file(font_paths[i], "rb");
        if (!f) { nx_log("Missing font: %s", font_paths[i]); return; }
        fseek(f, 0, SEEK_END); long size = ftell(f); rewind(f);
        if (size > 0) font_data[i] = malloc((size_t)size);
        int ok = font_data[i] && fread(font_data[i], 1, (size_t)size, f) == (size_t)size;
        fclose(f);
        if (!ok || cbmf_mount(&fonts[i], font_data[i], (size_t)size) != CBMF_OK) {
            nx_log("Invalid font: %s", font_paths[i]); return;
        }
    }
    ready = 1;
}
void graphics_shutdown(void) {
    while (glyphs) {
        GlyphCache *next = glyphs->next;
        if (glyphs->texture) SDL_DestroyTexture(glyphs->texture);
        free(glyphs); glyphs = next;
    }
    for (int i = 0; i < FONT_COUNT; ++i) { free(font_data[i]); font_data[i] = NULL; }
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    IMG_Quit();
    renderer = NULL; window = NULL; ready = 0;
}
void graphics_start_frame(void) { SDL_RenderSetClipRect(renderer, NULL); }
void graphics_end_frame(void) { SDL_RenderPresent(renderer); }
void graphics_set_scissor_fullscreen(void) { SDL_RenderSetClipRect(renderer, NULL); }
void graphics_clear(u32 color) { set_color(color); SDL_RenderClear(renderer); }
void graphics_draw_rect(int x, int y, int w, int h, u32 color) {
    SDL_Rect rect = {x, y, w, h}; set_color(color); SDL_RenderFillRect(renderer, &rect);
}
void graphics_draw_text(int x, int y, const char *text, u32 color, int height) {
    if (!text) return;
    int index = font_index(height), pen_x = x;
    const unsigned char *cursor = (const unsigned char *)text;
    while (*cursor) {
        unsigned int cp = next_codepoint(&cursor);
        if (cp == '\n') { pen_x = x; y += fonts[index].line_height; continue; }
        GlyphCache *g = get_glyph(index, cp);
        if (!g) continue;
        if (g->texture) {
            SDL_Rect dest = {pen_x + g->glyph.offset_x, y + g->glyph.offset_y,
                g->glyph.width, g->glyph.height};
            SDL_SetTextureColorMod(g->texture, color & 255, (color >> 8) & 255, (color >> 16) & 255);
            SDL_SetTextureAlphaMod(g->texture, color >> 24);
            SDL_RenderCopy(renderer, g->texture, NULL, &dest);
        }
        pen_x += g->glyph.advance_x;
    }
}
int graphics_measure_text(const char *text, int height) {
    int32_t width = 0;
    if (text) cbmf_text_width_utf8(&fonts[font_index(height)], text, &width);
    return (int)width;
}
void graphics_draw_number(int x, int y, int number, u32 color) {
    char text[32]; snprintf(text, sizeof(text), "%d", number);
    graphics_draw_text(x, y, text, color, 24);
}
int graphics_measure_number(int number) {
    char text[32]; snprintf(text, sizeof(text), "%d", number); return graphics_measure_text(text, 24);
}
void graphics_set_texturing(int enabled) { texturing = enabled != 0; }
void graphics_begin_plain(void) { texturing = 0; }
void graphics_begin_textured(void) { texturing = 1; }
int graphics_get_texturing_state(void) { return texturing; }
void graphics_bind_texture(texture_t *texture) { bound_texture = texture; }
void graphics_batch_sprite(int u1, int v1, int u2, int v2, int x, int y, int w, int h) {
    sprite_rect_t src = {u1, v1, u2-u1, v2-v1};
    png_draw_sprite(bound_texture, &src, x, y, w, h);
}
void graphics_batch_sprite_colored(int u1, int v1, int u2, int v2, int x, int y, int w, int h, u32 color) {
    if (!bound_texture || !bound_texture->data) return;
    SDL_Texture *t = bound_texture->data;
    SDL_SetTextureColorMod(t, color & 255, (color >> 8) & 255, (color >> 16) & 255);
    SDL_SetTextureAlphaMod(t, color >> 24);
    graphics_batch_sprite(u1, v1, u2, v2, x, y, w, h);
    SDL_SetTextureColorMod(t, 255, 255, 255); SDL_SetTextureAlphaMod(t, 255);
}
void graphics_flush_batch(void) { /* SDL preserves draw order and batches internally. */ }
