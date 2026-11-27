/* SPDX-License-Identifier: MIT */
#include "village_jon_portrait.h"
#include "../pc_port/forest_player_bytes.h"

#include <string.h>

static int tail_anchor(int offset, int right, const uint8_t *limits)
{
    /* The field measures the requested pixel offset in the balloon's local
     * coordinate system before clamping the tail to its two rim segments. */
    int scale = right ? -7232 : 7232;
    int x = offset * 4096 / scale;
    if (x) {
        int inner = x > s16le(limits + 2) && x < s16le(limits + 8);
        int low = s16le(limits + (inner ? 4 : 0));
        int high = s16le(limits + (inner ? 6 : 10));
        if (x < low) x = low;
        if (x > high) x = high;
    }
    int fixed = x * scale;
    return 160 + right + fixed / 4096 - (fixed % 4096 < 0);
}

int bfm_village_jon_portrait_load(BfmVillageJonPortrait *out, const MusashiDiscMedia *disc,
                                 const uint8_t *scene, size_t scene_bytes,
                                 const uint8_t *module, size_t module_bytes,
                                 uint16_t tiles[512][1024])
{
    enum { STAGING = 0x801f1080u - 0x801ef468u, TAIL_LIMITS = 0x8018dc54u - 0x80128158u };
    if (!out || !disc || !scene || !module || !tiles ||
        module_bytes < STAGING + 7 * 8 || scene_bytes < TAIL_LIMITS + 12) return 0;
    const uint8_t *stage = module + STAGING;
    if (u32le(stage) != 0x42 || u32le(stage + 4) != 24 ||
        u32le(stage + 48) != 0x42 || u32le(stage + 52) != 24) return 0;

    BfmVillageJonPortrait loaded = {0};
    for (unsigned hero = 0; hero < 2; ++hero) {
        const uint8_t *placement = stage + (4 + hero) * 8;
        if (u32le(placement) != 0x37 + hero) return 0;
        loaded.right[hero] = u16le(placement + 6) != 0;
        loaded.anchor[hero] = tail_anchor(s16le(placement + 4), loaded.right[hero],
                                          scene + TAIL_LIMITS);
    }

    /* Portrait 24 occupies a 32 by 40 region in the village atlas. Decode
     * that face into a separate message-art cache, then restore the shared
     * portrait slot even if the font or contours could not be loaded. */
    uint16_t previous_face[40][8], previous_palette[16];
    for (unsigned y = 0; y < 40; ++y) {
        memcpy(previous_face[y], &tiles[400 + y][456], sizeof previous_face[y]);
        memcpy(&tiles[400 + y][456], &tiles[464 + y][696], sizeof previous_face[y]);
    }
    memcpy(previous_palette, &tiles[457][352], sizeof previous_palette);
    memcpy(&tiles[457][352], &tiles[509][688], sizeof previous_palette);
    loaded.art = bfm_castle_message_art_load(disc, scene, scene_bytes, tiles, 0x65a84);
    for (unsigned y = 0; y < 40; ++y)
        memcpy(&tiles[400 + y][456], previous_face[y], sizeof previous_face[y]);
    memcpy(&tiles[457][352], previous_palette, sizeof previous_palette);
    if (!loaded.art) return 0;
    *out = loaded;
    return 1;
}
