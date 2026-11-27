/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_CHARGED_TARGETS_H
#define BFM_WOODS_CHARGED_TARGETS_H

#include "village_ground.h"
#include "woods_caster.h"
#include "woods_charged_hit.h"
#include "woods_king.h"
#include "woods_leno.h"

typedef struct {
    BfmWoodsCaster *casters;
    BfmWoodsKing *king;
    BfmWoodsLeno *leno;
    const BfmVillageGround *ground;
} BfmWoodsChargedTargets;

typedef struct {
    unsigned contacts, impact_sound, leno_yelp;
    int16_t point[3], normal[3];
    BfmWoodsChargedLevels levels;
    int terrain;
    uint16_t material;
    int16_t terrain_point[3], terrain_normal[3];
} BfmWoodsChargedResult;

/* One unclipped blade chord. Each body it reaches can receive the event.
 * Point, normal and impact sound describe the last accepted body, while
 * any terrain impact remains available for the room's surface effect. */
int bfm_woods_charged_blade_contact(const BfmWoodsChargedTargets *targets,
    const BfmWoodsChargedHit *hit, const BfmWeaponRank ranks[30], BfmGameplayRun *run,
    BfmWoodsWeaponCredit credit, unsigned heading, unsigned strength,
    const int16_t from[3], const int16_t to[3], BfmWoodsChargedResult *result);

/* One field of the active main wave. Its world center and model heading
 * are independent of Musashi's current position and facing. */
int bfm_woods_charged_wave_contact(const BfmWoodsChargedTargets *targets,
    const BfmWoodsChargedHit *hit, const BfmWeaponRank ranks[30], BfmGameplayRun *run,
    BfmWoodsWeaponCredit credit, unsigned heading, const int16_t center[3],
    BfmWoodsChargedResult *result);

#endif
