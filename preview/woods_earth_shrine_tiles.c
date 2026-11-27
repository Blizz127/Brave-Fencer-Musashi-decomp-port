/* SPDX-License-Identifier: MIT */
#include "woods_earth_shrine_tiles.h"
#include "scene_tiles.h"

#include <stdlib.h>
#include <string.h>

int bfm_woods_earth_shrine_tiles(const MusashiDiscMedia *disc, unsigned member,
    uint16_t vram[512][1024], unsigned *current_member)
{
    if (!disc || !vram || !current_member ||
        member < BFM_WOODS_EARTH_RELEASE_TILES || member > BFM_WOODS_EARTH_RESTORE_TILES)
        return 0;
    if (*current_member == member)
        return 1;

    /* These three sparse uploads share one 64-word-wide texture page.
     * The mounted decoder can fail partway through a tile list. Stage the
     * page with its current contents so omitted tiles survive a short list. */
    enum { PAGE_X = 640, PAGE_Y = 256, PAGE_WIDTH = 64, VRAM_HEIGHT = 512 };
    uint16_t (*staged)[1024] = malloc(VRAM_HEIGHT * sizeof(*staged));
    if (!staged)
        return 0;
    const size_t row_bytes = PAGE_WIDTH * sizeof(uint16_t);
    for (unsigned y = PAGE_Y; y < VRAM_HEIGHT; ++y)
        memcpy(&staged[y][PAGE_X], &vram[y][PAGE_X], row_bytes);

    int ok = bfm_scene_read_tiles(disc, "SC03.CD", member, staged);
    if (ok) {
        for (unsigned y = PAGE_Y; y < VRAM_HEIGHT; ++y)
            memcpy(&vram[y][PAGE_X], &staged[y][PAGE_X], row_bytes);
        *current_member = member;
    }
    free(staged);
    return ok;
}
