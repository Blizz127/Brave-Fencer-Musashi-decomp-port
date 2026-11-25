/* SPDX-License-Identifier: MIT */
#include "palace_dialogue.h"
#include <stdlib.h>
#include <string.h>

enum { CORNERS = 30, CHAR_ROWS = 12, FIRST_CHAR = 32, CHAR_COUNT = 95 };
enum { PORTRAIT_W = 32, PORTRAIT_H = 40, LINE_CAPACITY = 36 };
typedef struct { int x, y; } Point;

struct PalaceDialogue {
    uint8_t glyph[CHAR_COUNT][CHAR_ROWS];
    Point contour[CORNERS], inset[CORNERS];
    uint16_t portrait[PORTRAIT_H][PORTRAIT_W], tail[16][16];
    uint16_t ink, outline, highlight, paper;
    uint8_t name[LINE_CAPACITY], line[LINE_CAPACITY];
    size_t name_size, line_size;
};

static unsigned little16(const uint8_t *p)
{
    return p[0] | (unsigned)p[1] << 8;
}

static unsigned little32(const uint8_t *p)
{
    return little16(p) | little16(p + 2) << 16;
}

static int signed16(const uint8_t *p)
{
    unsigned n = little16(p);
    return n < 32768 ? (int)n : (int)n - 65536;
}

static int signed8(uint8_t n)
{
    return n < 128 ? n : (int)n - 256;
}

static int line_bytes(uint8_t *out, size_t *count, const uint8_t *scene,
                      size_t limit, size_t *position, uint8_t terminator)
{
    size_t size = 0;
    while (*position < limit && scene[*position] != terminator) {
        unsigned c = scene[(*position)++];
        if (size >= LINE_CAPACITY || c < FIRST_CHAR || c >= FIRST_CHAR + CHAR_COUNT)
            return 0;
        out[size++] = (uint8_t)c;
    }
    if (!size || *position == limit)
        return 0;
    ++*position;
    *count = size;
    return 1;
}

/* This scene's first speech instruction marks its speaker string, changes
 * style after the newline and supplies the voice cue before the first line. */
static int read_script(PalaceDialogue *d, const uint8_t *scene, size_t size)
{
    size_t p = 0x60e04;
    if (p >= size || scene[p++] != 0x18 ||
        !line_bytes(d->name, &d->name_size, scene, size, &p, '\n') ||
        size - p < 4 || scene[p] != 0x19 || scene[p + 1] != 0x14)
        return 0;
    p += 4;
    return line_bytes(d->line, &d->line_size, scene, size, &p, 0x15);
}

static int read_message_art(PalaceDialogue *d, const MusashiDiscMedia *disc,
                            const uint8_t *scene, const uint16_t vram[512][1024])
{
    enum { FONT = 0x24cbc, SHAPE = 0x257fc, TAIL = 0x2030, INSET = 0x5bdd0 };
    uint8_t entry[8], shape[4 * CORNERS], tail[15 * 64 + 8];
    size_t length;
    if (!musashi_disc_media_read_root_range(disc, "MAIN.CD", 8 + 8 * 10,
                                            entry, sizeof(entry), &length))
        return 0;
    unsigned sector = little32(entry), extent = little32(entry + 4);
    if (!sector || sector > length / 2048)
        return 0;
    size_t base = (size_t)sector * 2048;
    if (extent > length - base || extent < SHAPE + sizeof(shape) ||
        !musashi_disc_media_read_root_range(disc, "MAIN.CD", base + FONT,
                                            &d->glyph[0][0], sizeof(d->glyph), &length) ||
        !musashi_disc_media_read_root_range(disc, "MAIN.CD", base + SHAPE,
                                            shape, sizeof(shape), &length) ||
        !musashi_disc_media_read_root_range(disc, "MAIN.CD", base + TAIL,
                                            tail, sizeof(tail), &length))
        return 0;
    for (unsigned i = 0; i < CORNERS; ++i) {
        d->contour[i] = (Point){signed16(&shape[4*i]), signed16(&shape[4*i+2])};
        d->inset[i] = (Point){signed8(scene[INSET + 2*i]), signed8(scene[INSET + 2*i+1])};
    }
    const uint16_t *bubble_colors = &vram[447][352];
    d->paper = bubble_colors[0]; d->highlight = bubble_colors[1];
    d->outline = bubble_colors[2]; d->ink = vram[446][353];
    for (unsigned y = 0; y < 16; ++y)
        for (unsigned x = 0; x < 16; ++x)
            d->tail[y][x] = bubble_colors[(tail[y * 64 + x/2] >> (4 * (x & 1))) & 15];
    for (unsigned y = 0; y < PORTRAIT_H; ++y)
        for (unsigned x = 0; x < PORTRAIT_W; ++x) {
            uint16_t pack = vram[400+y][456+x/4];
            d->portrait[y][x] = vram[457][352+((pack >> (4*(x & 3))) & 15)];
        }
    return 1;
}

PalaceDialogue *bfm_palace_dialogue_load(const MusashiDiscMedia *disc,
                                         const uint8_t *scene, size_t size,
                                         const uint16_t vram[512][1024])
{
    if (!disc || !scene || !vram || size <= 0x60e04)
        return NULL;
    PalaceDialogue *d = calloc(1, sizeof(*d));
    if (!d || !read_script(d, scene, size) || !read_message_art(d, disc, scene, vram)) {
        free(d);
        return NULL;
    }
    return d;
}

void bfm_palace_dialogue_free(PalaceDialogue *dialogue)
{
    free(dialogue);
}

static uint32_t rgb(uint16_t c)
{
    return UINT32_C(0xff000000) | ((c & 31u) * 255 / 31) << 16 |
           (((c >> 5) & 31u) * 255 / 31) << 8 | ((c >> 10) & 31u) * 255 / 31;
}

static int scaled(int v, int scale)
{
    int n = v * scale;
    return n < 0 ? -((-n + 4095) / 4096) : n / 4096;
}

static void put(uint32_t pixels[240][320], int x, int y, uint32_t color)
{
    if ((unsigned)x < 320 && (unsigned)y < 240)
        pixels[y][x] = color;
}

static void edge_line(uint32_t pixels[240][320], Point a, Point b, uint32_t color)
{
    if (a.x > b.x) { Point tmp = a; a = b; b = tmp; }
    int dx = abs(b.x - a.x), dy = -abs(b.y - a.y);
    int sx = a.x < b.x ? 1 : -1, sy = a.y < b.y ? 1 : -1;
    int err = dx + dy;
    if (dx > -dy) err -= (dx - dy - 1) / 2 - (dx - 1) / 2;
    for (;;) {
        put(pixels, a.x, a.y, color);
        if (a.x == b.x && a.y == b.y) break;
        int twice = 2 * err;
        if (twice > dy) { err += dy; a.x += sx; }
        if (twice < dx) { err += dx; a.y += sy; }
    }
}

static void draw_balloon(const PalaceDialogue *d, unsigned step, uint32_t pixels[240][320])
{
    Point border[CORNERS], inset[CORNERS];
    int sx = (int)step * 904, sy = (int)step * 4096 / 6;
    int low = 240, high = 0;
    for (unsigned i = 0; i < CORNERS; ++i) {
        Point p = {160 + scaled(d->contour[i].x, sx), 44 + scaled(d->contour[i].y, sy)};
        border[i] = p;
        inset[i] = (Point){p.x + d->inset[i].x, p.y + d->inset[i].y};
        if (low > p.y) low = p.y;
        if (high < p.y) high = p.y;
    }
    if (low < 0) low = 0;
    if (high > 240) high = 240;
    uint32_t paper = rgb(d->paper);
    for (int y = low; y < high; ++y) {
        int crossings[CORNERS], count = 0;
        for (unsigned i = 0; i < CORNERS; ++i) {
            Point a = border[i], b = border[(i + 1) % CORNERS];
            if ((y < a.y) == (y < b.y)) continue;
            int x = a.x + (y - a.y) * (b.x - a.x) / (b.y - a.y);
            int j = count++;
            while (j && crossings[j-1] > x) {
                crossings[j] = crossings[j-1]; --j;
            }
            crossings[j] = x;
        }
        for (int i = 1; i < count; i += 2)
            for (int x = crossings[i-1]; x < crossings[i]; ++x)
                put(pixels, x, y, paper);
    }
    for (unsigned i = 0; i < CORNERS; ++i)
        edge_line(pixels, inset[i], inset[(i + 1) % CORNERS], rgb(d->highlight));
    for (unsigned i = 0; i < CORNERS; ++i)
        edge_line(pixels, border[i], border[(i + 1) % CORNERS], rgb(d->outline));
}

static void draw_words(const PalaceDialogue *d, int y, const uint8_t *text, size_t count,
                       uint32_t pixels[240][320])
{
    for (size_t i = 0; i < count; ++i) {
        const uint8_t *glyph = d->glyph[text[i] - FIRST_CHAR];
        for (unsigned line = 0; line < CHAR_ROWS; ++line)
            for (unsigned bit = 0; bit < 6; ++bit)
                if (glyph[line] & (128u >> bit))
                    pixels[y+line][72+6*i+bit] = rgb(d->ink);
    }
}

void bfm_palace_dialogue_draw(const PalaceDialogue *d, unsigned fields,
                              uint32_t pixels[240][320])
{
    /* The textbox opens in seven two-field steps. Name and body are
     * separate updates; the first voice cue precedes the body by 16 fields. */
    if (!d || !pixels || fields < 1790)
        return;
    unsigned elapsed = fields - 1790;
    unsigned step = elapsed < 12 ? 2 + elapsed / 2 : 8;
    draw_balloon(d, step, pixels);
    if (step == 8)
        for (unsigned y = 0; y < 11; ++y)
            for (unsigned x = 0; x < 15; ++x) {
                uint16_t color = d->tail[(y*15+5)/11][x];
                if (color) pixels[76+y][160+x] = rgb(color);
            }
    if (elapsed < 16) return;
    for (unsigned y = 0; y < PORTRAIT_H; ++y)
        for (unsigned x = 0; x < PORTRAIT_W; ++x)
            if (d->portrait[y][x]) pixels[24+y][32+x] = rgb(d->portrait[y][x]);
    draw_words(d, 24, d->name, d->name_size, pixels);
    if (elapsed >= 32) {
        size_t count = 1 + (elapsed - 32) / 2;
        if (count > d->line_size) count = d->line_size;
        draw_words(d, 38, d->line, count, pixels);
    }
}
