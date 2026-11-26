/* SPDX-License-Identifier: MIT */
#include "tower_sky.h"
#include "../pc_port/forest_player_bytes.h"
#include <math.h>
#include <stdlib.h>

enum SkyLayout { TOWER, SUMMIT, FIXED };

struct BfmTowerSky {
    const uint16_t (*tiles)[1024];
    const uint8_t *cells, *materials;
    unsigned columns, rows;
    enum SkyLayout placement;
};

static const uint8_t *room_range(const uint8_t *scene, size_t size,
                                 uint32_t address, size_t count)
{
    const uint32_t base = UINT32_C(0x80128158);
    if (address < base || address - base > size || count > size - (address - base)) return NULL;
    return scene + (address - base);
}

static BfmTowerSky *open_sky(const uint8_t *scene, size_t size, const uint16_t (*tiles)[1024],
                             uint32_t descriptor_slot, enum SkyLayout placement)
{
    if (!scene || !tiles) return NULL;
    const uint8_t *slot = room_range(scene, size, descriptor_slot, 4);
    const uint8_t *room = slot ? room_range(scene, size, u32le(slot), 0x34) : NULL;
    if (!room) return NULL;
    uint32_t sheet = u32le(room + 0x28), layout = u32le(room + 0x30);
    const uint8_t *materials = room_range(scene, size, sheet, 8);
    const uint8_t *map = room_range(scene, size, layout, 8);
    if (!materials || !map) return NULL;
    unsigned columns = map[4], rows = map[5], count = u16le(materials + 4);
    if (map[6] != 16 || map[7] != 16 || materials[6] * materials[7] != count ||
        columns * 16 < 320 || !rows || (placement != FIXED && rows * 16 < 240) || !count ||
        !room_range(scene, size, sheet, 8 + 8u * count) ||
        !room_range(scene, size, layout, 8 + 2u * rows * columns)) return NULL;
    materials += 8;
    map += 8;
    for (unsigned i = 0; i < rows * columns; ++i)
        if (u16le(map + 2 * i) >= count) return NULL;
    for (unsigned i = 0; i < count; ++i) {
        const uint8_t *tile = materials + 8 * i;
        unsigned clut = u16le(tile + 2), page = u16le(tile + 6);
        unsigned depth = (page >> 7) & 3;
        if (depth > 1 || tile[0] > 240 || tile[1] > 240) return NULL;
        unsigned bits = 4u << depth;
        if ((page & 15) * 64 + (tile[0] + 15u) * bits / 16 >= 1024 ||
            (clut >> 6) >= 512 || (clut & 63) * 16 + (1u << bits) > 1024) return NULL;
    }
    BfmTowerSky *sky = malloc(sizeof *sky);
    if (sky) *sky = (BfmTowerSky){tiles, map, materials, columns, rows, placement};
    return sky;
}

BfmTowerSky *bfm_tower_sky_open(const uint8_t *scene, size_t size,
                                const uint16_t (*tiles)[1024])
{
    return open_sky(scene, size, tiles, UINT32_C(0x8018350c), TOWER);
}

BfmTowerSky *bfm_tower_summit_sky_open(const uint8_t *scene, size_t size,
                                       const uint16_t (*tiles)[1024])
{
    return open_sky(scene, size, tiles, UINT32_C(0x8018abb0), SUMMIT);
}

BfmTowerSky *bfm_castle_drawbridge_sky_open(const uint8_t *scene, size_t size,
                                           const uint16_t (*tiles)[1024])
{
    return open_sky(scene, size, tiles, UINT32_C(0x8017dc90), FIXED);
}

void bfm_tower_sky_free(BfmTowerSky *sky)
{
    free(sky);
}

static uint32_t field_color(uint16_t word, enum SkyLayout placement)
{
    unsigned r = word & 31, g = (word >> 5) & 31, b = (word >> 10) & 31;
    if (placement == FIXED)
        return UINT32_C(0xff000000) | (r * 255 / 31) << 16 | (g * 255 / 31) << 8 | b * 255 / 31;
    return UINT32_C(0xff000000) | (r * 8 + r / 4) << 16 |
                                  (g * 8 + g / 4) << 8 | (b * 8 + b / 4);
}

int bfm_tower_sky_draw(const BfmTowerSky *sky, const BfmForestCamera *camera,
                       uint32_t pixels[240][320])
{
    if (!sky || !camera || !pixels) return 0;
    const int16_t *forward = camera->rotation[2];
    double level = hypot((double)forward[0], (double)forward[2]);
    if (level == 0 && !forward[1]) return 0;
    const double turns = 2048.0 / acos(-1.0);
    int heading = (int)lround(atan2((double)forward[0], (double)forward[2]) * turns);
    int pitch = (int)lround(atan2((double)forward[1], level) * turns);
    int width = (int)sky->columns * 16, height = (int)sky->rows * 16;
    int left, top;
    if (sky->placement == FIXED) {
        left = top = 0;
    } else if (sky->placement == SUMMIT) {
        left = (80 + (heading & 4095) * width / 4096) % width;
        top = 160 + pitch * 360 / 1024;
    } else {
        left = (width / 2 + heading * width / 4096) % width;
        top = 200 + pitch * 300 / 1024;
    }
    if (left < 0) left += width;
    if (top < 0) top = 0;
    if (sky->placement != FIXED && top > height - 240) top = height - 240;
    for (unsigned y = 0; y < 240; ++y) {
        if (y < 10 || y >= 230 || y + (unsigned)top >= (unsigned)height) {
            for (unsigned x = 0; x < 320; ++x) pixels[y][x] = UINT32_C(0xff000000);
            continue;
        }
        unsigned map_y = (unsigned)top + y;
        const uint8_t *row = sky->cells + (map_y / 16) * sky->columns * 2;
        unsigned map_x = (unsigned)left;
        for (unsigned x = 0; x < 320;) {
            unsigned material = u16le(row + (map_x / 16) * 2);
            const uint8_t *cell = sky->materials + material * 8;
            unsigned clut = u16le(cell + 2), page = u16le(cell + 6);
            unsigned bits = 4u << ((page >> 7) & 1);
            unsigned mask = (1u << bits) - 1;
            const uint16_t *palette = sky->tiles[clut >> 6] + (clut & 63) * 16;
            const uint16_t *texture = sky->tiles[(page >> 4 & 1) * 256 + cell[1] + map_y % 16]
                                      + (page & 15) * 64;
            unsigned inside = map_x % 16, n = 16 - inside;
            if (n > 320 - x) n = 320 - x;
            for (unsigned i = 0; i < n; ++i) {
                unsigned bit = (cell[0] + inside + i) * bits;
                unsigned index = (texture[bit / 16] >> (bit % 16)) & mask;
                pixels[y][x + i] = field_color(palette[index], sky->placement);
            }
            map_x = (map_x + n) % (unsigned)width;
            x += n;
        }
    }
    return 1;
}
