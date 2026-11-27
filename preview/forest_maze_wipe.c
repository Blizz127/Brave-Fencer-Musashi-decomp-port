/* SPDX-License-Identifier: MIT */
#include "forest_maze_wipe.h"

static unsigned subtract_level(unsigned channel, unsigned loss)
{
    unsigned level = (channel & 255u) >> 3;
    if (level <= loss) return 0;
    level -= loss;
    return (level << 3) | (level >> 2);
}

void bfm_forest_maze_wipe_shade(unsigned crossing, unsigned reveal,
                                uint32_t pixels[240][320])
{
    unsigned shade;
    int arriving = reveal && crossing >= reveal;
    if (arriving) {
        unsigned elapsed = crossing - reveal;
        if (elapsed >= BFM_MAZE_REVEAL_POLLS) return;
        /* The incoming full-screen quad subtracts grey 255, 239, ... 15. */
        shade = (255u - 16u * elapsed) >> 3;
    } else {
        if (crossing < BFM_MAZE_HELD_TICK + 5) return;
        unsigned elapsed = crossing - BFM_MAZE_HELD_TICK;
        if (elapsed < 91) {
            /* The sixth shutter update first darkens the outside strip. */
            shade = (elapsed - 3) / 2;
        } else if (elapsed == 91) {
            /* With both jobs ready the loader exits before submitting its
             * final strip list. A pending load submits that list again. */
            shade = reveal == BFM_MAZE_REVEAL_TICK ? 43 : 44;
        } else {
            /* The destination setup or a repeated final list closes the
             * remaining two center strips before the receiver appears. */
            shade = 45;
        }
    }

    for (unsigned y = 0; y < 240; ++y) {
        unsigned band = arriving ? 0 : (y < 120 ? y : 239 - y) / 8;
        if (shade <= band) continue;
        unsigned loss = shade - band;
        for (unsigned x = 0; x < 320; ++x) {
            uint32_t pixel = pixels[y][x];
            unsigned red = subtract_level(pixel >> 16, loss);
            unsigned green = subtract_level(pixel >> 8, loss);
            unsigned blue = subtract_level(pixel, loss);
            pixels[y][x] = 0xff000000u | red << 16 | green << 8 | blue;
        }
    }
}
