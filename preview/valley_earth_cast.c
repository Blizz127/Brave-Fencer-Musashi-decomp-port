/* SPDX-License-Identifier: MIT */
#include "valley_earth_cast.h"

enum { READY, TAKEOFF, RISE, HOVER, FALL, RECOVER };

/* The room's idle/strafe charging slots and its takeoff slot refer to the
 * resident player archive. The Earth Scroll selects the other two directly. */
static const uint32_t charge_stand = UINT32_C(0x800d5734);
static const uint32_t charge_walk  = UINT32_C(0x800d5790);
static const uint32_t takeoff      = UINT32_C(0x800d4b48);
static const uint32_t raised_blade = UINT32_C(0x800d53c0);
static const uint32_t downward_cut = UINT32_C(0x800d5404);

static int whole_height(int eighths)
{
    return eighths < 0 ? -((7 - eighths) / 8) : eighths / 8;
}

BfmValleyEarthFrame bfm_valley_earth_cast_step(BfmValleyEarthCast *cast,
    BfmSaveRecord *record, BfmFieldInput input, int can_begin)
{
    BfmValleyEarthFrame frame = {0};
    if (!cast || !record) return frame;

    if (cast->phase == READY) {
        uint8_t *gauge = &record->progress[0x47];
        if (!can_begin || record->progress[0x48] != 2 || !(record->flags[2] & 0x40) ||
            !input.shoulder_held || input.square_pressed || input.cross_pressed) {
            if (cast->charging) *gauge = 0;
            cast->charging = 0;
            return frame;
        }
        if (cast->charging && *gauge == 128 && input.triangle_pressed) {
            cast->charging = 0;
            *gauge = 0;
            cast->phase = TAKEOFF;
            cast->height_eighths = 0;
            cast->velocity_eighths = -18 * 8;
            frame.casting = 1;
            frame.program = takeoff;
            return frame;
        }
        if (input.triangle_pressed) {
            *gauge = 0;
            cast->charging = 0;
            return frame;
        }
        if (cast->charging) {
            unsigned next = *gauge + ((record->flags[4] & 1u) ? 6u : 3u);
            *gauge = next > 128 ? 128 : (uint8_t)next;
        } else {
            *gauge = 0;
            cast->charging = 1;
        }
        frame.charging = 1;
        frame.program = input.x || input.y ? charge_walk : charge_stand;
        return frame;
    }

    frame.casting = 1;
    switch (cast->phase) {
    case TAKEOFF:
        frame.program = takeoff;
        cast->phase = RISE;
        break;
    case RISE:
        frame.program = takeoff;
        cast->velocity_eighths += 13;
        cast->height_eighths += cast->velocity_eighths;
        if (cast->velocity_eighths > 0) {
            cast->phase = HOVER;
            cast->remaining = 10; /* Ten body keys before the raised-blade hold. */
            cast->velocity_eighths = 4 * 8;
            frame.program = raised_blade;
        }
        break;
    case HOVER:
        frame.program = raised_blade;
        if (--cast->remaining == 0) {
            cast->phase = FALL;
            frame.program = downward_cut;
        }
        break;
    case FALL:
        frame.program = downward_cut;
        if (cast->height_eighths >= 0) {
            cast->height_eighths = 0;
            frame.quake = 1;
            cast->remaining = 16;
            cast->phase = RECOVER;
        } else {
            cast->velocity_eighths += 13;
            cast->height_eighths += cast->velocity_eighths;
        }
        break;
    case RECOVER:
        frame.program = downward_cut;
        if (--cast->remaining == 0) {
            cast->phase = READY;
            frame.casting = 0;
            frame.finished = 1;
            frame.program = 0;
        }
        break;
    }
    frame.height = whole_height(cast->height_eighths);
    return frame;
}
