/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_FUSION_POWER_H
#define BFM_FOREST_FUSION_POWER_H

#include "forest_field.h"
#include "forest_status.h"

typedef struct {
    uint16_t body[30], sword[30];
    struct {
        uint16_t percent, spread;
    } move[2];
} BfmForestFusionPower;

/* Mount the resident rank table and the first two field attack descriptors. */
int bfm_forest_fusion_power_load(const MusashiDiscMedia *disc, const BfmForestField *field,
                                BfmForestFusionPower *out);
/* The action owner supplies the shared gameplay RNG draw and current flag 0x81. */
int bfm_forest_fusion_strength(const BfmForestFusionPower *power, const BfmForestStatus *status,
                              unsigned move, int boosted, unsigned random_sample, uint16_t *strength);

#endif
