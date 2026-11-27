/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_BOX_RETURN_H
#define BFM_WOODS_BOX_RETURN_H

#include "bedroom_record.h"
#include <stddef.h>
#include <stdint.h>

/* Borrowed records from a mounted field. A passage describes the outgoing
 * transfer; the receiving field selects its own entrance by index. */
typedef struct {
    const uint8_t *passage_actor, *passage;
    const uint8_t *valley_actor, *valley_passage;
    const uint8_t *box_actor, *box_destination, *box_script;
    unsigned room;
} BfmWoodsBoxRoute;

int bfm_woods_box_route_open(BfmWoodsBoxRoute *route, unsigned room,
                            const uint8_t *scene, size_t bytes);

const uint8_t *bfm_woods_passage_actor(const BfmWoodsBoxRoute *route, const uint8_t *door);

/* Read the mounted contact for the Cleft mountain and valley paths.
 * Call after the ordinary player sweep, before consuming its wall contact. */
const uint8_t *bfm_woods_box_passage(const BfmWoodsBoxRoute *route,
                                    const int16_t foot[3], unsigned wall_contact);

/* Return the local placement and actor program for this arrival. For an
 * explicit Memory Box respawn the placement must be the recorded marker. */
const uint8_t *bfm_woods_box_arrival(const BfmWoodsBoxRoute *route,
                                    const uint8_t destination[36]);

typedef struct BfmForestMessage BfmForestMessage;
typedef struct BfmGameplayRun BfmGameplayRun;

typedef struct {
    const BfmWoodsBoxRoute *route;
    const uint8_t *main;
    size_t main_bytes;
    unsigned cursor, wait, answer, wait_for_answer, openness;
    int lid_motion;
} BfmWoodsBoxUse;

/* Begin only after the ordinary Square interaction has selected the Box.
 * Both mounted assets outlive the interaction and its text windows. */
int bfm_woods_box_use_begin(BfmWoodsBoxUse *use, const BfmWoodsBoxRoute *route,
                            const uint8_t *main, size_t main_bytes);
void bfm_woods_box_use_close(BfmWoodsBoxUse *use, unsigned answer);

/* Run after the frame's current window closes, or once per field tick when
 * it stays open. Emits at most one new window and sound in each update. */
int bfm_woods_box_use_step(BfmWoodsBoxUse *use, BfmGameplayRun *run,
                           const BfmSaveRecord *live, BfmForestMessage **opened, unsigned *cue);

#endif
