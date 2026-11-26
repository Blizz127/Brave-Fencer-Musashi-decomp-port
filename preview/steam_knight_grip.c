/* SPDX-License-Identifier: MIT */
#include "steam_knight_grip.h"
#include "forest_lift.h"
#include "../pc_port/forest_player_bytes.h"

static int read_box(const uint8_t *record, BfmForestGuardBox *box)
{
    if (u32le(record) != 0) return 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        box->low[axis] = s16le(record + 4 + 4 * axis);
        box->high[axis] = s16le(record + 6 + 4 * axis);
        if (box->low[axis] >= box->high[axis]) return 0;
    }
    return 1;
}

int bfm_steam_knight_grip_load(BfmSteamKnightGrip *out, const uint8_t *scene, size_t bytes)
{
    const uint32_t origin = UINT32_C(0x80128158);
    const uint32_t first_box = UINT32_C(0x80195b2c);
    const size_t shapes = first_box - origin;
    const size_t registration = UINT32_C(0x80196150) - origin;
    if (!out || !scene || bytes < registration + 18 || bytes < shapes + 32)
        return 0;

    const uint8_t *actor = scene + registration;
    BfmSteamKnightGrip loaded;
    if (u32le(actor + 8) != first_box ||
        !read_box(scene + shapes, &loaded.body) ||
        !read_box(scene + shapes + 16, &loaded.pickup)) return 0;
    for (unsigned axis = 0; axis < 3; ++axis)
        loaded.grip[axis] = s16le(actor + 12 + 2 * axis);

    *out = loaded;
    return 1;
}

int bfm_steam_knight_grip_brace(const BfmForestGuardBox *surface,
                                const BfmForestTransform *machine,
                                const int16_t previous[3], const int16_t proposed[3],
                                int16_t corrected[3])
{
    if (!surface || !machine || !previous || !proposed || !corrected ||
        previous[1] < INT16_MIN + 16 || proposed[1] < INT16_MIN + 16) return 0;
    const int16_t from[3] = { previous[0], (int16_t)(previous[1] - 16), previous[2] };
    const int16_t to[3] = { proposed[0], (int16_t)(proposed[1] - 16), proposed[2] };
    int16_t hit[3];
    if (!bfm_forest_body_slide(surface, machine, from, to, hit)) return 0;
    corrected[0] = hit[0];
    corrected[1] = proposed[1];
    corrected[2] = hit[2];
    return 1;
}

int bfm_steam_knight_grip_reaches(const BfmForestGuardBox *surface,
                                  const BfmForestTransform *machine,
                                  const BfmForestTransform *hero, const int16_t feet[3])
{
    if (!surface || !machine || !hero || !feet) return 0;
    const int16_t reach[3] = { 0, -16, -32 };
    BfmForestTransform facing = *hero;
    for (unsigned axis = 0; axis < 3; ++axis)
        facing.translation[axis] = feet[axis];
    int16_t end[3], center[3];
    bfm_forest_lift_world(&facing, reach, end);
    return bfm_forest_guard_contact(surface, machine, feet, end, center);
}
