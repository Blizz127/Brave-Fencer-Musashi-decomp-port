/* SPDX-License-Identifier: MIT */
#include "forest_message_art.h"
#include "dialogue_contour.h"

#include <stdlib.h>

enum { CORNERS = BFM_DIALOGUE_CORNERS, GLYPHS = 95, FONT_HEIGHT = 12 };
enum { TAIL_OFFSET = 0x2030, FONT_OFFSET = 0x24cbc, CONTOUR_OFFSET = 0x257fc,
       INSET_OFFSET = 0x5f094 };

struct BfmForestMessageArt {
    BfmDialoguePoint contour[2][CORNERS], inset[2][CORNERS];
    uint8_t glyph[GLYPHS][FONT_HEIGHT];
    uint32_t portrait[40][32], tail[16][16], ink[16];
    uint32_t paper, highlight, outline;
};

static unsigned le16(const uint8_t *p)
{
    return p[0] | (unsigned)p[1] << 8;
}

static uint32_t le32(const uint8_t *p)
{
    return le16(p) | (uint32_t)le16(p + 2) << 16;
}

static int signed16(const uint8_t *p)
{
    unsigned n = le16(p);
    return n < 32768 ? (int)n : (int)n - 65536;
}

static int signed8(uint8_t n)
{
    return n < 128 ? n : (int)n - 256;
}

static uint32_t rgb(uint16_t c)
{
    return UINT32_C(0xff000000) | ((c & 31u) * 255 / 31) << 16 |
           (((c >> 5) & 31u) * 255 / 31) << 8 | ((c >> 10) & 31u) * 255 / 31;
}

static int read_art(BfmForestMessageArt *art, const MusashiDiscMedia *disc,
                    const uint8_t *scene, const uint16_t tiles[512][1024])
{
    uint8_t entry[8], contours[2][CORNERS][4], tail[15 * 64 + 8];
    size_t length;
    if (!musashi_disc_media_read_root_range(disc, "MAIN.CD", 8 + 8 * 10, entry,
                                           sizeof(entry), &length))
        return 0;
    uint32_t sector = le32(entry), extent = le32(entry + 4);
    if (!sector || sector > length / 2048)
        return 0;
    size_t base = (size_t)sector * 2048;
    if (extent > length - base || extent < CONTOUR_OFFSET + sizeof(contours) ||
        !musashi_disc_media_read_root_range(disc, "MAIN.CD", base + CONTOUR_OFFSET,
                                            &contours[0][0][0], sizeof(contours), &length) ||
        !musashi_disc_media_read_root_range(disc, "MAIN.CD", base + FONT_OFFSET,
                                            &art->glyph[0][0], sizeof(art->glyph), &length) ||
        !musashi_disc_media_read_root_range(disc, "MAIN.CD", base + TAIL_OFFSET,
                                            tail, sizeof(tail), &length))
        return 0;
    for (unsigned style = 0; style < 2; ++style)
        for (unsigned i = 0; i < CORNERS; ++i) {
            const uint8_t *p = contours[style][i];
            const uint8_t *q = scene + INSET_OFFSET + (style * CORNERS + i) * 2;
            art->contour[style][i] = (BfmDialoguePoint){signed16(p), signed16(p + 2)};
            art->inset[style][i] = (BfmDialoguePoint){signed8(q[0]), signed8(q[1])};
        }
    const uint16_t *bubble = &tiles[447][352];
    art->paper = rgb(bubble[0]);
    art->highlight = rgb(bubble[1]);
    art->outline = rgb(bubble[2]);
    for (unsigned i = 0; i < 16; ++i)
        art->ink[i] = rgb(tiles[446][352+i]);
    for (unsigned y = 0; y < 16; ++y)
        for (unsigned x = 0; x < 16; ++x) {
            unsigned index = (tail[y * 64 + x/2] >> (4 * (x & 1))) & 15;
            art->tail[y][x] = bubble[index] ? rgb(bubble[index]) : 0;
        }
    for (unsigned y = 0; y < 40; ++y)
        for (unsigned x = 0; x < 32; ++x) {
            unsigned pack = tiles[416+y][624+x/4];
            uint16_t color = tiles[296][352+((pack >> (4*(x & 3))) & 15)];
            art->portrait[y][x] = color ? rgb(color) : 0;
        }
    return 1;
}

BfmForestMessageArt *bfm_forest_message_art_load(const MusashiDiscMedia *disc,
                                                const uint8_t *scene, size_t bytes,
                                                const BfmForestTerrain *terrain)
{
    const uint16_t (*tiles)[1024] = bfm_forest_terrain_tiles(terrain);
    if (!disc || !scene || !tiles || bytes < INSET_OFFSET + 4 * CORNERS)
        return NULL;
    BfmForestMessageArt *art = malloc(sizeof(*art));
    if (art && !read_art(art, disc, scene, tiles)) {
        free(art);
        return NULL;
    }
    return art;
}

void bfm_forest_message_art_free(BfmForestMessageArt *art)
{
    free(art);
}

/* Both interpolation and scaling round toward the lesser pixel coordinate. */
static int floor_div(int value, int divisor)
{
    return value / divisor - (value % divisor < 0);
}

static int coordinate(int round, int pointed, int mix, int scale)
{
    return floor_div((round + floor_div((pointed - round) * mix, 4)) * scale, 4096);
}

void bfm_forest_message_art_draw(const BfmForestMessageArt *art,
                                 const BfmForestMessageFrame *frame,
                                 uint32_t pixels[240][320])
{
    if (!art || !frame || !pixels || !frame->border_step || frame->border_step > 8 ||
        frame->jagged_mix > 4)
        return;
    int step = (int)frame->border_step, mix = (int)frame->jagged_mix;
    BfmDialoguePoint border[CORNERS], inset[CORNERS];
    for (unsigned i = 0; i < CORNERS; ++i) {
        BfmDialoguePoint a = art->contour[0][i], b = art->contour[1][i];
        border[i].x = 160 + coordinate(a.x, b.x, mix, step * 904);
        border[i].y = 44 + coordinate(a.y, b.y, mix, step * 4096 / 6);
        BfmDialoguePoint delta = art->inset[mix > 2][i];
        inset[i] = (BfmDialoguePoint){border[i].x + delta.x, border[i].y + delta.y};
    }
    bfm_dialogue_contour(border, inset, art->paper, art->highlight, art->outline, 10, pixels);
    if (!mix && step == 8)
        for (unsigned y = 0; y < 11; ++y)
            for (unsigned x = 0; x < 15; ++x) {
                uint32_t color = art->tail[(y*15+5)/11][x];
                if (color) pixels[76+y][160+x] = color;
            }
    if (!frame->text_visible)
        return;
    for (unsigned y = 0; y < 40; ++y)
        for (unsigned x = 0; x < 32; ++x)
            if (art->portrait[y][x]) pixels[24+y][32+x] = art->portrait[y][x];
    for (unsigned row = 0; row < 3; ++row)
        for (unsigned col = 0; col < 36; ++col) {
            BfmForestMessageCell cell = frame->cells[row][col];
            if (cell.glyph < 32 || cell.glyph >= 32 + GLYPHS || !cell.ink || cell.ink >= 16)
                continue;
            const uint8_t *glyph = art->glyph[cell.glyph-32];
            for (unsigned y = 0; y < FONT_HEIGHT; ++y)
                for (unsigned x = 0; x < 6; ++x)
                    if (glyph[y] & (128u >> x))
                        pixels[24 + 14*row + y][72 + 6*col + x] = art->ink[cell.ink];
        }
}
