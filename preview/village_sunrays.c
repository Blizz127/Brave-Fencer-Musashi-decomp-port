/* SPDX-License-Identifier: MIT */
#include "village_sunrays.h"
#include "forest_raster.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>

enum { BANDS = 4, QUADS = 3, QUARTER_TURN = 1024 };

typedef struct {
    int x, y;
    unsigned phase, hold;
    int visible, renewed;
} Sunray;

struct BfmVillageSunrays {
    Sunray bands[BANDS];
    int16_t shape[QUADS][4][2];
    uint8_t wave[2 * (QUARTER_TURN + 1)];
    int running;
};

BfmVillageSunrays *bfm_village_sunrays_open(const MusashiDiscMedia *disc,
                                            const uint8_t *scene, size_t bytes)
{
    const size_t geometry = UINT32_C(0x80191074) - UINT32_C(0x80128158);
    const size_t placement = UINT32_C(0x80195980) - UINT32_C(0x80128158);
    const size_t wave = UINT32_C(0x8006d0a8) - UINT32_C(0x80010000) + 2048;
    if (!disc || !scene || bytes < placement + 20 || u16le(scene + placement + 6) != 831)
        return NULL;

    BfmVillageSunrays *rays = calloc(1, sizeof *rays);
    if (!rays) return NULL;
    size_t file_size;
    if (!musashi_disc_media_read_root_range(disc, "SLUS_007.26", wave, rays->wave,
                                            sizeof rays->wave, &file_size) ||
        u16le(rays->wave) != 0 || u16le(rays->wave + 2 * QUARTER_TURN) != 4096) {
        free(rays);
        return NULL;
    }
    const uint8_t *point = scene + geometry;
    for (unsigned i = 0; i < QUADS; ++i) {
        for (unsigned j = 0; j < 4; ++j) {
            rays->shape[i][j][0] = s16le(point);
            rays->shape[i][j][1] = s16le(point + 2);
            point += 4;
        }
    }
    return rays;
}

void bfm_village_sunrays_free(BfmVillageSunrays *rays)
{
    free(rays);
}

static void new_beam(BfmVillageSunrays *rays, BfmGameplayRun *run, unsigned band)
{
    const int spread = 5 * (bfm_gameplay_random(run) & 7);
    Sunray *beam = &rays->bands[band];
    *beam = (Sunray){.hold = band >= 2 ? 60u : 0u, .visible = 1};
    beam->x = (int)band * 80 - 160 + spread + (bfm_gameplay_random(run) & 15);
    beam->y = -2 * (bfm_gameplay_random(run) & 15);
}

void bfm_village_sunrays_step(BfmVillageSunrays *rays, BfmGameplayRun *run, unsigned hour)
{
    if (!rays || !run || hour >= 24) return;
    if (!rays->running) {
        if (hour == 7) {
            /* All four initial placements belong to the shared random stream,
             * including the two bands that start dormant. */
            for (unsigned i = 0; i < BANDS; ++i) {
                new_beam(rays, run, i);
                rays->bands[i].visible = !(i & 1u);
            }
            rays->running = 1;
        }
        return;
    }
    int active = 0;
    for (unsigned i = 0; i < BANDS; ++i) {
        Sunray *beam = &rays->bands[i];
        if (!beam->visible) continue;
        active = 1;
        if (beam->phase > QUARTER_TURN && beam->hold) {
            --beam->hold;
        } else {
            beam->phase += 11;
        }
        if (beam->phase > 2 * QUARTER_TURN) {
            beam->visible = 0;
        } else if (beam->phase >= 1366 && !beam->renewed) {
            beam->renewed = 1;
            if (hour < 9) {
                for (unsigned next = 0; next < BANDS; ++next) {
                    if (rays->bands[next].visible) continue;
                    new_beam(rays, run, next);
                    break;
                }
            }
        }
    }
    if (!active && hour >= 9) rays->running = 0;
}

void bfm_village_sunrays_draw(const BfmVillageSunrays *rays, uint32_t pixels[240][320])
{
    if (!rays || !pixels || !rays->running) return;
    const BfmForestRaster surface = {.pixels = pixels, .top = 10, .bottom = 229};
    const BfmForestRasterMaterial light = {.count = 4, .page = 1u << 5, .translucent = 1};
    for (unsigned i = 0; i < BANDS; ++i) {
        const Sunray *beam = &rays->bands[i];
        if (!beam->visible) continue;
        unsigned index = beam->phase;
        if (index > QUARTER_TURN) index = 2 * QUARTER_TURN - index;
        unsigned level = u16le(rays->wave + 2 * index);
        uint32_t bright = (level / 64) * UINT32_C(0x010101);
        uint32_t dim = (level / 256) * UINT32_C(0x010101);
        for (unsigned j = 0; j < QUADS; ++j) {
            BfmForestRasterVertex face[4] = {
                {.rgb = bright}, {.rgb = bright}, {.rgb = dim}, {.rgb = 0}
            };
            for (unsigned k = 0; k < 4; ++k) {
                face[k].x = rays->shape[j][k][0] + 160 + beam->x;
                face[k].y = rays->shape[j][k][1] + 120 + beam->y;
            }
            bfm_field_raster_face(&surface, &light, face);
        }
    }
}
