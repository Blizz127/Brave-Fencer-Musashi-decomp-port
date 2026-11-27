/* SPDX-License-Identifier: MIT */
#include "woods_charged_hit.h"
#include "bedroom_figure.h"
#include "forest_field.h"
#include "../pc_port/forest_player_bytes.h"

int bfm_woods_charged_hit_load(const uint8_t *scene, size_t bytes, BfmWoodsChargedHit *out)
{
    const uint32_t base = UINT32_C(0x80128158);
    const size_t move_offset = UINT32_C(0x801896d0) - base + 8 * 20;
    if (!scene || !out || bytes < move_offset + 20) return 0;
    const uint8_t *move = scene + move_offset;
    const unsigned event = u32le(move);
    if (event != 11 || u32le(move + 4) != 1 || u16le(move + 18) != 2) return 0;

    const size_t power_offset = UINT32_C(0x80189874) - base + event * 12;
    if (bytes < power_offset + 12) return 0;
    const uint8_t *power = scene + power_offset;
    const unsigned mode = u16le(power);
    const unsigned spread = u16le(power + 2);
    const unsigned fusion_exp = u16le(power + 8);
    const unsigned lumina_exp = u16le(power + 10);
    if ((mode & 0xf000) != 0x2000 || spread > INT16_MAX ||
        fusion_exp > UINT8_MAX / 2 || lumina_exp > UINT8_MAX - 5) return 0;
    const size_t sound_offset = UINT32_C(0x80189ae4) - base;
    if (bytes < sound_offset + sizeof out->surface_sound) return 0;
    *out = (BfmWoodsChargedHit){.percent=mode & 0x0fff, .spread=spread,
                                .fusion_exp=fusion_exp, .lumina_exp=lumina_exp};
    for (unsigned i = 0; i < 8; ++i)
        out->surface_sound[i] = u16le(scene + sound_offset + 2 * i);
    return 1;
}

int bfm_woods_charged_hit_step(const BfmWoodsChargedHit *hit,
    const BfmWeaponRank ranks[30], BfmGameplayRun *run, const BfmBedroomRoot *root,
    uint32_t actor_flags, int window, const int8_t tip[3], const int8_t next_tip[3],
    BfmWoodsCasterMeleeFrame *frame)
{
    if (!hit || !frame || (window && (!root || !tip || !next_tip))) return 0;
    /* Both complete weapon stats are summed before the shared percentage
     * is varied. Requesting their base values consumes no random draw. */
    const BfmWeaponPower both_bases = {100, 0, 100, 0, 0};
    BfmSteamSpinRoll base;
    if (!bfm_weapon_roll(ranks, &both_bases, run, actor_flags, &base)) return 0;
    unsigned percent = hit->percent;
    if (hit->spread) percent += bfm_gameplay_random(run) % hit->spread;
    frame->event = 11;
    frame->strength = (unsigned)((uint32_t)base.strength * percent / 100) & 0x7fff;
    frame->critical = 0;
    frame->rays = 0;
    if (window) {
        BfmForestTransform transform;
        if (!bfm_bedroom_figure_actor_root(root, 0, &transform)) return 0;
        const int8_t next[3] = {next_tip[1], next_tip[1], next_tip[2]};
        frame->rays = (unsigned)bfm_forest_blade_rays(&transform, tip, next,
                                                     frame->from, frame->to);
    }
    return 1;
}

unsigned bfm_woods_charged_wave_damage(const BfmWoodsChargedHit *hit, BfmGameplayRun *run)
{
    if (!hit || !run) return 0;
    unsigned draw = bfm_gameplay_random(run) % (hit->spread + 1u);
    return (hit->percent + draw) / 2;
}

static void write_word(uint8_t *p, uint32_t word)
{
    for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(word >> (i * 8));
}

static unsigned gain_weapon(const BfmWeaponRank ranks[30], uint8_t *level_slot,
                            unsigned skill, unsigned cap, unsigned points)
{
    uint32_t rank = u32le(level_slot);
    if (!points || rank >= 29) return 0;
    unsigned limit = ranks[rank].exp_limit[skill];
    if (!limit) return 0;
    uint32_t exp = u32le(level_slot + 4) + points;
    if (exp < limit || rank + 1 >= cap) {
        write_word(level_slot + 4, exp < limit ? exp : limit);
        return 0;
    }
    write_word(level_slot, rank + 1);
    write_word(level_slot + 4, 0);
    return skill == 2 ? ranks[rank + 1].lumina - ranks[rank].lumina :
                        ranks[rank + 1].fusion - ranks[rank].fusion;
}

BfmWoodsWeaponCredit bfm_woods_weapon_credit(const BfmGameplayRun *run, unsigned fusion,
                                          unsigned lumina)
{
    if (lumina) {
        for (unsigned scroll = 5; scroll; --scroll) {
            if (bfm_gameplay_story_flag(run, 21 + scroll)) {
                lumina += scroll;
                break;
            }
        }
    }
    if (bfm_gameplay_story_flag(run, 129)) fusion *= 2;
    return (BfmWoodsWeaponCredit){(uint8_t)lumina, (uint8_t)fusion};
}

int bfm_woods_weapon_credit_read(const uint8_t *scene, size_t bytes, unsigned event,
    const BfmGameplayRun *run, BfmWoodsWeaponCredit *credit)
{
    if (!scene || !run || !credit || event > UINT16_MAX) return 0;
    size_t offset = UINT32_C(0x80189874) - UINT32_C(0x80128158) + 12 * event;
    if (bytes < offset + 12) return 0;
    *credit = bfm_woods_weapon_credit(run, u16le(scene + offset + 8), u16le(scene + offset + 10));
    return 1;
}

int bfm_woods_weapon_credit_apply(const BfmWeaponRank ranks[30], BfmGameplayRun *run,
    BfmWoodsWeaponCredit credit, BfmWoodsChargedLevels *levels)
{
    if (!ranks || !run || !run->has_player || !levels) return 0;
    *levels = (BfmWoodsChargedLevels){0};
    uint8_t *status = run->player.record;
    uint32_t story = u32le(status + 8);
    if (story < 10 || story > INT32_MAX) return 1;
    unsigned cap = story < 200 ? 8 : story < 600 ? 16 : story < 900 ? 22 :
                   story < 1200 ? 27 : 99;
    levels->lumina = gain_weapon(ranks, status + 0x84, 2, cap, credit.lumina);
    levels->fusion = gain_weapon(ranks, status + 0x7c, 3, cap, credit.fusion);
    return 1;
}

int bfm_woods_charged_hit_connected(const BfmWoodsChargedHit *hit,
    const BfmWeaponRank ranks[30], BfmGameplayRun *run, BfmWoodsChargedLevels *levels)
{
    if (!hit || !run) return 0;
    BfmWoodsWeaponCredit credit = bfm_woods_weapon_credit(run, hit->fusion_exp, hit->lumina_exp);
    return bfm_woods_weapon_credit_apply(ranks, run, credit, levels);
}
