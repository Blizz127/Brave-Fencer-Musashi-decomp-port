/* SPDX-License-Identifier: MIT */
#include "forest_lesson_pages.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_archive.h"
#include "../pc_port/forest_player_bytes.h"

static BfmForestMessage *open_award(const BfmForestField *field, const MusashiDiscMedia *disc)
{
    /* The room indexes the shared acquisition messages by the earned ability.
     * Its successor in this table bounds the read from the mounted player PAC. */
    enum { GUNSHOT = 8 };
    const uint32_t table = UINT32_C(0x80189514);
    const uint8_t *pair = bfm_forest_room_bytes(field, table + 4 * GUNSHOT, 8);
    ForestPlayerArchive archive;
    if (!pair || !forest_player_archive_open(disc, &archive)) return NULL;
    uint32_t begin = u32le(pair), end = u32le(pair + 4);
    uint8_t program[256];
    if (end <= begin || end - begin > sizeof(program) ||
        !forest_player_archive_read(&archive, begin, program, end - begin)) return NULL;
    return bfm_field_message_open(program, end - begin);
}

BfmForestMessage *bfm_forest_lesson_open(const BfmForestField *field, const MusashiDiscMedia *disc,
                                        BfmForestLessonPage page)
{
    if (!field || !disc) return NULL;
    uint32_t begin;
    size_t bytes;
    switch (page) {
    case BFM_FOREST_LESSON_CLIFF:         begin = 0x801b17ac; bytes = 52; break;
    case BFM_FOREST_LESSON_ALREADY_KNOWN: begin = 0x801b1720; bytes = 140; break;
    case BFM_FOREST_LESSON_DISCOVERY:     begin = 0x801b17f0; bytes = 85; break;
    case BFM_FOREST_LESSON_INSTRUCTIONS:  begin = 0x801b1848; bytes = 376; break;
    case BFM_FOREST_LESSON_AWARD:         return open_award(field, disc);
    case BFM_FOREST_LESSON_LOGS:          begin = 0x801b19c0; bytes = 82; break;
    default: return NULL;
    }
    return bfm_field_message_borrow(bfm_forest_room_bytes(field, begin, bytes), bytes);
}
