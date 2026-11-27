/* SPDX-License-Identifier: MIT */
#ifndef BFM_VALLEY_GATE_DOOR_H
#define BFM_VALLEY_GATE_DOOR_H

#include "bedroom_figure.h"
#include "forest_guard_contact.h"
#include "gameplay_run.h"

typedef struct BfmValleyGateDoor BfmValleyGateDoor;

typedef struct {
    int shake;
    unsigned slide_sounds; /* Requests for cue 0x5c4. Both leaves call it. */
} BfmValleyGateDoorEvents;

/* The two northern rock faces use separate meshes and collision profiles. */
BfmValleyGateDoor *bfm_valley_gate_door_open(const uint8_t *scene, size_t bytes, unsigned stage);
void bfm_valley_gate_door_free(BfmValleyGateDoor *door);

/* Call once per active field after the Soldier update. The landing edge starts
 * both leaves. Restoring stage 160 or later skips the sound, shake and dust. */
BfmValleyGateDoorEvents bfm_valley_gate_door_step(BfmValleyGateDoor *door,
    BfmGameplayRun *run, int soldier_landed);

/* Return the active collision flags for a leaf in room order (0, 1).
 * A nonzero result supplies its finite box and current world transform. */
unsigned bfm_valley_gate_door_shape(const BfmValleyGateDoor *door, unsigned leaf,
    const BfmForestGuardBox **box, const BfmForestTransform **root);
int bfm_valley_gate_door_draw(void *door, BfmBedroomFigure *figure, const BfmBedroomView *view);

#endif
