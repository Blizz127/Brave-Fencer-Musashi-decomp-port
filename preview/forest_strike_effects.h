/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_STRIKE_EFFECTS_H
#define BFM_FOREST_STRIKE_EFFECTS_H

#include "forest_damage_digits.h"
#include "forest_impact.h"
#include "forest_sparks.h"

typedef enum {
    BFM_STRIKE_EMPTY, BFM_STRIKE_STRIP, BFM_STRIKE_BURST, BFM_STRIKE_SPARK, BFM_STRIKE_NUMBER
} BfmForestStrikeKind;

typedef struct {
    BfmForestStrikeKind kind;
    int fresh;
    union {
        BfmForestImpact impact;
        BfmForestDamageDigits number;
        struct {
            BfmForestSpark motion;
            int32_t origin[3];
            int16_t face[3][3];
            unsigned index;
        } spark;
    };
} BfmForestStrikeEffect;

typedef struct {
    BfmForestStrikeEffect slots[BFM_FOREST_STRIKE_SLOTS];
} BfmForestStrikeEffects;

int bfm_forest_strike_effects_queue(BfmForestStrikeEffects *effects, const BfmForestField *field,
                                    const BfmForestHit *hit, const int32_t eye[3]);
int bfm_forest_strike_effects_queue_hurt(BfmForestStrikeEffects *effects, const BfmForestField *field,
                                         const int16_t player[3], uint16_t amount);
unsigned bfm_forest_strike_effects_step(BfmForestStrikeEffects *effects, const int32_t eye[3],
                                        const BfmForestCamera *camera, BfmGameplayRun *run,
                                        BfmForestDustCard cards[BFM_FOREST_STRIKE_CARDS]);
#endif
