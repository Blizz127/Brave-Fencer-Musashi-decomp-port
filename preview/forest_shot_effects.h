/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_SHOT_EFFECTS_H
#define BFM_FOREST_SHOT_EFFECTS_H

#include "forest_shot_flash.h"
#include "forest_shot_impact.h"
#include "forest_shot_spark.h"

enum { BFM_FOREST_SHOT_EFFECT_SLOTS = 30,
       BFM_FOREST_SHOT_EFFECT_CARDS = BFM_FOREST_SHOT_EFFECT_SLOTS * BFM_FOREST_SHOT_SPARK_CARDS };

typedef unsigned (*BfmForestShotRandom)(void *context);

typedef struct {
    enum { BFM_SHOT_EFFECT_FREE, BFM_SHOT_EFFECT_EMITTER,
           BFM_SHOT_EFFECT_SPARK, BFM_SHOT_EFFECT_FLASH } kind;
    union {
        BfmForestShotImpact emitter;
        BfmForestShotSpark spark;
        BfmForestShotFlash flash;
    } state;
} BfmForestShotEffectSlot;

typedef struct {
    BfmForestShotEffectSlot slots[BFM_FOREST_SHOT_EFFECT_SLOTS];
} BfmForestShotEffects;

/* Zero-initialize with the field. Tick existing effects before advancing bullets.
 * On an actor hit, pass the outward Q12 normal from that exact intersection.
 * Each allocated scatter child takes a draw from the field's shared generator. */
void bfm_forest_shot_effects_tick(BfmForestShotEffects *effects, const int32_t eye[3],
                                  BfmForestShotRandom random, void *context);
int bfm_forest_shot_effects_hit(BfmForestShotEffects *effects, const int16_t hit[3],
                                const int16_t normal[3], const int32_t eye[3],
                                BfmForestShotRandom random, void *context);
int bfm_forest_shot_effects_expire(BfmForestShotEffects *effects,
                                   const int16_t point[3], const int32_t eye[3]);
size_t bfm_forest_shot_effects_project(const BfmForestShotEffects *effects,
                  const BfmForestDust *atlas, const BfmForestCamera *camera,
                  BfmForestDustCard *cards, size_t capacity);
#endif
