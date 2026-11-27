/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_ROOTRICK_BOULDER_H
#define BFM_WOODS_ROOTRICK_BOULDER_H

#include "bedroom_figure.h"
#include "forest_guard_contact.h"
#include "village_ground.h"

typedef struct BfmWoodsRootrickBoulder BfmWoodsRootrickBoulder;
typedef enum { BFM_BOULDER_ROOTRICK_THROWS, BFM_BOULDER_MUSASHI_THROWS } BfmBoulderThrower;

/* Model and bounds are the previously completed live target transform and its
 * current body box. The foot may differ from the model's displayed root. */
typedef struct {
    const int16_t *foot;
    const BfmForestTransform *model;
    const BfmForestGuardBox *bounds;
} BfmBoulderTarget;

typedef enum {
    BFM_BOULDER_ERROR = -1, BFM_BOULDER_QUIET, BFM_BOULDER_STRUCK_MUSASHI,
    BFM_BOULDER_STRUCK_ROOTRICK, BFM_BOULDER_GONE
} BfmBoulderEvent;
typedef struct BfmBoulderImpact {
    unsigned voice;              /* Bank 1 streamed voice. */
    int16_t impact[3], normal[3]; /* Clipped contact and outward Q12 normal. */
    unsigned hit_flags, power, attacker_yaw; /* Queue to the ordinary hurt receiver. */
    int hero_z_step, block_revival; /* Apply once at physical player contact. */
} BfmBoulderImpact;

/* Spawn only when the mounted race script calls for this actor. Pass the
 * thrower's position after that command's X alignment. On Musashi's branch
 * the fractional coordinates come from the ordinary player movement. */
BfmWoodsRootrickBoulder *bfm_woods_rootrick_boulder_spawn(const uint8_t *scene, size_t bytes,
    const BfmVillageGround *ground, BfmBoulderThrower thrower,
    const int16_t xyz[3], const uint16_t fractions[3]);
void bfm_woods_rootrick_boulder_free(BfmWoodsRootrickBoulder *rock);

/* Musashi's mounted carry action publishes the actor's actual socket and
 * facing. Pass -1 on keys that preserve the boulder's facing. On the release
 * key, pass the corrected socket and set thrown.
 * Process the actor's update afterwards. */
int bfm_woods_rootrick_boulder_carry(BfmWoodsRootrickBoulder *rock,
    const int16_t socket[3], int held_yaw, int thrown);
const int16_t *bfm_woods_rootrick_boulder_grip(const BfmWoodsRootrickBoulder *rock);
/* Defaults for the two ordinary models in this room. An active receiver
 * that selected a different box should supply its own in BfmBoulderTarget. */
int bfm_woods_rootrick_boulder_target_boxes(const uint8_t *scene, size_t bytes,
    BfmForestGuardBox out[2]); /* Indexed by actor, including when no stone is present. */
const BfmForestGuardBox *bfm_woods_rootrick_boulder_body(const BfmWoodsRootrickBoulder *rock,
    BfmBoulderThrower actor);
const int16_t *bfm_woods_rootrick_boulder_xyz(const BfmWoodsRootrickBoulder *rock);

BfmBoulderEvent bfm_woods_rootrick_boulder_step(BfmWoodsRootrickBoulder *rock,
    const BfmBoulderTarget *hero, const BfmBoulderTarget *rival, unsigned rival_yaw,
    uint32_t rival_clip, BfmBoulderImpact *impact);
int bfm_woods_rootrick_boulder_add(const BfmWoodsRootrickBoulder *rock,
    BfmBedroomFigure *figure, const BfmBedroomView *view);
#endif
