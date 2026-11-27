/* SPDX-License-Identifier: MIT */
#ifndef BFM_VALLEY_BINCHO_H
#define BFM_VALLEY_BINCHO_H

#include "bedroom_figure.h"
#include "village_ground.h"
#include "woods_caster_melee.h"

typedef struct BfmValleyBincho BfmValleyBincho;
typedef struct {
    unsigned sound, captive_yaw;
    int reveal, approach;
} BfmValleyBinchoEvents;

/* The mounted room and live variable bank outlive the actor. */
BfmValleyBincho *bfm_valley_bincho_open(const uint8_t *scene, size_t bytes,
                                        const uint8_t variables[256]);
void bfm_valley_bincho_free(BfmValleyBincho *actor);
const BfmBedroomRoot *bfm_valley_bincho_root(const BfmValleyBincho *actor);
int bfm_valley_bincho_detector(const BfmValleyBincho *actor);
int bfm_valley_bincho_releasing(const BfmValleyBincho *actor);

/* Resolve the player's ordinary body movement against the last completed crystal
 * transform before the room terrain is sampled. */
int bfm_valley_bincho_body(const BfmValleyBincho *actor, const int16_t previous[3],
                          int16_t proposed[3], unsigned tick, int airborne);

/* Only a mounted attack chord reaching the sealed actor can start the release.
 * Resolve the player's attack before advancing the crystal's actor slot. */
int bfm_valley_bincho_hit(BfmValleyBincho *actor, const BfmVillageGround *ground,
                          const BfmWoodsCasterMeleeFrame *swing);
int bfm_valley_bincho_step(BfmValleyBincho *actor, BfmGameplayRun *run, const int16_t player[3],
    const int32_t eye[3], unsigned tick, BfmValleyBinchoEvents *events);
int bfm_valley_bincho_draw(const BfmValleyBincho *actor, BfmBedroomFigure *figure,
                           const BfmBedroomView *view);

#endif
