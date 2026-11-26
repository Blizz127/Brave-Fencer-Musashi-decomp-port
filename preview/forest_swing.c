/* SPDX-License-Identifier: MIT */
#include "forest_swing.h"
#include "musashi_forest_player_mesh.h"
#include "../pc_port/forest_player_archive.h"
#include "../pc_port/forest_player_bytes.h"
#include <string.h>

int bfm_forest_swing_load(const MusashiDiscMedia *disc, BfmForestSwing *out)
{
    ForestPlayerArchive archive;
    uint8_t script[128], parts[4 * (MUSASHI_FOREST_PLAYER_PARTS + 1)];
    BfmForestSwing swing = {0};
    if (!disc || !out || !forest_player_archive_open(disc, &archive) ||
        !forest_player_archive_read(&archive, UINT32_C(0x800d4c94), script, sizeof(script)))
        return 0;

    uint32_t list = u32le(script + 4);
    if (u32le(script) != 4 || u32le(script + 8) != 19 || (list & 3) ||
        !forest_player_archive_read(&archive, list, parts, sizeof(parts)) ||
        u32le(parts + 4 * MUSASHI_FOREST_PLAYER_PARTS)) return 0;
    for (unsigned i = 0; i < MUSASHI_FOREST_PLAYER_PARTS; ++i) {
        if ((u32le(parts + 4 * i) >> 24) != 0x80) return 0;
        swing.visible |= UINT32_C(1) << i;
    }
    /* Fusion is equipped. The alternate blade and both Lumina parts are hidden. */
    swing.visible &= ~((UINT32_C(1) << 21) | (UINT32_C(1) << 24) | (UINT32_C(1) << 25));

    unsigned marker = 0, window = 0;
    for (unsigned i = 3; i < sizeof(script) / 4; ++i) {
        uint32_t command = u32le(script + i * 4);
        switch (command) {
        case 0:
            if (!swing.count || marker || window) return 0;
            *out = swing;
            return 1;
        case 7:
            marker = 1;
            break;
        case 20:
            window ^= 1;
            break;
        default: {
            uint8_t packed[256];
            if ((command & 3) || command > UINT32_MAX - sizeof(packed) ||
                swing.count == BFM_FOREST_SWING_KEYS ||
                !forest_player_archive_read(&archive, command, packed, sizeof(packed)))
                return 0;
            BfmForestSwingFrame *frame = &swing.frames[swing.count++];
            frame->pose = command + 16;
            frame->marker = marker;
            frame->window = window;
            memcpy(frame->tip, packed, sizeof(frame->tip));
            marker = 0;
            break;
        }
        }
    }
    return 0;
}

const BfmForestSwingFrame *bfm_forest_swing_frame(const BfmForestSwing *swing, unsigned tick)
{
    return swing && tick < swing->count && tick < BFM_FOREST_SWING_KEYS ? &swing->frames[tick] : NULL;
}
