/* SPDX-License-Identifier: MIT */
#include "steam_knight_carry.h"
#include "forest_walk.h"

int bfm_steam_carry_load(BfmSteamKnightCarry *carry, const MusashiDiscMedia *disc,
                         const uint8_t *scene, size_t bytes)
{
    return carry && bfm_steam_knight_grip_load(&carry->target, scene, bytes) &&
           bfm_steam_hero_lift_load(disc, scene, bytes, &carry->lift);
}

int bfm_steam_carry_knockdown(BfmSteamKnightCarry *carry, BfmSteamKnightMachine *machine,
                              unsigned round)
{
    int32_t position[3];
    if (!carry || !bfm_steam_knight_machine_root_q12(machine, position) ||
        !bfm_steam_throw_collapse_q12(&carry->path, round, position)) return 0;
    carry->held = carry->thrown = 0;
    return 1;
}

static void pickup_root(const BfmSteamKnightCarry *carry, BfmForestTransform *root)
{
    bfm_forest_actor_root(carry->path.round == 2 ? 3072 : 0, carry->path.body, root);
}

int bfm_steam_carry_try_grab(BfmSteamKnightCarry *carry, BfmSteamKnightHeroMotion *hero,
                             uint16_t pressed)
{
    if (!carry || carry->held || carry->thrown) return 0;
    const BfmSteamKnightHeroSample *player = bfm_steam_hero_sample(hero);
    if (!player || player->action != 1 || !(pressed & BFM_FOREST_SQUARE)) return 0;
    BfmForestTransform target, facing;
    pickup_root(carry, &target);
    bfm_forest_actor_root(player->heading, player->feet, &facing);
    if (!bfm_steam_knight_grip_reaches(&carry->target.pickup, &target, &facing, player->feet) ||
        !bfm_steam_hero_grab(hero, pressed, &carry->lift, carry->target.grip)) return 0;
    carry->held = 1;
    return 1;
}

int bfm_steam_carry_check_brace(BfmSteamKnightCarry *carry, BfmSteamKnightHeroMotion *hero,
                                const int16_t previous[3])
{
    const BfmSteamKnightHeroSample *player = bfm_steam_hero_sample(hero);
    if (!carry || !player || !previous) return 0;
    if (carry->held || carry->thrown) return 1;
    BfmForestTransform target;
    int16_t corrected[3];
    pickup_root(carry, &target);
    if (player->action == 1 && bfm_steam_knight_grip_brace(&carry->target.pickup, &target,
                                                           previous, player->feet, corrected))
        return bfm_steam_hero_brace(hero, corrected, &carry->lift);
    return bfm_steam_hero_brace(hero, NULL, NULL);
}

int bfm_steam_carry_tick(BfmSteamKnightCarry *carry, BfmSteamKnightHeroMotion *hero,
                         BfmSteamKnightMachine *machine, unsigned *events)
{
    if (!carry || !hero || !machine || !events) return 0;
    *events = 0;
    if (carry->held) {
        int16_t hand[3];
        BfmLiftStep step = bfm_steam_hero_carry(hero, hand);
        if (step == BFM_LIFT_RELEASE) {
            if (!bfm_steam_knight_machine_world_root(machine, hand) ||
                !bfm_steam_throw_release(&carry->path, hand)) return 0;
            carry->held = 0;
            carry->thrown = 1;
        } else if (step == BFM_LIFT_ATTACH || step == BFM_LIFT_WALK_ATTACH) {
            return bfm_steam_knight_machine_set_world_root(machine, hand);
        } else {
            return 1;
        }
    }
    *events = bfm_steam_throw_step(&carry->path);
    return bfm_steam_knight_machine_set_world_root(machine, carry->path.body);
}
