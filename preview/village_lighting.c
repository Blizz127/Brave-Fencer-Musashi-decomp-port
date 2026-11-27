/* SPDX-License-Identifier: MIT */
#include "village_lighting.h"
#include "../pc_port/forest_player_bytes.h"

#include <string.h>

int bfm_village_lower_light(BfmBedroomFigure *figure, const uint8_t *scene,
                            size_t bytes, unsigned phase)
{
    const size_t table = 0x8018de3cu - 0x80128158u;
    if (!scene || phase >= 5 || bytes < table || bytes - table < 45) return 0;
    BfmBedroomLight lights[3] = {
        {.axis = {0,30,0}}, {.axis = {0,7,20}}, {.axis = {0,-18,0}}
    };
    const uint8_t *colors = scene + table + phase * 9;
    for (unsigned i = 0; i < 3; ++i) memcpy(lights[i].rgb,colors + 3*i,3);
    return bfm_bedroom_figure_lights(figure,lights);
}

static const uint32_t scene_base = UINT32_C(0x80128158);

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t need)
{
    if (address < scene_base || address - scene_base > bytes ||
        need > bytes - (address - scene_base)) return NULL;
    return scene + (address - scene_base);
}

static unsigned approach(unsigned value, unsigned goal, unsigned step)
{
    if (goal > value && goal - value > step) return value + step;
    if (value > goal && value - goal > step) return value - step;
    return goal;
}

static void shade_street(const BfmStreetColors *colors, uint16_t tiles[512][1024])
{
    for (unsigned i = colors->count; i > 0; --i) {
        const uint8_t *row = colors->rows + 16 * (i - 1);
        unsigned page = u16le(row + 4);
        unsigned sx = 64 * (page & 15) + u16le(row);
        unsigned sy = 16 * (page & 16) + u16le(row + 2);
        unsigned dx = u16le(row + 8), dy = u16le(row + 10);
        unsigned width = u16le(row + 12), height = u16le(row + 14);
        for (unsigned y = 0; y < height; ++y) {
            for (unsigned x = 0; x < width; ++x) {
                unsigned texel = tiles[sy + y][sx + x];
                if (!texel) continue;
                unsigned color = texel & 0x8000;
                for (unsigned c = 0; c < 3; ++c) {
                    unsigned v = ((texel >> (5*c)) & 31) * colors->gain[c] / 128;
                    color |= (v > 31 ? 31 : v) << (5*c);
                }
                tiles[dy + y][dx + x] = (uint16_t)color;
            }
        }
    }
}

int bfm_village_street_colors_begin(BfmStreetColors *colors, const uint8_t *scene, size_t bytes,
                                   uint32_t room, unsigned phase, uint16_t tiles[512][1024])
{
    if (!colors || !scene || !tiles || phase >= 5) return 0;
    uint32_t gains_address;
    unsigned mode;
    switch (room) {
    case UINT32_C(0x80192b18): mode = 0x11; gains_address = UINT32_C(0x8018dca4); break;
    case UINT32_C(0x80190780): mode = 0x15; gains_address = UINT32_C(0x8018b39c); break;
    default: return 0;
    }
    const uint8_t *descriptor = mounted(scene, bytes, room, 0xa8);
    if (!descriptor || u32le(descriptor + 0xa4) != mode) return 0;
    uint32_t list_address = u32le(descriptor + 0xa0);
    const uint8_t *list = mounted(scene, bytes, list_address, 2);
    if (!list) return 0;
    BfmStreetColors next = {.phase = phase, .count = u16le(list), .deferred = mode == 0x15};
    if (!next.count || !mounted(scene, bytes, list_address, 2 + 16 * next.count)) return 0;
    next.rows = list + 2;
    for (unsigned i = 0; i < next.count; ++i) {
        const uint8_t *r = next.rows + 16 * i;
        unsigned u = u16le(r), v = u16le(r + 2), page = u16le(r + 4);
        unsigned x = u16le(r + 8), y = u16le(r + 10);
        unsigned width = u16le(r + 12), height = u16le(r + 14);
        if (((page >> 7) & 3) != 2 || !width || !height ||
            u + width > 256 || v + height > 256 || 64 * (page & 15) + u + width > 1024 ||
            x + width > 1024 || y + height > 512) return 0;
    }
    next.levels = mounted(scene, bytes, gains_address, 60);
    if (!next.levels) return 0;
    for (unsigned i = 0; i < 15; ++i)
        if (u32le(next.levels + 4 * i) > 255) return 0;
    for (unsigned c = 0; c < 3; ++c) next.gain[c] = u32le(next.levels + 12 * phase + 4 * c);
    shade_street(&next, tiles);
    *colors = next;
    return 1;
}

void bfm_village_street_colors_step(BfmStreetColors *colors, unsigned phase, uint16_t tiles[512][1024])
{
    if (!colors || !colors->levels || !tiles || phase >= 5) return;
    /* The illness street submits the new remap after its main world list. */
    if (colors->dirty) {
        shade_street(colors, tiles);
        colors->dirty = 0;
    }
    if (phase != colors->phase) {
        colors->phase = phase;
        colors->wait = 0;
    }
    if (colors->wait) {
        --colors->wait;
        return;
    }
    int changed = 0;
    for (unsigned c = 0; c < 3; ++c) {
        unsigned current = colors->gain[c];
        unsigned target = u32le(colors->levels + 12 * phase + 4 * c);
        colors->gain[c] = approach(current, target, 4);
        changed |= current != colors->gain[c];
    }
    if (changed) {
        if (colors->deferred) colors->dirty = 1;
        else shade_street(colors, tiles);
        colors->wait = 1;
    }
}

int bfm_village_illness_light_begin(BfmVillageDaylight *light, const uint8_t *scene, size_t bytes,
                                   unsigned phase, uint16_t tiles[512][1024])
{
    if (!light || !scene || phase >= 5) return 0;
    BfmVillageDaylight next = {.phase = phase,
        .lamps = {{{0,30,0},{0}}, {{0,7,20},{0}}, {{0,-18,0},{0}}}};
    next.sky_colors = mounted(scene, bytes, UINT32_C(0x8018b518), 60);
    next.lamp_colors = mounted(scene, bytes, UINT32_C(0x8018b4e0), 45);
    if (!next.sky_colors || !next.lamp_colors) return 0;
    for (unsigned i = 0; i < 15; ++i) if (u32le(next.sky_colors + 4*i) > 16) return 0;
    for (unsigned i = 0; i < 3; ++i) {
        next.next_sky[i] = next.sky[i] = (uint8_t)u32le(next.sky_colors + 12*phase + 4*i);
        memcpy(next.lamps[i].rgb, next.lamp_colors + 9*phase + 3*i, 3);
    }
    if (!bfm_village_street_colors_begin(&next.street, scene, bytes, UINT32_C(0x80190780),
                                         phase, tiles)) return 0;
    *light = next;
    return 1;
}

/* Entry uses the full-precision room multipliers. During play each component
 * approaches the color computed from the original entry and an eight-bit gain. */
static int palettes(const BfmVillageDaylight *light, uint16_t tiles[512][1024], int entry)
{
    unsigned gain[3], divisor = entry ? 2560 : 256;
    for (unsigned c = 0; c < 3; ++c)
        gain[c] = entry ? u32le(light->entry_colors + (3 * light->phase + c) * 4) :
                          light->transition_colors[light->phase * 4 + c];
    int changed = 0;
    for (size_t i = 0; i < light->count; ++i) {
        const uint8_t *row = light->rectangles + 16 * i;
        unsigned x = u16le(row + 4), y = u16le(row + 6), width = u16le(row + 8), height = u16le(row + 10);
        const uint8_t *source = light->scene + (u32le(row + 12) - scene_base);
        for (unsigned j = 0; j < height; ++j) {
            uint16_t *dest = &tiles[y + j][x];
            for (unsigned k = 0; k < width; ++k, source += 2) {
                unsigned original = u16le(source), current = dest[k], result = original & 0x8000u;
                for (unsigned c = 0; c < 3; ++c) {
                    unsigned goal = ((original >> (5 * c)) & 31u) * gain[c] / divisor;
                    unsigned v = entry ? goal : approach((current >> (5 * c)) & 31u, goal, 1);
                    result |= (v & 31u) << (5 * c);
                }
                if (!result && original) result = 0x8000u;
                changed |= result != current;
                dest[k] = (uint16_t)result;
            }
        }
    }
    return changed;
}

static int begin_daylight(BfmVillageDaylight *light, const uint8_t *scene, size_t bytes,
                          uint32_t room, uint32_t color_table, unsigned mode, unsigned phase,
                          uint16_t tiles[512][1024])
{
    if (!light || !scene || !tiles || phase >= 5) return 0;
    const uint8_t *descriptor = mounted(scene, bytes, room, 0xa8);
    if (!descriptor || u32le(descriptor + 0xa4) != mode) return 0;
    BfmVillageDaylight next = {.scene = scene, .phase = phase,
        .lamps = {{{0,30,0},{0}}, {{0,7,20},{0}}, {{0,-18,0},{0}}}};
    next.entry_colors = mounted(scene, bytes, color_table, 60);
    next.transition_colors = mounted(scene, bytes, color_table + 0xa8, 20);
    next.lamp_colors = mounted(scene, bytes, color_table + 0xf0, 45);
    next.sky_colors = mounted(scene, bytes, color_table + 0x128, 60);
    next.rectangles = mounted(scene, bytes, u32le(descriptor + 0xa0), 16);
    if (!next.entry_colors || !next.transition_colors || !next.lamp_colors ||
        !next.sky_colors || !next.rectangles) return 0;
    size_t remaining = bytes - (size_t)(next.rectangles - scene);
    const uint8_t *row = next.rectangles;
    while (remaining >= 16 && u16le(row) == 9) {
        unsigned x = u16le(row + 4), y = u16le(row + 6), width = u16le(row + 8), height = u16le(row + 10);
        if (!width || !height || x >= 1024 || y >= 512 || width > 1024 - x || height > 512 - y ||
            !mounted(scene, bytes, u32le(row + 12), (size_t)width * height * 2)) return 0;
        ++next.count;
        row += 16;
        remaining -= 16;
    }
    if (!next.count || remaining < 2 || u16le(row) != 0xffu) return 0;
    for (unsigned i = 0; i < 15; ++i) {
        if (u32le(next.sky_colors + i * 4) > 16 || u32le(next.entry_colors + i * 4) > 2560) return 0;
    }
    for (unsigned i = 0; i < 3; ++i) {
        memcpy(next.lamps[i].rgb, next.lamp_colors + phase * 9 + i * 3, 3);
        next.sky[i] = (uint8_t)u32le(next.sky_colors + (phase * 3 + i) * 4);
    }
    palettes(&next, tiles, 1);
    *light = next;
    return 1;
}

int bfm_village_daylight_begin(BfmVillageDaylight *light, const uint8_t *scene, size_t bytes,
                               uint32_t room, unsigned phase, uint16_t tiles[512][1024])
{
    return begin_daylight(light, scene, bytes, room, UINT32_C(0x80185d3c), 1, phase, tiles);
}

int bfm_woods_daylight_begin(BfmVillageDaylight *light, const uint8_t *scene, size_t bytes,
                             unsigned phase, uint16_t tiles[512][1024])
{
    return begin_daylight(light, scene, bytes, UINT32_C(0x8018ea38), UINT32_C(0x80188050),
                          1, phase, tiles);
}

int bfm_hells_valley_daylight_begin(BfmVillageDaylight *light, const uint8_t *scene, size_t bytes,
                                    unsigned phase, uint16_t tiles[512][1024])
{
    return begin_daylight(light, scene, bytes, UINT32_C(0x80186620), UINT32_C(0x80182ba8),
                          1, phase, tiles);
}

int bfm_forest_bank_daylight_begin(BfmVillageDaylight *light, const uint8_t *scene, size_t bytes,
                                   unsigned phase, uint16_t tiles[512][1024])
{
    return begin_daylight(light, scene, bytes, UINT32_C(0x801910e4), UINT32_C(0x8018ce20),
                          3, phase, tiles);
}

int bfm_forest_maze_daylight_begin(BfmVillageDaylight *light, const uint8_t *scene, size_t bytes,
                                   unsigned phase, uint16_t tiles[512][1024])
{
    return begin_daylight(light, scene, bytes, UINT32_C(0x8018c890), UINT32_C(0x80187c18),
                          1, phase, tiles);
}

void bfm_village_daylight_step(BfmVillageDaylight *light, unsigned phase, uint16_t tiles[512][1024])
{
    if (!light || !tiles || phase >= 5) return;
    uint8_t *sky_gain = light->sky;
    if (light->street.levels) {
        /* The sky is queued before the room phase hook changes its gains;
         * body lighting is consumed after the hook on this same field. */
        memcpy(light->sky, light->next_sky, sizeof light->sky);
        sky_gain = light->next_sky;
        bfm_village_street_colors_step(&light->street, phase, tiles);
    }
    if (light->phase != phase) {
        light->phase = phase;
        light->blending = 1;
        light->wait = 0;
    }
    if (!light->blending) return;
    if (light->wait) {
        --light->wait;
        return;
    }
    light->blending = light->street.levels ? 0 : palettes(light, tiles, 0);
    light->wait = 1;
    for (unsigned i = 0; i < 3; ++i) {
        const uint8_t *target = light->lamp_colors + phase * 9 + 3 * i;
        for (unsigned c = 0; c < 3; ++c) {
            light->lamps[i].rgb[c] = (uint8_t)approach(light->lamps[i].rgb[c], target[c], 8);
            if (light->street.levels && light->lamps[i].rgb[c] != target[c]) light->blending = 1;
        }
        unsigned sky = u32le(light->sky_colors + (phase * 3 + i) * 4);
        sky_gain[i] = (uint8_t)approach(sky_gain[i], sky, 1);
        if (light->street.levels && sky_gain[i] != sky) light->blending = 1;
    }
}
