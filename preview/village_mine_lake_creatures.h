/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MINE_LAKE_CREATURES_H
#define BFM_VILLAGE_MINE_LAKE_CREATURES_H

#include "village_mine_corridor_387.h"

typedef struct BfmMineLakeCreatures BfmMineLakeCreatures;
typedef int (*BfmMineLakeSlotInUse)(void *context, unsigned slot);

typedef struct {
    int struck;
    BfmMineLungeHit hit;
    unsigned landing_cue, landing_control;
} BfmMineLakeCreatureEvent;

/* The room borrows the same expanded scene and ground until free.
 * Open runs the fixed creature constructor at the numbered pad. */
BfmMineLakeCreatures *bfm_mine_lake_creatures_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, const BfmVillageGround *ground, const int16_t pad[3]);
void bfm_mine_lake_creatures_free(BfmMineLakeCreatures *room);
int bfm_mine_lake_creatures_occupied(const BfmMineLakeCreatures *room, unsigned slot);

/* Visit all occupied slots in ascending order. The fixed placement uses slot 3
 * if it survived the pad cull. Children can acquire later slots in this field.
 * occupied describes the other room actors only. A null run preserves a
 * prepared room until the playthrough takes ownership. */
int bfm_mine_lake_creatures_visit(BfmMineLakeCreatures *room, unsigned slot,
    const int16_t raw[3], const int16_t published[3], const int16_t target[3],
    BfmGameplayRun *run, unsigned tick, const BfmBedroomView *view,
    BfmMineLakeSlotInUse occupied, void *context, BfmMineLakeCreatureEvent *out);
/* Call after the occupied field with the published foot and the same pool. */
void bfm_mine_lake_creatures_admit(BfmMineLakeCreatures *room, const int16_t published[3],
    BfmMineLakeSlotInUse occupied, void *context);

int bfm_mine_lake_creatures_draw_slot(const BfmMineLakeCreatures *room, unsigned slot,
    BfmBedroomFigure *figure, const BfmBedroomView *view);
/* The caller offers the blade segment after earlier surfaces have clipped it.
 * Contact shortens the endpoint, writes the normal and returns the profile's
 * hit sound for feedback. */
int bfm_mine_lake_creatures_blade(BfmMineLakeCreatures *room, unsigned slot,
    unsigned event, unsigned strength, unsigned yaw, const int16_t from[3],
    int16_t to[3], int16_t normal[3], unsigned *sound);

#endif
