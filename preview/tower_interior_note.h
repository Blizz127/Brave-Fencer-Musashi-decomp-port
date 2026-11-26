/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_INTERIOR_NOTE_H
#define BFM_TOWER_INTERIOR_NOTE_H

#include "forest_message_art.h"
#include "forest_speech_actor.h"
#include "forest_voice.h"

typedef struct {
    BfmForestSpeechActor actor;
    BfmForestMessage *page;
    unsigned stage, pending_flags;
} BfmTowerInteriorNote;

/* Begin with a fresh presenter when the interior camera starts its sweep. */
int bfm_tower_interior_note_begin(BfmTowerInteriorNote *note, const BfmForestField *room,
                                  const MusashiDiscMedia *disc, uint32_t parts, BfmForestPose *pose);
/* Open the mounted speech on the update following the last entrance camera key. */
int bfm_tower_interior_note_open(BfmTowerInteriorNote *note, unsigned entrance);
/* Step the voice transport before this call. During the entrance sweep only
 * the pose advances. Release the camera and controls when the page closes. */
int bfm_tower_interior_note_tick(BfmTowerInteriorNote *note, ForestVoice *voice, unsigned *generation,
                                 int confirm, int reveal, BfmForestPose *pose);
void bfm_tower_interior_note_close(BfmTowerInteriorNote *note);

BfmForestMessageArt *bfm_tower_interior_note_art(const BfmForestField *room, const MusashiDiscMedia *disc);
void bfm_tower_interior_note_draw(const BfmTowerInteriorNote *note, const BfmForestMessageArt *art,
                                  uint32_t pixels[240][320]);
#endif
