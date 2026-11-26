/* SPDX-License-Identifier: MIT */
#include "steam_knight_core_damage.h"

enum {
    CAP_OPEN = 0x2,
    BROKEN_SIDES = 0x60000,
    FIRST_HALF_SPENT = 0x80000
};

int bfm_steam_knight_core_strike(const BfmSteamKnightCoreState *body, unsigned *struck,
                                 const BfmSteamKnightCoreBoxes *boxes,
                                 const BfmForestTransform *attachment,
                                 const int16_t from[3], const int16_t ends[][3], size_t count,
                                 uint16_t strength, BfmSteamKnightCoreHit *hit)
{
    if (!body || !struck || *struck || !hit || !strength || body->round > 1 ||
        body->body_stage != 11 || !(body->flags & BROKEN_SIDES) ||
        body->hp <= 0 || body->hp > body->max_hp) return 0;

    int floor_hp = body->flags & FIRST_HALF_SPENT ? 0 : body->max_hp / 2;
    if (body->hp <= floor_hp) return 0;

    BfmSteamKnightCoreSurface surface;
    if (body->weak_stage == 1 && !(body->flags & CAP_OPEN))
        surface = BFM_STEAM_CLOSED_CAP;
    else if (body->weak_stage == 2 && (body->flags & CAP_OPEN))
        surface = BFM_STEAM_OPEN_CORE;
    else
        return 0;

    BfmSteamKnightCoreHit result = {0};
    if (!bfm_steam_knight_core_intersect(boxes, surface, attachment, from, ends, count,
                                         &result.contact)) return 0;

    result.hp_after = body->hp;
    if (surface == BFM_STEAM_CLOSED_CAP) {
        result.response = BFM_STEAM_CAP_STRUCK;
    } else {
        result.hp_after = body->hp - strength;
        if (result.hp_after > floor_hp) {
            result.response = BFM_STEAM_CORE_DAMAGED;
        } else {
            result.hp_after = floor_hp;
            result.clear_flags = BROKEN_SIDES;
            if (floor_hp > 0) {
                result.response = BFM_STEAM_CORE_HALF_DEPLETED;
                result.set_flags = FIRST_HALF_SPENT;
            } else {
                result.response = BFM_STEAM_CORE_DEPLETED;
                result.clear_flags |= FIRST_HALF_SPENT;
            }
        }
    }
    *struck = 1;
    *hit = result;
    return 1;
}
