/* SPDX-License-Identifier: MIT */
#include "forest_shot_effects.h"

static BfmForestShotEffectSlot *free_slot(BfmForestShotEffects *effects)
{
    for (size_t i = 0; i < BFM_FOREST_SHOT_EFFECT_SLOTS; ++i)
        if (effects->slots[i].kind == BFM_SHOT_EFFECT_FREE)
            return &effects->slots[i];
    return NULL;
}

static void add_children(BfmForestShotEffects *effects, const BfmForestShotEmission *births,
                         size_t count, const int32_t eye[3], BfmForestShotRandom random,
                         void *context)
{
    for (size_t i = 0; i < count; ++i) {
        BfmForestShotEffectSlot *slot = free_slot(effects);
        if (!slot) return;
        const BfmForestShotEmission *birth = &births[i];
        if (birth->kind == 9) {
            if (bfm_forest_shot_spark_fire(&slot->state.spark, birth->position,
                            birth->angles, (unsigned)birth->variant, random(context), eye))
                slot->kind = BFM_SHOT_EFFECT_SPARK;
        } else if (birth->kind == 3 && bfm_forest_shot_flash_start(&slot->state.flash,
                            BFM_SHOT_FLASH_CONTACT, birth->position, eye)) {
            slot->kind = BFM_SHOT_EFFECT_FLASH;
        }
    }
}

int bfm_forest_shot_effects_hit(BfmForestShotEffects *effects, const int16_t hit[3],
                                const int16_t normal[3], const int32_t eye[3],
                                BfmForestShotRandom random, void *context)
{
    if (!effects || !hit || !normal || !eye || !random) return 0;
    BfmForestShotEffectSlot *slot = free_slot(effects);
    if (!slot) return 0;
    BfmForestShotEmission births[BFM_FOREST_SHOT_EMISSIONS];
    size_t count = bfm_forest_shot_impact_begin(&slot->state.emitter, hit, normal, births);
    if (!count) return 0;
    slot->kind = BFM_SHOT_EFFECT_EMITTER;
    add_children(effects, births, count, eye, random, context);
    return 1;
}

int bfm_forest_shot_effects_expire(BfmForestShotEffects *effects,
                                   const int16_t point[3], const int32_t eye[3])
{
    if (!effects || !point || !eye) return 0;
    BfmForestShotEffectSlot *slot = free_slot(effects);
    if (!slot || !bfm_forest_shot_flash_start(&slot->state.flash, BFM_SHOT_FLASH_EXPIRY,
                                               point, eye)) return 0;
    slot->kind = BFM_SHOT_EFFECT_FLASH;
    return 1;
}

void bfm_forest_shot_effects_tick(BfmForestShotEffects *effects, const int32_t eye[3],
                                  BfmForestShotRandom random, void *context)
{
    if (!effects || !eye || !random) return;
    /* A child born in a later vacant slot has already executed its birth step. */
    uint32_t active = 0;
    for (size_t i = 0; i < BFM_FOREST_SHOT_EFFECT_SLOTS; ++i)
        if (effects->slots[i].kind != BFM_SHOT_EFFECT_FREE)
            active |= UINT32_C(1) << i;

    for (size_t i = 0; i < BFM_FOREST_SHOT_EFFECT_SLOTS; ++i) {
        if (!(active & (UINT32_C(1) << i))) continue;
        BfmForestShotEffectSlot *slot = &effects->slots[i];
        switch (slot->kind) {
        case BFM_SHOT_EFFECT_EMITTER: {
            BfmForestShotEmission births[BFM_FOREST_SHOT_EMISSIONS];
            size_t count = bfm_forest_shot_impact_tick(&slot->state.emitter, births);
            add_children(effects, births, count, eye, random, context);
            if (!slot->state.emitter.remaining) slot->kind = BFM_SHOT_EFFECT_FREE;
            break;
        }
        case BFM_SHOT_EFFECT_SPARK:
            bfm_forest_shot_spark_tick(&slot->state.spark);
            if (!slot->state.spark.step) slot->kind = BFM_SHOT_EFFECT_FREE;
            break;
        case BFM_SHOT_EFFECT_FLASH:
            bfm_forest_shot_flash_advance(&slot->state.flash);
            if (!slot->state.flash.cell) slot->kind = BFM_SHOT_EFFECT_FREE;
            break;
        case BFM_SHOT_EFFECT_FREE:
            break;
        }
    }
}

size_t bfm_forest_shot_effects_project(const BfmForestShotEffects *effects,
                  const BfmForestDust *atlas, const BfmForestCamera *camera,
                  BfmForestDustCard *cards, size_t capacity)
{
    if (!effects || !camera || !cards) return 0;
    size_t count = 0;
    for (size_t i = 0; i < BFM_FOREST_SHOT_EFFECT_SLOTS && count < capacity; ++i) {
        const BfmForestShotEffectSlot *slot = &effects->slots[i];
        if (slot->kind == BFM_SHOT_EFFECT_SPARK)
            count += bfm_forest_shot_spark_project(&slot->state.spark, camera,
                                                   cards + count, capacity - count);
        else if (slot->kind == BFM_SHOT_EFFECT_FLASH && atlas &&
                 bfm_forest_shot_flash_project(&slot->state.flash, atlas, camera,
                                               &cards[count]))
            ++count;
    }
    return count;
}
