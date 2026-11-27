/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_MAZE_WIPE_H
#define BFM_FOREST_MAZE_WIPE_H

#include <stdint.h>

/* One step is a field simulation update (two NTSC vblanks). Tick 1 is
 * contact. These offsets assume the transfer and destination are ready at
 * the first eligible poll, with the outgoing backbuffer on its ready parity. */
enum {
    BFM_MAZE_SOURCE_DISPATCH_TICK = 3,
    BFM_MAZE_LAST_SOURCE_TICK = BFM_MAZE_SOURCE_DISPATCH_TICK + 11 + 16 - 1,
    BFM_MAZE_HELD_TICK = BFM_MAZE_LAST_SOURCE_TICK + 2,
    BFM_MAZE_REVEAL_TICK = BFM_MAZE_HELD_TICK + 91 + 6 + 1,
    BFM_MAZE_REVEAL_POLLS = 16
};

/* Supply the original, unshaded outgoing composite through the last source
 * update and hold that image until reveal. From reveal onward, supply the
 * unshaded destination composite. A zero reveal keeps the closed image
 * while the destination is unavailable. */
void bfm_forest_maze_wipe_shade(unsigned crossing, unsigned reveal,
                                uint32_t pixels[240][320]);

#endif
