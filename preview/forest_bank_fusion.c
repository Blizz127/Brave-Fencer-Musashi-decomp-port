/* SPDX-License-Identifier: MIT */
#include "forest_bank_fusion.h"
#include "field_fusion.h"
#include "forest_bank_hopper.h"
#include "../pc_port/forest_player_bytes.h"

enum { HOP = 12 };

BfmWoodsFusion *bfm_forest_bank_fusion_open(const MusashiDiscMedia *disc,
                                            const uint8_t *scene, size_t bytes)
{
    static const BfmFieldFusionCatalog bank = {
        .room=0x801910e4, .gain=0x8018f35c, .partial_bp=0x8018f37c, .duration=0x8018f2e0,
        .message=0x8018efd8, .credit_palette=0x8018f748, .glow_edge=0x8018f4d0
    };
    static const BfmFieldFusionAbility ability = {
        .id=HOP, .resource_row=6, .resource_tag=91, .section_kinds=0x0f
    };
    /* The bank actor's own profile selects this ability. */
    const size_t reward = UINT32_C(0x801bfe40) - UINT32_C(0x80128158) + 44;
    if (!scene || bytes < reward + 2 || u16le(scene + reward) != HOP) return NULL;
    return bfm_field_fusion_open(disc, scene, bytes, &bank, &ability, 1);
}

static int hopper_read(const void *actors, int id, BfmFusionRecipient *target)
{
    target->ability = HOP;
    target->generation = bfm_bank_hoppers_generation(actors, id);
    return bfm_bank_hoppers_target(actors, id, target->center) &&
           bfm_bank_hoppers_root(actors, id, target->root);
}

static int hopper_contact(const void *actors, const int16_t from[3], const int16_t to[3], int16_t hit[3])
{
    return bfm_bank_hoppers_contact(actors, from, to, hit);
}

static int hopper_react(void *actors, int id, BfmFusionRecipientEvent event, unsigned meter)
{
    switch (event) {
    case BFM_FUSION_LATCH: return bfm_bank_hoppers_latch(actors, id);
    case BFM_FUSION_METER: bfm_bank_hoppers_meter(actors, id, meter); return 1;
    case BFM_FUSION_RELEASE: bfm_bank_hoppers_release(actors, id); return 1;
    case BFM_FUSION_ABSORB: return bfm_bank_hoppers_absorb(actors, id);
    }
    return 0;
}

int bfm_forest_bank_fusion_step(BfmWoodsFusion *fusion, BfmBankHoppers *hoppers,
    BfmGameplayRun *run, const BfmBedroomRoot *root, unsigned field_tick,
    const BfmWoodsFusionInput *input, BfmWoodsFusionFrame *frame)
{
    static const int placements[] = {1,2,3,4};
    const BfmFieldFusionTargets targets = {hoppers, placements, sizeof placements / sizeof placements[0],
                                           hopper_read, hopper_contact, hopper_react};
    return run && bfm_field_fusion_step(fusion, &targets, run, &run->player, root, field_tick, input, frame);
}
