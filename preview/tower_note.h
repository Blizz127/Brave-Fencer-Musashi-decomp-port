/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_NOTE_H
#define BFM_TOWER_NOTE_H
#include "forest_field.h"
#include "forest_message.h"
#include "forest_message_art.h"

typedef enum {
    BFM_TOWER_ARRIVAL_NOTE,
    BFM_TOWER_PLATE_NOTE
} BfmTowerNoteKind;

BfmForestMessageArt *bfm_tower_note_art(const BfmForestField *room, const MusashiDiscMedia *disc);
BfmForestMessage *bfm_tower_note_open(const BfmForestField *room, unsigned entrance,
                                      BfmTowerNoteKind note);
#endif
