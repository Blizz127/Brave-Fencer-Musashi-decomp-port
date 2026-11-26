/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_SUMMIT_STUN_H
#define BFM_TOWER_SUMMIT_STUN_H

#include "forest_message.h"
#include "forest_fusion_model.h"
#include "tower_summit_residents.h"

typedef struct BfmTowerSummitStun BfmTowerSummitStun;
typedef struct {
    int grounded, moving, shoulder, square_edge, circle_edge, confirm_edge, reveal_edge;
    uint32_t visible;
} BfmSummitSkillInput;

typedef struct {
    /* Zero action leaves the ordinary ground pose with the walk owner. */
    unsigned action;
    uint32_t pose, visible;
    int suppress_square, blade_away, tether_gauge;
    const BfmForestActor *blade;
    const BfmForestMessage *award;
    /* Equipped skill overlay poses include their own decoded body and anchors. */
    const BfmForestPose *skill_body;
    int8_t skill_anchors[2][2][3];
} BfmSummitSkillFrame;

BfmTowerSummitStun *bfm_tower_summit_stun_open(const MusashiDiscMedia *disc,
        const BfmForestField *room, uint32_t animation_table);
void bfm_tower_summit_stun_free(BfmTowerSummitStun *skill);

/* Feed ordinary physical button edges once per field update. Call before the
 * resident pool tick. A stationary action (21/22/23) owns movement; shoulder
 * stance (17) admits directional steps without changing its facing. The host
 * draws award text and the detached blade from this update's borrowed frame. */
int bfm_tower_summit_stun_tick(BfmTowerSummitStun *skill, BfmTowerSummitResidents *residents,
        BfmGameplayRun *run, const BfmForestTransform *root, int16_t yaw, unsigned tick,
        const BfmSummitSkillInput *input, BfmSummitSkillFrame *out);
#endif
