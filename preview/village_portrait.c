/* SPDX-License-Identifier: MIT */
#include "village_portrait.h"

#include <string.h>

static void copy_portrait(uint16_t tiles[512][1024], unsigned left, unsigned top,
                          unsigned palette_x, unsigned palette_y)
{
    /* The SC03 village atlas stores four indexed pixels per word. The
     * shared dialogue renderer reads a packed 32 by 40 face and its CLUT. */
    for (unsigned y = 0; y < 40; ++y)
        memcpy(&tiles[400 + y][456], &tiles[top + y][left], 8 * sizeof(uint16_t));
    memcpy(&tiles[457][352], &tiles[palette_y][palette_x], 16 * sizeof(uint16_t));
}

void bfm_village_macho_portrait_load(uint16_t tiles[512][1024])
{
    copy_portrait(tiles, 656, 424, 672, 506);
}

void bfm_village_hotelo_portrait_load(uint16_t tiles[512][1024])
{
    copy_portrait(tiles, 648, 464, 656, 508);
}

void bfm_village_mayor_portrait_load(uint16_t tiles[512][1024])
{
    copy_portrait(tiles, 672, 464, 640, 509);
}

void bfm_village_wid_portrait_load(uint16_t tiles[512][1024])
{
    copy_portrait(tiles, 688, 464, 672, 509);
}
