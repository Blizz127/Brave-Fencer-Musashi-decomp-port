/* SPDX-License-Identifier: MIT */
#include "woods_flight_glow.h"
#include "forest_bullet_art.h"
#include <string.h>

static int down(int value, int divider)
{
    return value / divider - (value % divider < 0);
}

int bfm_woods_flight_glow_load(BfmWoodsFlightGlow *art, const uint8_t *scene, size_t size)
{
    const size_t origin = UINT32_C(0x80128158);
    const size_t edge = UINT32_C(0x8018a700) - origin;
    const size_t round = UINT32_C(0x8018a714) - origin;
    const size_t ring = UINT32_C(0x8018a728) - origin;
    if (!art || !scene || size < ring + 34 || memcmp(scene+edge, scene+edge+16, 2) ||
        memcmp(scene+round, scene+round+16, 2) || memcmp(scene+ring, scene+ring+32, 2))
        return 0;
    *art = (BfmWoodsFlightGlow){.edge=scene+edge, .round=scene+round, .attached_ring=scene+ring};
    return 1;
}

void bfm_woods_flight_glow_tick(BfmWoodsFlightGlow *art, const BfmFusionFlight *flight,
    unsigned meter, const int32_t eye[3], BfmTetherRandom random, void *context)
{
    if (!art || !flight || !eye || !random) return;
    bfm_woods_blade_sparks_tick(&art->sparks);
    art->draw_blade = 0;
    if (flight->phase == BFM_FUSION_IDLE || flight->phase == BFM_FUSION_DONE) {
        art->alive = 0;
        return;
    }
    if (!art->alive) {
        art->alive = 1;
        art->gripped = 0;
        art->evolution = 0;
        art->attached_ticks = 0;
        art->pulse = 0;
        art->threshold = 128;
        art->delta = 32;
        return;
    }
    if (art->delta > 0 && art->pulse > art->threshold) {
        art->threshold = random(context) & 48;
        art->delta = art->gripped ? -32 : -48;
    } else if (art->delta < 0 && art->pulse <= art->threshold) {
        art->threshold = 64 + (int)(random(context) & 48);
        art->delta = art->gripped ? 32 : 48;
    }
    art->pulse += art->delta;
    if (art->gripped) {
        art->evolution += (art->attached_ticks & 8) ? 128 : -64;
        ++art->attached_ticks;
    } else {
        bfm_woods_blade_sparks_emit(&art->sparks, flight->model.translation,
                                    &flight->model, eye, random(context));
    }

    /* The latch update still draws the outbound fan. The next update starts
     * the growing halo. A surviving halo keeps its last size term on return. */
    int attached = flight->phase == BFM_FUSION_ATTACHED;
    art->draw_blade = 1;
    art->meter = meter;
    art->visible_evolution = art->evolution;
    if (attached && !art->gripped) art->evolution = 1536;
    art->gripped = attached;
}

size_t bfm_woods_flight_glow_project(const BfmWoodsFlightGlow *art, const BfmFusionFlight *flight,
    const BfmForestCamera *camera, const int32_t eye[3], BfmForestDustCard *out, size_t capacity)
{
    if (!art || !flight || !camera || !eye || !out || !capacity) return 0;
    size_t count = 0;
    if (art->draw_blade) {
        BfmForestTransform facing;
        int pitch = bfm_forest_glow_facing(eye, flight->model.translation, 7, &facing);
        unsigned pulse = (uint8_t)art->pulse;
        count = bfm_forest_glow_fan_project(art->round, 4, &facing, 0,
            (112u<<16) | (pulse<<8) | pulse, camera, out, capacity);
        count += bfm_forest_glow_fan_project(art->edge, 4, &flight->model, 1,
            (16u<<16) | (160u<<8) | 16u, camera, out+count, capacity-count);
        if (art->visible_evolution > 0) {
            int scale = art->visible_evolution / 2 + (int)art->meter * 32;
            for (unsigned row = 0; row < 3; ++row) {
                facing.translation[row] += down(pitch, 64);
                for (unsigned col = 0; col < 3; ++col)
                    facing.rotation[row][col] = (int16_t)down(facing.rotation[row][col] * scale, 4096);
            }
            unsigned green = (uint8_t)(art->pulse + 96);
            count += bfm_forest_glow_fan_project(art->attached_ring, 8, &facing, 0,
                (32u<<16) | (green<<8) | 32u, camera, out+count, capacity-count);
        }
    }
    count += bfm_woods_blade_sparks_project(&art->sparks, camera, out+count, capacity-count);
    count += bfm_woods_capture_light_project(&art->capture, camera, eye, out+count, capacity-count);
    return count;
}
