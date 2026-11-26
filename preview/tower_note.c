/* SPDX-License-Identifier: MIT */
#include "tower_note.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_bytes.h"

BfmForestMessageArt *bfm_tower_note_art(const BfmForestField *room, const MusashiDiscMedia *disc)
{
    size_t bytes;
    const uint8_t *scene = bfm_forest_field_scene(room, &bytes);
    return bfm_castle_message_art_load(disc, scene, bytes, bfm_forest_field_tiles(room), 0x5bad0);
}

BfmForestMessage *bfm_tower_note_open(const BfmForestField *room, unsigned entrance,
                                      BfmTowerNoteKind note)
{
    const uint8_t *door = bfm_forest_field_entrance(room, entrance);
    if (!door || u16le(door) != 0x3009) return NULL;
    /* The entrance selects its voiced monologue. The first uncovered plate
     * has its own short conversation before the room accepts a footfall. */
    uint32_t plate = UINT32_C(0x8018a6a8);
    uint32_t begin = note == BFM_TOWER_ARRIVAL_NOTE ? u32le(door + 12) : plate;
    uint32_t end = note == BFM_TOWER_ARRIVAL_NOTE ? plate : UINT32_C(0x8018a704);
    if (begin >= end) return NULL;
    return bfm_field_message_borrow(bfm_forest_room_bytes(room, begin, end - begin), end - begin);
}
