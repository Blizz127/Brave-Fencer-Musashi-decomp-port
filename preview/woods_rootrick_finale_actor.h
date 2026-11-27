/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_ROOTRICK_FINALE_ACTOR_H
#define BFM_WOODS_ROOTRICK_FINALE_ACTOR_H

#include "woods_rootrick_actor.h"

typedef struct {
    const uint8_t *scene;
    size_t bytes;
    unsigned outcome, phase, visible;
    unsigned flash_fields, drive_fields;
    int32_t drive_speed;
    int32_t recoil[3];
    int16_t rim_probe[3];
} BfmWoodsRootrickFinale;

enum {
    BFM_ROOTRICK_FINALE_ERROR = -1,
    BFM_ROOTRICK_FINALE_RELEASE = 1,
    BFM_ROOTRICK_FINALE_SHAKE = 2,
    BFM_ROOTRICK_FINALE_CRY = 4,             /* Area-1 voice 0x4331. */
    BFM_ROOTRICK_FINALE_CONSUMED_HIT = 8,
    BFM_ROOTRICK_FINALE_REMOVE_SHADOW = 16,
    BFM_ROOTRICK_FINALE_STOP_RUMBLE = 32
};

/* Keep the same mounted rival, including his current position, facing, clip,
 * pose cursor and velocity. Result 1 is loss, 2 ordinary win, 3 cape ruse. */
int bfm_woods_rootrick_finale_begin(BfmWoodsRootrickFinale *finale,
    BfmWoodsRootrickActor *rival, unsigned outcome, int visible,
    const uint8_t *scene, size_t bytes);

/* Run on genuine boulder contact. The boulder's creation and subsequent
 * independent movement are the boulder owner's responsibility. */
int bfm_woods_rootrick_finale_boulder_hit(BfmWoodsRootrickFinale *finale, unsigned variant);
int bfm_woods_rootrick_finale_stage(BfmWoodsRootrickFinale *finale, unsigned stage);
void bfm_woods_rootrick_finale_turn(BfmWoodsRootrickFinale *finale,
    BfmWoodsRootrickActor *rival, int16_t relative_yaw);
void bfm_woods_rootrick_finale_drive(BfmWoodsRootrickFinale *finale,
    BfmWoodsRootrickActor *rival, int16_t speed_q8, unsigned fields);

/* One field before the outcome script polls. hero_qualified is the real
 * hero state tested after the boulder hit. rival_hit is the rival's pending
 * field-attack contact. Acknowledge it only when CONSUMED_HIT is returned.
 * The script sets visible and may bind an existing clip or set yaw directly. */
int bfm_woods_rootrick_finale_step(BfmWoodsRootrickFinale *finale,
    BfmWoodsRootrickActor *rival, int hero_qualified, int rival_hit);

#endif
