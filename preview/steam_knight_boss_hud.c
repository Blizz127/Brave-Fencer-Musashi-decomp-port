/* SPDX-License-Identifier: MIT */
#include "steam_knight_boss_hud.h"
#include "forest_raster.h"
#include "../pc_port/forest_player_bytes.h"

void bfm_steam_knight_boss_title(const uint16_t tiles[512][1024],
                                 uint32_t pixels[240][320])
{
    if (!tiles || !pixels) return;
    const BfmForestRaster surface = {tiles, pixels, 10, 229};
    const BfmForestRasterMaterial title = {.page = 15, .clut = 0x6316, .count = 4, .textured = 1};
    /* The room stacks the two halves in the rightmost texture page. The
     * last row of each half is transparent; omit it at the UV page edge. */
    for (unsigned half = 0; half < 2; ++half) {
        int x = 80 + 80 * (int)half;
        unsigned v = 192 + 32 * half;
        const BfmForestRasterVertex corners[4] = {
            {x,      184, 160, v,      0x808080}, {x + 80, 184, 240, v,      0x808080},
            {x,      215, 160, v + 31, 0x808080}, {x + 80, 215, 240, v + 31, 0x808080}
        };
        bfm_field_raster_face(&surface, &title, corners);
    }
}

int bfm_steam_knight_boss_hud_load(BfmSteamKnightBossHud *hud, const MusashiDiscMedia *disc,
                                  const uint8_t *scene, size_t size)
{
    const uint32_t origin = UINT32_C(0x80128158);
    const size_t name_slot = UINT32_C(0x801942f0) - origin;
    const size_t alphabet_offset = 2048 + 0x636d8 - 0x10000;
    if (!hud || !disc || !scene || size < name_slot + 4) return 0;
    uint32_t address = u32le(scene + name_slot);
    if (address < origin || address - origin >= size) return 0;
    uint8_t alphabet[2 * 80];
    size_t file_size;
    if (!musashi_disc_media_read_root_range(disc, "SLUS_007.26", alphabet_offset,
                                             alphabet, sizeof alphabet, &file_size)) return 0;
    BfmSteamKnightBossHud parsed = {0};
    size_t off = address - origin;
    while (off < size && scene[off]) {
        if (size - off < 2 || parsed.count == sizeof parsed.name / sizeof parsed.name[0]) return 0;
        unsigned kind = scene[off], c = scene[off + 1];
        if (kind == 0x81 && c == 0x41) {
            parsed.name[parsed.count++] = UINT16_MAX;
        } else if (kind == 0x82 && c >= 0x4f && c < 0x9f) {
            uint16_t uv = u16le(alphabet + 2 * (c - 0x4f));
            if ((uv & 255) > 247 || (uv >> 8) > 247) return 0;
            parsed.name[parsed.count++] = uv;
        } else {
            return 0;
        }
        off += 2;
    }
    if (off == size || !parsed.count) return 0;
    *hud = parsed;
    return 1;
}

static void draw_name(const BfmSteamKnightBossHud *hud, const uint16_t tiles[512][1024],
                      uint32_t pixels[240][320])
{
    const BfmForestRaster surface = {tiles, pixels, 10, 229};
    const BfmForestRasterMaterial font = {.page = 21, .clut = 0x4056, .count = 4, .textured = 1};
    int x = 18;
    for (unsigned i = 0; i < hud->count; ++i) {
        if (hud->name[i] == UINT16_MAX) {
            x += 4;
            continue;
        }
        unsigned u = hud->name[i] & 255, v = hud->name[i] >> 8;
        const BfmForestRasterVertex glyph[4] = {
            {x,     25, u,     v,     0x808080}, {x + 8, 25, u + 8, v,     0x808080},
            {x,     33, u,     v + 8, 0x808080}, {x + 8, 33, u + 8, v + 8, 0x808080}
        };
        bfm_field_raster_face(&surface, &font, glyph);
        x += 7;
    }
}

static uint32_t shadow_pixel(uint32_t p, int amount)
{
    uint32_t q = 0xff000000;
    for (unsigned shift = 0; shift <= 16; shift += 8) {
        int level = (int)((p >> (shift + 3)) & 31u) - amount;
        if (level > 0) q |= (uint32_t)(level * 255 / 31) << shift;
    }
    return q;
}

static uint32_t ramp(unsigned width)
{
    return width <= 48 ? 255u | (255u * width / 48u << 8)
                       : (255u - 254u * (width - 48u) / 48u) | 0xff00u;
}

static void bar_region(BfmForestRaster *surface, unsigned start, unsigned end)
{
    uint32_t color0 = ramp(start), color1 = ramp(end);
    BfmForestRasterVertex corners[4] = {
        {12 + (int)start, 16, 0, 0, color0}, {12 + (int)end, 16, 0, 0, color1},
        { 8 + (int)start, 23, 0, 0, color0}, { 8 + (int)end, 23, 0, 0, color1}
    };
    const BfmForestRasterMaterial m = {.count = 4, .dither = 1};
    bfm_field_raster_face(surface, &m, corners);
}

void bfm_steam_knight_boss_status(const BfmSteamKnightBossHud *hud, const uint16_t tiles[512][1024],
                                  unsigned body_hp, unsigned age, uint32_t pixels[240][320])
{
    if (!hud || !tiles || !pixels || age < 3) return;
    const unsigned capacity = 192;
    unsigned shown = age - 2 < capacity / 3 ? 3 * (age - 2) : capacity;
    unsigned width = (body_hp < shown ? body_hp : shown) / 2;
    /* The shadow follows the same seven scanlines, five pixels right and
     * four down from the filled slanted bar. Its dither depends on the field pixel. */
    for (int row = 0; row < 7; ++row) {
        int y = 20 + row, start = 17 - row / 2;
        for (int x = start; x < start + (int)width; ++x)
            pixels[y][x] = shadow_pixel(pixels[y][x], 8 + ((x ^ y) % 2 == 0));
    }
    BfmForestRaster surface = {.pixels = pixels, .top = 10, .bottom = 229};
    if (width) bar_region(&surface, 0, width < 48 ? width : 48);
    if (width > 48) bar_region(&surface, 48, width);
    draw_name(hud, tiles, pixels);
}
