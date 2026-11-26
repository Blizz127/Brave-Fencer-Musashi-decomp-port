/* SPDX-License-Identifier: MIT */
#include "forest_strike_effects.h"
#include <string.h>

static BfmForestStrikeEffect *allocate(BfmForestStrikeEffects *effects, BfmForestStrikeKind kind)
{
    for (unsigned i = 0; i < BFM_FOREST_STRIKE_SLOTS; ++i) {
        BfmForestStrikeEffect *slot = &effects->slots[i];
        if (slot->kind == BFM_STRIKE_EMPTY) {
            *slot = (BfmForestStrikeEffect){.kind = kind, .fresh = 1};
            return slot;
        }
    }
    return NULL;
}

int bfm_forest_strike_effects_queue(BfmForestStrikeEffects *effects, const BfmForestField *field,
                                    const BfmForestHit *hit, const int32_t eye[3])
{
    BfmForestStrikeEffect *strip = allocate(effects, BFM_STRIKE_STRIP);
    if (!strip) return 1;
    if (!bfm_forest_impact_start(&strip->impact, field, hit->position, hit->normal, eye)) {
        strip->kind = BFM_STRIKE_EMPTY;
        return 0;
    }
    if (hit->strength) {
        BfmForestStrikeEffect *number = allocate(effects, BFM_STRIKE_NUMBER);
        if (number && !bfm_forest_damage_digits_start(&number->number, field, hit->position, hit->strength)) {
            number->kind = BFM_STRIKE_EMPTY;
            strip->kind = BFM_STRIKE_EMPTY;
            return 0;
        }
    }
    return 1;
}

int bfm_forest_strike_effects_queue_hurt(BfmForestStrikeEffects *effects, const BfmForestField *field,
                                         const int16_t player[3], uint16_t amount)
{
    if (!amount || amount == 1000) return 1;
    if (amount > 999) return 0;
    BfmForestStrikeEffect *slot = allocate(effects, BFM_STRIKE_NUMBER);
    if (!slot) return 1;
    if (bfm_forest_damage_digits_prepare_hurt(&slot->number, field, player, amount)) return 1;
    slot->kind = BFM_STRIKE_EMPTY;
    return 0;
}

static void issue_spark(BfmForestStrikeEffects *effects, const BfmForestImpact *parent)
{
    BfmForestStrikeEffect *child = allocate(effects, BFM_STRIKE_SPARK);
    if (child) {
        memcpy(child->spark.origin, parent->origin, sizeof child->spark.origin);
        memcpy(child->spark.face, parent->face.rotation, sizeof child->spark.face);
        child->spark.index = parent->cell;
    }
}

static unsigned step_strip(BfmForestStrikeEffects *effects, BfmForestStrikeEffect *slot,
                           const BfmForestCamera *camera, int emit_sparks, BfmForestDustCard *card)
{
    BfmForestImpact *impact = &slot->impact;
    if (!slot->fresh && ++impact->cell >= impact->strip.count) {
        slot->kind = BFM_STRIKE_EMPTY;
        return 0;
    }
    if (emit_sparks && impact->cell < BFM_FOREST_STRIKE_SPARKS) issue_spark(effects, impact);
    if (slot->fresh) {
        BfmForestStrikeEffect *burst = allocate(effects, BFM_STRIKE_BURST);
        if (burst) burst->impact = *impact;
    }
    return bfm_forest_impact_strip_project(impact, camera, card);
}

unsigned bfm_forest_strike_effects_step(BfmForestStrikeEffects *effects, const int32_t eye[3],
                                        const BfmForestCamera *camera, BfmGameplayRun *run,
                                        BfmForestDustCard cards[BFM_FOREST_STRIKE_CARDS])
{
    unsigned count = 0;
    /* A child allocated below this cursor starts on the following update.
     * Its random velocity is drawn on its own first turn. */
    for (unsigned i = 0; i < BFM_FOREST_STRIKE_SLOTS; ++i) {
        BfmForestStrikeEffect *slot = &effects->slots[i];
        switch (slot->kind) {
        case BFM_STRIKE_STRIP:
            count += step_strip(effects, slot, camera, run != NULL, cards + count);
            break;
        case BFM_STRIKE_BURST:
            if (slot->fresh) bfm_forest_impact_face_eye(&slot->impact, eye);
            else ++slot->impact.cell;
            count += bfm_forest_impact_burst_project(&slot->impact, camera, cards + count);
            if (slot->impact.cell >= slot->impact.burst.count) slot->kind = BFM_STRIKE_EMPTY;
            break;
        case BFM_STRIKE_SPARK:
            if (slot->fresh && run)
                bfm_forest_spark_start(&slot->spark.motion, slot->spark.origin, slot->spark.face, run, eye);
            else
                bfm_forest_spark_advance(&slot->spark.motion);
            count += bfm_forest_spark_project(&slot->spark.motion, slot->spark.index, camera, cards + count);
            if (!slot->spark.motion.alive) slot->kind = BFM_STRIKE_EMPTY;
            break;
        case BFM_STRIKE_NUMBER:
            if (slot->fresh && slot->number.hurt) {
                if (!run) continue;
                bfm_forest_damage_digits_scatter(&slot->number, bfm_gameplay_random(run));
            }
            if (!slot->fresh) bfm_forest_damage_digits_advance(&slot->number);
            count += bfm_forest_damage_digits_project(&slot->number, camera, cards + count);
            if (bfm_forest_damage_digits_last(&slot->number)) slot->kind = BFM_STRIKE_EMPTY;
            break;
        case BFM_STRIKE_EMPTY:
            break;
        }
        slot->fresh = 0;
    }
    return count;
}
