/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_STUN_CAST_H
#define BFM_TOWER_STUN_CAST_H

#include "forest_field.h"

typedef struct BfmTowerStunCast BfmTowerStunCast;
enum { BFM_TOWER_STUN_RAYS = 3 };
typedef struct {
    const BfmForestPose *body;
    uint32_t pose, visible;
    int8_t blade[12];
    unsigned key, ray_count;
    int16_t origin[3], end[BFM_TOWER_STUN_RAYS][3];
} BfmTowerStunFrame;

BfmTowerStunCast *bfm_tower_stun_cast_open(const MusashiDiscMedia *disc,
                                            const BfmForestField *field);
void bfm_tower_stun_cast_free(BfmTowerStunCast *cast);
/* Begin accepts one already-paid request while idle. Each call advances one
 * field update. A nonzero return supplies that update's body and optional
 * room-contact rays. A zero return leaves the cleared frame to locomotion.
 * The rooted transform includes the player's heading. */
int bfm_tower_stun_cast_tick(BfmTowerStunCast *cast, int begin,
                             const BfmForestTransform *root, BfmTowerStunFrame *frame);
#endif
