/* SPDX-License-Identifier: MIT */
#include "woods_body_notice.h"
#include "../pc_port/forest_player_bytes.h"

#include <string.h>

static const uint32_t scene_base = UINT32_C(0x80128158);

static int load_label(BfmWoodsRankLabel *label, unsigned slot, const uint8_t *scene,
                      size_t scene_bytes, const uint8_t *layout, const uint8_t *alphabet,
                      const uint8_t *marks)
{
    uint32_t address = u32le(layout + 4*slot);
    unsigned insertion = layout[24 + slot]; /* Byte offset of the rank digit in this label. */
    if (!insertion || insertion % 2 || insertion > sizeof label->prefix ||
        address < scene_base || address - scene_base > scene_bytes ||
        insertion > scene_bytes - (address - scene_base)) return 0;
    const uint8_t *text = scene + (address - scene_base);
    for (unsigned i = 0; i < insertion; i += 2) {
        unsigned group = text[i], code = text[i + 1];
        uint16_t uv;
        if (group == 0x82 && code >= 0x4f && code < 0x9f)
            uv = u16le(alphabet + 2*(code-0x4f));
        else if (group == 0x81 && code >= 0x40 && code < 0x5c)
            uv = u16le(marks + 2*(code-0x40));
        else return 0;
        if (uv & 0x8000) return 0; /* These labels use single atlas cells. */
        label->prefix[label->count++] = uv;
    }
    label->palette = layout[16 + slot];
    label->height = s16le(layout + 28 + 2*slot);
    return 1;
}

int bfm_woods_body_notice_load(BfmWoodsBodyNotice *notice, const MusashiDiscMedia *disc,
                              const uint8_t *scene, size_t scene_bytes)
{
    const size_t table = UINT32_C(0x8018abac) - scene_base;
    enum { ALPHABET = 2048 + 0x636d8 - 0x10000, MARKS = 2048 + 0x63818 - 0x10000 };
    uint8_t alphabet[80*2], marks[28*2];
    size_t extent;
    if (!notice || !disc || !scene || scene_bytes < table + 48 ||
        !musashi_disc_media_read_root_range(disc, "SLUS_007.26", ALPHABET,
                                            alphabet, sizeof alphabet, &extent) ||
        !musashi_disc_media_read_root_range(disc, "SLUS_007.26", MARKS,
                                            marks, sizeof marks, &extent)) return 0;

    BfmWoodsBodyNotice decoded = {0};
    const uint8_t *layout = scene + table;
    if (!load_label(&decoded.body, 0, scene, scene_bytes, layout, alphabet, marks) ||
        !load_label(&decoded.fusion, 2, scene, scene_bytes, layout, alphabet, marks) ||
        !load_label(&decoded.lumina, 3, scene, scene_bytes, layout, alphabet, marks)) return 0;
    for (unsigned i = 0; i < 10; ++i) {
        decoded.digits[i] = u16le(alphabet + 2*i);
        if (decoded.digits[i] & 0x8000) return 0;
    }
    memcpy(decoded.pulse, layout + 20, sizeof decoded.pulse);
    for (unsigned i = 0; i < 6; ++i) decoded.slide[i] = s16le(layout + 36 + 2*i);
    *notice = decoded;
    return 1;
}

enum { SLIDE_FIELDS = 6, HOLD_FIELDS = 33, LAST_FIELD = 1 + 2*SLIDE_FIELDS + HOLD_FIELDS };

static void start_label(BfmWoodsRankLabel *label, unsigned delta)
{
    if (label->count && delta && delta < 10) {
        label->frame = 1;
        label->delta = (uint8_t)delta;
    }
}

void bfm_woods_body_notice_start(BfmWoodsBodyNotice *notice, unsigned delta)
{
    if (notice) start_label(&notice->body, delta);
}

void bfm_woods_weapon_notice_start(BfmWoodsBodyNotice *notice, unsigned lumina_delta,
                                   unsigned fusion_delta)
{
    if (notice) {
        start_label(&notice->lumina, lumina_delta);
        start_label(&notice->fusion, fusion_delta);
    }
}

static void step_label(BfmWoodsRankLabel *label)
{
    if (label->frame)
        label->frame = label->frame < LAST_FIELD ? (uint8_t)(label->frame + 1) : 0;
}

void bfm_woods_body_notice_step(BfmWoodsBodyNotice *notice)
{
    if (notice) {
        step_label(&notice->body);
        step_label(&notice->fusion);
        step_label(&notice->lumina);
    }
}

static unsigned shade(unsigned component, unsigned light)
{
    unsigned value = component * light / 128;
    return (value < 31 ? value : 31) * 255 / 31;
}

/* Both the heading and the label are indexed four-bit HUD sprites. */
static void draw_strip(const uint16_t tiles[512][1024], uint32_t pixels[240][320],
                       const uint32_t colors[16], int x, int y, unsigned u, unsigned v, int width)
{
    for (int row = 0; row < 8; ++row) {
        if (y + row < 10 || y + row >= 230) continue;
        for (int col = 0; col < width; ++col) {
            if ((unsigned)(x + col) >= 320) continue;
            unsigned texel = u + (unsigned)col;
            unsigned packed = tiles[v + (unsigned)row][texel / 4];
            uint32_t color = colors[(packed >> (4*(texel % 4))) & 15];
            if (color) pixels[y+row][x+col] = color;
        }
    }
}

static void draw_label(const BfmWoodsBodyNotice *notice, const BfmWoodsRankLabel *label,
                       const uint16_t tiles[512][1024], uint32_t pixels[240][320])
{
    if (!label->frame) return;
    unsigned elapsed = label->frame - 1;
    unsigned distance = elapsed <= SLIDE_FIELDS ? elapsed : SLIDE_FIELDS;
    unsigned light = 128;
    if (elapsed > SLIDE_FIELDS + HOLD_FIELDS) {
        distance = LAST_FIELD - label->frame;
    } else if (elapsed > SLIDE_FIELDS) {
        light = notice->pulse[(SLIDE_FIELDS + HOLD_FIELDS - elapsed) % 4];
    }
    int center = 160 - 215;
    for (unsigned i = 0; i < distance; ++i) center += notice->slide[i];
    int y = 120 + label->height;

    uint32_t colors[16];
    const uint16_t *clut = tiles[256 + label->palette] + (label->palette < 224 ? 352 : 256);
    for (unsigned i = 0; i < 16; ++i) {
        unsigned c = clut[i];
        colors[i] = c ? UINT32_C(0xff000000) | shade(c & 31, light) << 16 |
                        shade((c >> 5) & 31, light) << 8 | shade((c >> 10) & 31, light) : 0;
    }
    draw_strip(tiles, pixels, colors, center + 16 - 20, y - 4, 0x640, 0x1c8, 40);
    for (unsigned i = 0; i <= label->count; ++i) {
        unsigned uv = i == label->count ? notice->digits[label->delta] : label->prefix[i];
        if (uv == 0x3872) { /* Narrow word separator. */
            center += 2;
        } else {
            draw_strip(tiles, pixels, colors, center - 4, y + 10 - 4,
                       0x500 + (uv & 255), 0x100 + (uv >> 8), 8);
            center += 8;
        }
    }
}

void bfm_woods_body_notice_draw(const BfmWoodsBodyNotice *notice,
                               const uint16_t tiles[512][1024], uint32_t pixels[240][320])
{
    if (!notice || !tiles || !pixels) return;
    draw_label(notice, &notice->body, tiles, pixels);
    draw_label(notice, &notice->fusion, tiles, pixels);
    draw_label(notice, &notice->lumina, tiles, pixels);
}
