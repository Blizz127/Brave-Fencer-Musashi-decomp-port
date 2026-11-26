/* SPDX-License-Identifier: MIT */
#include "village_portrait.h"

#include <string.h>

void bfm_village_macho_portrait_load(uint16_t tiles[512][1024])
{
    /* The SC03 village atlas stores four indexed pixels per word. The
     * shared dialogue renderer reads a packed 32 by 40 face and its CLUT. */
    for (unsigned y = 0; y < 40; ++y)
        memcpy(&tiles[400 + y][456], &tiles[424 + y][656], 8 * sizeof(uint16_t));
    memcpy(&tiles[457][352], &tiles[506][672], 16 * sizeof(uint16_t));
}
