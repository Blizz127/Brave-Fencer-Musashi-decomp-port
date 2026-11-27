/* SPDX-License-Identifier: MIT */
#ifndef BFM_NIGHTMARE_TRANSITION_H
#define BFM_NIGHTMARE_TRANSITION_H

#include "musashi_disc_media.h"
#include <stdint.h>

typedef enum {
    BFM_NIGHTMARE_TRANSITION_NONE, BFM_NIGHTMARE_STOP_OPTICAL,
    BFM_NIGHTMARE_PREPARE_VILLAGE, BFM_NIGHTMARE_EXIT_CAMERA,
    BFM_NIGHTMARE_RELEASE_ROOM_AUDIO, BFM_NIGHTMARE_CAPTURE_OUTGOING,
    BFM_NIGHTMARE_ENTER_VILLAGE, BFM_NIGHTMARE_REVEAL_COMPLETE
} BfmNightmareTransitionEvent;

typedef struct {
    unsigned stage, increment, progress, shutter_call, wave;
    int pause_requested;
} BfmNightmareTransition;

/* Mount the native field's default wipe increment from the player's disc. */
int bfm_nightmare_transition_open(const MusashiDiscMedia *disc, BfmNightmareTransition *t);

/* On entry into either the sleeping room or lower village, draw fully black
 * immediately. Advance the receiving room's own script alongside the reveal. */
void bfm_nightmare_transition_begin_arrival(BfmNightmareTransition *t);
/* Elapsed playtime runs in the live field between arrival and departure. */
int bfm_nightmare_transition_live(const BfmNightmareTransition *t);
/* Start once upon the sleeping actor's 1010/10 descriptor. */
void bfm_nightmare_transition_begin_departure(BfmNightmareTransition *t);

/* One field/loader service update. The caller owns the wake voice and the
 * completed capture page. PREPARE_VILLAGE requests the distinct receiving
 * field. RELEASE_ROOM_AUDIO retires running effects, resets the room's ambient
 * control, suppresses new ordinary effects and fades the current score.
 * CAPTURE_OUTGOING requests a stable copy of the completed departing field.
 * Supply capture_page_ready only after that copy succeeds, and destination_ready
 * only after the receiving room and its resources are ready for installation. */
BfmNightmareTransitionEvent bfm_nightmare_transition_step(BfmNightmareTransition *t,
                int optical_idle, int capture_page_ready, int destination_ready);

/* Draw after all room layers and HUD. During the shutter, always supply a
 * fresh copy of the retained outgoing picture. This draw is repeatable. */
void bfm_nightmare_transition_draw(const BfmNightmareTransition *t, uint32_t pixels[240][320]);

#endif
