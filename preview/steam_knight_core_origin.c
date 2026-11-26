/* SPDX-License-Identifier: MIT */
#include "steam_knight_core_origin.h"

unsigned bfm_steam_knight_core_kneel_extension(unsigned held_updates, unsigned open_updates)
{
    unsigned pose_steps = held_updates > 3 ? held_updates - 3 : 0;
    unsigned open_steps = open_updates > 1 ? open_updates - 1 : 0;
    unsigned steps = pose_steps < open_steps ? pose_steps : open_steps;
    return steps >= 6 ? 0 : 24 - 4 * steps;
}

unsigned bfm_steam_knight_core_rise_extension(unsigned reverse_key)
{
    if (reverse_key >= 7) return 0;
    return reverse_key <= 1 ? 24 : 4 * (7 - reverse_key);
}

int bfm_steam_knight_core_transform(const BfmSteamKnightMachine *machine, int waist,
                                    unsigned extension, BfmForestTransform *world)
{
    if (!world || extension > 24 || (!waist && extension)) return 0;
    BfmForestTransform result;
    const MusashiForestPlayerPart *part;
    if (!bfm_steam_knight_machine_joint(machine, waist ? 21 : 0, &result, &part)) return 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        int product = result.rotation[axis][2] * (int)extension;
        int delta = product / 4096 - (product % 4096 < 0);
        int64_t position = (int64_t)result.translation[axis] + delta;
        if (position < INT32_MIN || position > INT32_MAX) return 0;
        result.translation[axis] = (int32_t)position;
    }
    *world = result;
    return 1;
}
