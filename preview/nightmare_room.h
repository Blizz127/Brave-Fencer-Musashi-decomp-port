/* SPDX-License-Identifier: MIT */
#ifndef BFM_NIGHTMARE_ROOM_H
#define BFM_NIGHTMARE_ROOM_H

#include "bedroom_record.h"
#include "forest_voice.h"

typedef struct BfmNightmareRoom BfmNightmareRoom;

typedef enum {
    BFM_NIGHTMARE_ERROR, BFM_NIGHTMARE_ASLEEP, BFM_NIGHTMARE_CLOSE_SHOT,
    BFM_NIGHTMARE_DEPARTING, BFM_NIGHTMARE_TRANSFER
} BfmNightmareStep;

/* Open the saved-data recovery room on its first sleeping-player update.
 * The supplied entry comes from bfm_woods_retry_destination. */
BfmNightmareRoom *bfm_nightmare_room_open(const MusashiDiscMedia *disc,
                                          const BfmBedroomEntry *entry);
void bfm_nightmare_room_free(BfmNightmareRoom *room);

/* Advance one subsequent player update. CLOSE_SHOT marks the first camera
 * change after the wake flash. The last completed view stays available. */
BfmNightmareStep bfm_nightmare_room_step(BfmNightmareRoom *room);
typedef struct {
    unsigned cue;
    /* Flush queued samples from the previous voice before accepting a new generation. */
    unsigned voice_stopped;
    /* Contact effect family 2 uses argument 1 on landing, 2 on run entry. */
    unsigned landed, started_run;
    /* Borrowed from this room until its transfer is accepted and it is freed. */
    const uint8_t *destination;
} BfmNightmareRoomEvent;

/* Ordinary 30 Hz field entry. Each tick advances the player and its focused
 * message together. The input edges belong to this field. A different pending
 * room transfer takes precedence at the authored camera and speech waits. */
BfmNightmareStep bfm_nightmare_room_step_field(BfmNightmareRoom *room, int confirm, int reveal,
                                               int other_transfer, BfmNightmareRoomEvent *event);
ForestVoice *bfm_nightmare_room_voice(BfmNightmareRoom *room);
unsigned bfm_nightmare_room_generation(const BfmNightmareRoom *room);
int bfm_nightmare_room_exit_camera(BfmNightmareRoom *room);
/* Draw the field, its player and its current mounted message. */
int bfm_nightmare_room_draw(BfmNightmareRoom *room, uint32_t pixels[240][320]);

#endif
