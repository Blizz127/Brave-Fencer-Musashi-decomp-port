/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_GREETING_H
#define BFM_VILLAGE_GREETING_H

#include "forest_message.h"

typedef enum {
    BFM_VILLAGE_ARRIVAL,
    BFM_VILLAGE_HUNGER
} BfmVillageSpeech;

/* Open the corresponding player window when the entrance 5 script reaches
 * its message call. Keep the mounted SC03/2 scene alive for the cursor.
 * Tick with the ST02 voice status and focused button edges; resume the
 * player script after BFM_FOREST_MESSAGE_CLOSED. Free with
 * bfm_forest_message_free. */
BfmForestMessage *bfm_village_greeting_open(const uint8_t *scene, size_t bytes,
                                            BfmVillageSpeech speech);

#endif
