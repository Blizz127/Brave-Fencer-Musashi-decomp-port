/* SPDX-License-Identifier: MIT */
#ifndef BFM_FIELD_FUSION_H
#define BFM_FIELD_FUSION_H
#include "woods_fusion.h"

typedef struct {
    unsigned id, resource_row, resource_tag, section_kinds;
} BfmFieldFusionAbility;

typedef struct {
    uint32_t room, gain, partial_bp, duration, message, credit_palette, glow_edge;
} BfmFieldFusionCatalog;

typedef struct {
    int16_t root[3], center[3];
    unsigned ability, generation;
} BfmFusionRecipient;

typedef enum {
    BFM_FUSION_LATCH, BFM_FUSION_METER, BFM_FUSION_RELEASE, BFM_FUSION_ABSORB
} BfmFusionRecipientEvent;

/* IDs address occurrences in this room. A replacement occurrence must have
 * a distinct generation so a returning sword cannot claim its award. */
typedef struct {
    void *actors;
    const int *order;
    size_t count;
    int (*read)(const void *actors, int id, BfmFusionRecipient *target);
    int (*contact)(const void *actors, const int16_t from[3], const int16_t to[3], int16_t hit[3]);
    int (*react)(void *actors, int id, BfmFusionRecipientEvent event, unsigned meter);
} BfmFieldFusionTargets;

BfmWoodsFusion *bfm_field_fusion_open(const MusashiDiscMedia *disc, const uint8_t *scene,
    size_t bytes, const BfmFieldFusionCatalog *catalog, const BfmFieldFusionAbility *abilities,
    size_t ability_count);
int bfm_field_fusion_step(BfmWoodsFusion *fusion, const BfmFieldFusionTargets *targets,
    BfmGameplayRun *run, BfmForestStatus *status, const BfmBedroomRoot *root, unsigned field_tick,
    const BfmWoodsFusionInput *input, BfmWoodsFusionFrame *frame);
#endif
