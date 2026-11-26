/* SPDX-License-Identifier: MIT */
#include "save_menu.h"
#include "scene_tiles.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { GLYPHS, LABELS, PAGE_WORDS = 64 };
struct BfmSaveArt {
    uint16_t pages[2][256][PAGE_WORDS];
    uint16_t palette[16];
};

BfmSaveArt *bfm_save_art_load(const MusashiDiscMedia *disc)
{
    uint16_t (*words)[1024] = calloc(512, sizeof *words);
    BfmSaveArt *art = NULL;
    if (words && bfm_scene_read_tiles(disc, "MAIN.CD", 12, words)) {
        art = malloc(sizeof *art);
        if (art) {
            memcpy(art->palette, &words[257][352], sizeof art->palette);
            for (unsigned y = 0; y < 256; ++y) {
                memcpy(art->pages[GLYPHS][y], &words[256 + y][320], sizeof art->pages[0][0]);
                memcpy(art->pages[LABELS][y], &words[y][960], sizeof art->pages[0][0]);
            }
        }
    }
    free(words);
    return art;
}

typedef struct {
    const BfmSaveArt *art;
    uint32_t (*pixels)[320];
} Canvas;

static uint32_t rgb(unsigned r, unsigned g, unsigned b)
{
    return 0xff000000u | (r * 255u / 31u << 16) | (g * 255u / 31u << 8) | (b * 255u / 31u);
}

/* The two mounted four-bit pages use the same 16-color CLUT. */
static void blit(Canvas *c, int page, int x, int y, int w, int h,
                 int u, int v, int uw, int vh, unsigned light)
{
    for (int j = 0; j < h; ++j) {
        const uint16_t *src = c->art->pages[page][v + j * vh / h];
        for (int i = 0; i < w; ++i) {
            unsigned tx = (unsigned)(u + i * uw / w);
            unsigned index = (src[tx / 4] >> ((tx % 4) * 4)) & 15u;
            unsigned texel = c->art->palette[index];
            if (texel && (unsigned)(x + i) < 320 && (unsigned)(y + j) < 240)
                c->pixels[y + j][x + i] = rgb((texel & 31u) * light / 128,
                                               ((texel >> 5) & 31u) * light / 128,
                                               ((texel >> 10) & 31u) * light / 128);
        }
    }
}

static void label(Canvas *c, int x, int y, int u, int v, int w, int h, unsigned light)
{
    blit(c, LABELS, x, y, w, h, u, v, w, h, light);
}

static void text(Canvas *c, int x, int y, const char *s, unsigned light)
{
    for (; *s; ++s, x += 7) {
        unsigned ch = (unsigned char)*s, tile;
        if (ch >= 'a' && ch <= 'z') tile = ch - 'a' + 1u + (ch >= 'o');
        else if (ch >= 'A' && ch <= 'Z') tile = ch - 'A' + 161u + (ch >= 'O');
        else if (ch >= '0' && ch <= '9') tile = ch - '0' + 129;
        else {
            switch (ch) {
            case ':': tile = 122; break;
            case '/': tile = 123; break;
            case '-': tile = 143; break;
            case '.': tile = 188; break;
            case '$': tile = 128; break;
            case ';': tile = 140; break; /* compact colon on the HP/BP lines */
            default: continue;
            }
        }
        blit(c, GLYPHS, x, y, 8, 8, (int)(tile % 16) * 8, (int)(tile / 16) * 8, 8, 8, light);
    }
}

static unsigned quantize(int value, int adjustment, int dim)
{
    int level = ((value >> dim) + adjustment) / 8;
    if (level < 0) level = 0;
    if (level > 31) level = 31;
    return (unsigned)level;
}

static void shade(Canvas *c, int x, int y, int w, int h, int stripe, int dim)
{
    static const int dither[4][4] = {
        {-4, 0,-3, 1}, {2,-2, 3,-1}, {-3, 1,-4, 0}, {3,-1, 2,-2}
    };
    for (int j = 0; j < h; ++j) {
        for (int i = 0; i < w; ++i) {
            int d = dither[(y+j) % 4][(x+i) % 4];
            int numerator = (stripe ? 40 : 120) * j * w - 40 * i * h;
            int blue = ((stripe ? 208 : 112) * w * h + numerator + w * h / 2) / (w * h);
            c->pixels[y+j][x+i] = rgb(quantize(stripe ? 8 : 0, d, dim),
                                       quantize(stripe ? 16 : 8, d, dim), quantize(blue, d, dim));
        }
    }
}

static void panel(Canvas *c, int x, int y, int w, int h, int bright)
{
    unsigned light = bright ? 128u : 88u;
    shade(c, x+4, y+6, w-8, h-9, 0, !bright);
    if (bright) shade(c, x+4, y+4, w-8, h == 38 ? 29 : 27, 1, 0);
    for (int side = 0; side <= 1; ++side) {
        int dx = side * (w-8), dy = side * (h-8);
        blit(c, GLYPHS, x+dx, y+8, 8, h-16, 32, 72, 8, 7, light);
        blit(c, GLYPHS, x+8, y+dy, w-16, 8, 40, 72, 7, 8, light);
        blit(c, GLYPHS, x+dx, y, 8, 8, side * 8, 72, 8, 8, light);
        blit(c, GLYPHS, x+dx, y+h-8, 8, 8, 16 + side * 8, 72, 8, 8, light);
    }
}

static void draw_row(Canvas *c, const BfmSaveMenu *menu, unsigned i)
{
    int x = 8 + 151 * (int)(i % 2), y = 48 + 90 * (int)(i / 2);
    unsigned light = i == menu->selected ? 128 : 88;
    const BfmSaveRow *row = &menu->rows[i];
    char line[64];
    panel(c, x, y, 153, 92, i == menu->selected);
    label(c, x+19, y+8, 36*(int)(i%2), 12*(int)(i/2), 35, 12, light);
    unsigned badge = row->state == BFM_SAVE_EMPTY ? 1 : row->chapter;
    if (badge) label(c, x+96, y+8, 72+18*(int)(badge-1), 0, 17, 12, light);
    if (row->state == BFM_SAVE_DAMAGED) {
        text(c, x+24, y+56, "Unreadable", light);
        return;
    }
    snprintf(line, sizeof line, "Playtime %02u:%02u", row->hours, row->minutes);
    text(c, x+24, y+22, line, light);
    if (row->state == BFM_SAVE_EMPTY) {
        text(c, x+45, y+56, "No Data", light);
        return;
    }
    static const char *const weekdays[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    snprintf(line, sizeof line, "Day:%3u %s %02u:%02u", row->day, weekdays[(row->day-1)%7],
             row->hour, row->minute);
    text(c, x+17, y+36, line, light);
    snprintf(line, sizeof line, "HP;%3u/%u", row->hp, row->max_hp);
    text(c, x+17, y+46, line, light);
    snprintf(line, sizeof line, "BP;%3u/%u", row->bp, row->max_bp);
    text(c, x+17, y+56, line, light);
    snprintf(line, sizeof line, "$%6llu", (unsigned long long)row->money);
    text(c, x+17, y+66, line, light);
    snprintf(line, sizeof line, "B.Field%3u/35", row->rescued);
    text(c, x+17, y+76, line, light);
}

void bfm_save_menu_draw(const BfmSaveArt *art, const BfmSaveMenu *menu, unsigned fields,
                        uint32_t pixels[240][320])
{
    Canvas c = {art, pixels};
    for (int y = 0; y < 240; ++y)
        for (int x = 0; x < 320; ++x) pixels[y][x] = 0xff000000u;
    panel(&c, 8, 12, 304, 38, 1);
    if (menu->issue) {
        text(&c, 22, 18, menu->issue, 128);
        text(&c, 22, 31, "Press T or Triangle to return.", 128);
        return;
    }
    for (unsigned i = 0; i < 4; ++i) draw_row(&c, menu, i);
    if (menu->page == BFM_SAVE_BOUNDARY) {
        text(&c, 22, 18, "This preview cannot resume this save.", 128);
        text(&c, 22, 31, "Press T or Triangle to return to files.", 128);
    } else if (menu->page == BFM_SAVE_CONFIRM || menu->page == BFM_SAVE_LOADED) {
        label(&c, 106, 18, 0, 180, 101, 12, 128);
        label(&c, 118, 31, 114, 72, 83, 12, 128);
    } else {
        label(&c, 58, 18, 0, 156, 203, 12, 128);
        label(&c, 46, 31, 0, 168, 221, 12, 128);
    }
    if (menu->page == BFM_SAVE_LOADED) {
        panel(&c, 32, 118, 256, 38, 1);
        label(&c, 122, 131, 76, 216, 74, 12, 128);
        return;
    }
    static const int motion[] = {0, 0, 2, 4, 4, 3, 2, 1};
    int x = 8 + 151 * (int)(menu->selected % 2);
    int y = 54 + 90 * (int)(menu->selected / 2);
    blit(&c, GLYPHS, x + motion[(fields / 2) % 8], y, 16, 16, 112, 16, 16, 16, 128);
}
