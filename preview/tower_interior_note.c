/* SPDX-License-Identifier: MIT */
#include "tower_interior_note.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_bytes.h"

enum { WAIT_FOR_CLUE, DRAWING_CLUE, READING_CLUE, STOWING_CLUE, WAIT_FOR_REPLY, REPLYING, DONE };

int bfm_tower_interior_note_begin(BfmTowerInteriorNote *note, const BfmForestField *room,
                                  const MusashiDiscMedia *disc, uint32_t parts, BfmForestPose *pose)
{
    BfmTowerInteriorNote fresh = {0};
    if (!note || !bfm_forest_speech_actor_begin(&fresh.actor, room, disc, parts, pose)) return 0;
    *note = fresh;
    return 1;
}

int bfm_tower_interior_note_open(BfmTowerInteriorNote *note, unsigned entrance)
{
    if (!note || note->page) return 0;
    const uint8_t *door = bfm_forest_field_entrance(note->actor.field, entrance);
    if (!door || u16le(door) != 0x300a) return 0;
    uint32_t start = u32le(door + 12);
    const uint32_t next_message = UINT32_C(0x801a3220);
    if (start >= next_message) return 0;
    size_t size = next_message - start;
    note->page = bfm_field_message_borrow(bfm_forest_room_bytes(note->actor.field, start, size), size);
    return note->page != NULL;
}

static uint32_t gesture(BfmTowerInteriorNote *note)
{
    switch (note->stage) {
    case WAIT_FOR_CLUE:
        if (note->pending_flags & 1u) {
            note->stage = DRAWING_CLUE;
            return UINT32_C(0x8018a310);
        }
        break;
    case DRAWING_CLUE:
    case READING_CLUE:
        if (note->pending_flags & 2u) {
            note->stage = STOWING_CLUE;
            return UINT32_C(0x8018a390);
        }
        if (note->stage == DRAWING_CLUE && note->actor.complete) {
            note->stage = READING_CLUE;
            return UINT32_C(0x8018a410);
        }
        break;
    case STOWING_CLUE:
    case REPLYING:
        if (note->actor.complete) {
            note->stage = note->stage == STOWING_CLUE ? WAIT_FOR_REPLY : DONE;
            return UINT32_C(0x800d5a34);
        }
        break;
    case WAIT_FOR_REPLY:
        if (note->pending_flags & 4u) {
            note->stage = REPLYING;
            return UINT32_C(0x800d59ec);
        }
        break;
    }
    return 0;
}

int bfm_tower_interior_note_tick(BfmTowerInteriorNote *note, ForestVoice *voice, unsigned *generation,
                                 int confirm, int reveal, BfmForestPose *pose)
{
    if (!note || !pose) return 0;
    if (!note->page) return bfm_forest_speech_motion_tick(&note->actor, 0, pose);
    if (!voice || !generation) return 0;
    const BfmForestMessageFrame *previous = bfm_forest_message_frame(note->page);
    note->pending_flags |= previous->actor_flags;
    if (!bfm_forest_speech_motion_tick(&note->actor, gesture(note), pose)) return 0;
    return bfm_forest_voice_page(voice, note->page, generation, confirm, reveal) != NULL;
}

void bfm_tower_interior_note_close(BfmTowerInteriorNote *note)
{
    if (note) {
        bfm_forest_message_free(note->page);
        note->page = NULL;
    }
}

BfmForestMessageArt *bfm_tower_interior_note_art(const BfmForestField *room, const MusashiDiscMedia *disc)
{
    size_t size = 0;
    const uint8_t *scene = bfm_forest_field_scene(room, &size);
    return bfm_castle_message_art_load(disc, scene, size, bfm_forest_field_tiles(room), 0x5edc4);
}

void bfm_tower_interior_note_draw(const BfmTowerInteriorNote *note, const BfmForestMessageArt *art,
                                  uint32_t pixels[240][320])
{
    if (note && note->page)
        bfm_bedroom_message_art_draw(art, bfm_forest_message_frame(note->page), 161, pixels);
}
