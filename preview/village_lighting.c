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
                          uint32_t room, uint32_t color_table, unsigned phase,
                          uint16_t tiles[512][1024])
{
    if (!light || !scene || !tiles || phase >= 5) return 0;
    const uint8_t *descriptor = mounted(scene, bytes, room, 0xa8);
    if (!descriptor || u32le(descriptor + 0xa4) != 1) return 0;
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
    return begin_daylight(light, scene, bytes, room, UINT32_C(0x80185d3c), phase, tiles);
}

int bfm_woods_daylight_begin(BfmVillageDaylight *light, const uint8_t *scene, size_t bytes,
                             unsigned phase, uint16_t tiles[512][1024])
{
    return begin_daylight(light, scene, bytes, UINT32_C(0x8018ea38), UINT32_C(0x80188050),
                          phase, tiles);
}

void bfm_village_daylight_step(BfmVillageDaylight *light, unsigned phase, uint16_t tiles[512][1024])
{
    if (!light || !tiles || phase >= 5) return;
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
    light->blending = palettes(light, tiles, 0);
    light->wait = 1;
    for (unsigned i = 0; i < 3; ++i) {
        const uint8_t *target = light->lamp_colors + phase * 9 + 3 * i;
        for (unsigned c = 0; c < 3; ++c)
            light->lamps[i].rgb[c] = (uint8_t)approach(light->lamps[i].rgb[c], target[c], 8);
        light->sky[i] = (uint8_t)approach(light->sky[i], u32le(light->sky_colors + (phase * 3 + i) * 4), 1);
    }
}
