/* SPDX-License-Identifier: MIT */
#include "woods_charged_targets.h"
#include <string.h>

/* The shared debit handler awards both weapons once per accepting target.
 * The other Cleft Woods actors have their own reactions to the same contact. */
static void credit_caster(const BfmWeaponRank ranks[30], BfmGameplayRun *run,
                          BfmWoodsWeaponCredit credit, BfmWoodsChargedResult *result)
{
    BfmWoodsChargedLevels gain;
    if (bfm_woods_weapon_credit_apply(ranks, run, credit, &gain)) {
        result->levels.lumina += gain.lumina;
        result->levels.fusion += gain.fusion;
    }
}

int bfm_woods_charged_blade_contact(const BfmWoodsChargedTargets *targets,
    const BfmWoodsChargedHit *hit, const BfmWeaponRank ranks[30], BfmGameplayRun *run,
    BfmWoodsWeaponCredit credit, unsigned heading, unsigned strength,
    const int16_t from[3], const int16_t to[3], BfmWoodsChargedResult *result)
{
    if (!targets || !targets->ground || !hit || !ranks || !run || !run->has_player ||
        !from || !to || !result) return 0;
    *result = (BfmWoodsChargedResult){0};
    int16_t end[3];
    memcpy(end, to, sizeof end);
    result->terrain = bfm_village_ground_blade(targets->ground, from, end, result->terrain_normal,
                                             &result->material);
    if (result->terrain) memcpy(result->terrain_point, end, sizeof end);

    if (bfm_woods_king_charged_blade(targets->king, from, end, result->point,
                                    result->normal, &result->impact_sound)) {
        memcpy(end, result->point, sizeof end);
        ++result->contacts;
    }
    if (bfm_woods_leno_charged_blade(targets->leno, from, end, heading, result->point,
                                    result->normal)) {
        memcpy(end, result->point, sizeof end);
        result->impact_sound = 0;
        result->leno_yelp = 1318;
        bfm_woods_caster_number(targets->casters, result->point, strength);
        ++result->contacts;
    }
    for (unsigned target = 1; target <= BFM_WOODS_SATIATE_TARGET; ++target) {
        unsigned applied = bfm_woods_caster_charged_blade(targets->casters, target, strength,
            heading, from, end, result->point, result->normal, &result->impact_sound);
        if (!applied) continue;
        memcpy(end, result->point, sizeof end);
        ++result->contacts;
        if (applied != BFM_WOODS_CASTER_RECOIL_ONLY)
            credit_caster(ranks, run, credit, result);
    }
    return 1;
}

int bfm_woods_charged_wave_contact(const BfmWoodsChargedTargets *targets,
    const BfmWoodsChargedHit *hit, const BfmWeaponRank ranks[30], BfmGameplayRun *run,
    BfmWoodsWeaponCredit credit, unsigned heading, const int16_t center[3],
    BfmWoodsChargedResult *result)
{
    if (!targets || !hit || !ranks || !run || !run->has_player || !center || !result) return 0;
    *result = (BfmWoodsChargedResult){0};
    if (bfm_woods_king_charged_wave(targets->king, center, result->point)) {
        /* The producer rolls before the King can choose its own response. */
        bfm_woods_charged_wave_damage(hit, run);
        ++result->contacts;
    }
    if (bfm_woods_leno_charged_wave(targets->leno, center, heading, result->point)) {
        unsigned displayed = bfm_woods_charged_wave_damage(hit, run);
        bfm_woods_caster_number(targets->casters, result->point, displayed);
        result->leno_yelp = 1318;
        ++result->contacts;
    }
    for (unsigned target = 1; target <= BFM_WOODS_SATIATE_TARGET; ++target) {
        unsigned applied = bfm_woods_caster_charged_wave(targets->casters, target, hit, run,
                                                         center, heading, result->point);
        if (!applied) continue;
        ++result->contacts;
        if (applied != BFM_WOODS_CASTER_RECOIL_ONLY)
            credit_caster(ranks, run, credit, result);
    }
    return 1;
}
