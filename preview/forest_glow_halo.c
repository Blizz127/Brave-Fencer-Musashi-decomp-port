/* SPDX-License-Identifier: MIT */
#include "forest_glow_halo.h"
#include "../pc_port/forest_player_bytes.h"
#include <math.h>
#include <stdlib.h>

static int down(int64_t n, int divisor)
{
    return (int)(n / divisor - (n % divisor < 0));
}

static int radius(const uint8_t *halo, uint16_t phase)
{
    const double turn = 6.28318530717958647693;
    int period = s16le(halo + 2);
    double angle = turn * (phase % period) / period;
    int slow = (int)lround(4096 * cos(2 * angle));
    int fast = (int)lround(4096 * cos(4 * angle));
    int amplitude = abs(slow / 2 + 3 * fast / 8 - 2048);
    return 8 + amplitude * s16le(halo) / 4096;
}

unsigned bfm_forest_glows_project(const BfmForestGlows *lights, const BfmForestCamera *camera,
                                  BfmForestDustCard out[BFM_FOREST_HALO_CARDS])
{
    if (!lights || !lights->halo || !camera || !camera->projection || !out) return 0;
    unsigned count = 0;
    for (unsigned i = 0; i < lights->draw_count; ++i) {
        const BfmForestGlowFrame *frame = &lights->frame[i];
        int view[3];
        for (unsigned axis = 0; axis < 3; ++axis) {
            int64_t dot = 0;
            for (unsigned k = 0; k < 3; ++k)
                dot += (int64_t)camera->rotation[axis][k] * frame->position[k];
            view[axis] = down(dot, 4096) + camera->translation[axis];
        }
        int bucket = view[2] / 4;
        if (bucket <= 0 || bucket >= 4096) continue;
        int64_t projection = ((int64_t)camera->projection * 65536 + view[2] / 2) / view[2];
        if (projection > 131071) continue;
        int x = 160 + down(view[0] * projection, 65536);
        int y = 120 + down(view[1] * projection, 65536);
        if (x - 160 < -1024 || x - 160 > 1023 || y - 120 < -1024 || y - 120 > 1023) continue;
        int outer = radius(lights->halo, frame->phase) * (camera->projection + 500) / (4 * bucket);
        int inner = 179 * outer / 256;
        int offsets[5] = {-outer, -inner, 0, inner, outer};
        /* The mounted corner indices select positions around the glow and
         * the artist's three gradient colors. */
        for (unsigned quad = 0; quad < 4; ++quad) {
            BfmForestDustCard *card = &out[count++];
            card->slot = i;
            card->bucket = (unsigned)bucket;
            card->material = (BfmForestRasterMaterial){.page = 1u << 5, .count = 4, .translucent = 1};
            for (unsigned corner = 0; corner < 4; ++corner) {
                unsigned at = quad * 4 + corner;
                unsigned color = lights->halo[60 + at];
                card->vertices[corner] = (BfmForestRasterVertex){
                    .x = x + offsets[lights->halo[28 + at]],
                    .y = y + offsets[lights->halo[44 + at]],
                    .rgb = u32le(lights->halo + 16 + color * 4) & UINT32_C(0xffffff)
                };
            }
        }
    }
    return count;
}
