/* SPDX-License-Identifier: MIT */
#include "woods_pole_climb.h"
#include "../pc_port/forest_player_bytes.h"
#include "forest_guard_contact.h"

#include <limits.h>

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address,
                              size_t length)
{
    const uint32_t base = UINT32_C(0x80128158);
    if (!scene || address < base) return NULL;
    size_t offset = address - base;
    return offset <= bytes && length <= bytes - offset ? scene + offset : NULL;
}

static int load_poles(BfmWoodsPoleLayout *layout, const uint8_t *scene, size_t bytes,
                      uint32_t room_address, uint32_t shape_address, unsigned pole_class,
                      unsigned expected)
{
    const uint8_t *room = mounted(scene, bytes, room_address, 0xbc);
    const uint8_t *shape = mounted(scene, bytes, shape_address, 16);
    if (!layout || !room || !shape || u32le(shape)) return 0;
    uint32_t begin = u32le(room + 0xb4), end = u32le(room + 0xb8);
    if (end <= begin || (end - begin) % 20) return 0;
    const uint8_t *row = mounted(scene, bytes, begin, end - begin);
    if (!row) return 0;

    BfmWoodsPoleLayout next = {0};
    for (unsigned axis = 0; axis < 3; ++axis) {
        next.minimum[axis] = s16le(shape + 4 + 4 * axis);
        next.maximum[axis] = s16le(shape + 6 + 4 * axis);
        if (next.minimum[axis] > next.maximum[axis]) return 0;
    }
    for (size_t left = end - begin; left >= 20; row += 20, left -= 20) {
        unsigned kind = u16le(row + 6);
        if (!kind) {
            if (next.count != expected) return 0;
            *layout = next;
            return 1;
        }
        if (kind != pole_class) continue;
        if (next.count == expected || u16le(row + 12)) return 0;
        /* Only subtype zero uses the mine's first contact shape. */
        if (pole_class == 482 && u16le(row + 8)) return 0;
        for (unsigned axis = 0; axis < 3; ++axis) {
            int origin = s16le(row + 2 * axis);
            if (origin + next.minimum[axis] < INT16_MIN ||
                origin + next.maximum[axis] > INT16_MAX) return 0;
            next.origin[next.count][axis] = (int16_t)origin;
        }
        ++next.count;
    }
    return 0;
}

int bfm_woods_pole_load(BfmWoodsPoleLayout *layout, const uint8_t *scene, size_t bytes)
{
    return load_poles(layout, scene, bytes, UINT32_C(0x801961e8), UINT32_C(0x801958d8),
                      220, BFM_WOODS_POLE_COUNT);
}

int bfm_mine_pole_load(BfmWoodsPoleLayout *layout, const uint8_t *scene, size_t bytes)
{
    return load_poles(layout, scene, bytes, UINT32_C(0x8018615c), UINT32_C(0x801860c8), 482, 1);
}

static int touches(const BfmWoodsPoleLayout *layout, unsigned row,
                   const int16_t prior[3], const int16_t feet[3])
{
    BfmForestGuardBox box;
    BfmForestTransform transform = {.rotation = {{4096,0,0}, {0,4096,0}, {0,0,4096}}};
    for (unsigned k = 0; k < 3; ++k) {
        transform.translation[k] = layout->origin[row][k];
        box.low[k] = layout->minimum[k];
        box.high[k] = layout->maximum[k];
    }
    int16_t center[3];
    return bfm_forest_guard_contact(&box, &transform, prior, feet, center);
}

void bfm_woods_pole_begin_field(BfmWoodsPoleGrip *grip)
{
    if (grip && grip->regrab) --grip->regrab;
}

int bfm_woods_pole_catch(const BfmWoodsPoleLayout *layout, BfmWoodsPoleGrip *grip,
                         const int16_t prior[3], int16_t feet[3])
{
    if (!layout || layout->count > BFM_WOODS_POLE_COUNT ||
        !grip || !prior || !feet || grip->pole || grip->regrab) return 0;
    /* After jumping off, the first eligible sweep still belongs to the old
     * pole. Its miss clears the selection for the following update. */
    unsigned first = 0, end = layout->count;
    if (grip->previous) {
        first = grip->previous - 1;
        end = grip->previous;
        grip->previous = 0;
        if (first >= layout->count) return 0;
    }
    for (unsigned row = first; row < end; ++row) {
        if (!touches(layout, row, prior, feet)) continue;
        feet[0] = layout->origin[row][0];
        feet[2] = layout->origin[row][2];
        grip->pole = row + 1;
        grip->settling = 15;
        return 1;
    }
    return 0;
}

BfmWoodsPoleMove bfm_woods_pole_step(const BfmWoodsPoleLayout *layout, BfmWoodsPoleGrip *grip,
    BfmFieldInput input, int interaction_target_active, int16_t feet[3], unsigned *yaw)
{
    if (!layout || !grip || !feet || !yaw || !grip->pole ||
        grip->pole > layout->count || layout->count > BFM_WOODS_POLE_COUNT) return BFM_POLE_FREE;
    if (input.cross_pressed && !interaction_target_active) {
        *grip = (BfmWoodsPoleGrip){.regrab = 10, .previous = grip->pole};
        return BFM_POLE_JUMP;
    }

    const int16_t prior[3] = {feet[0], feet[1], feet[2]};
    BfmWoodsPoleMove move = BFM_POLE_HOLD;
    int height = feet[1];
    if (grip->settling) {
        --grip->settling;
    } else if (input.y > 64) {
        move = BFM_POLE_DESCEND;
        height += 12;
        *yaw = (*yaw - 170u) & 4095u;
    } else {
        if (input.y < -64) {
            move = BFM_POLE_ASCEND;
            height -= 6;
        }
        if (input.x < -64) *yaw = (*yaw + 113u) & 4095u;
        if (input.x > 64) *yaw = (*yaw - 113u) & 4095u;
    }

    unsigned row = grip->pole - 1;
    if (height < INT16_MIN || height > INT16_MAX) {
        *grip = (BfmWoodsPoleGrip){0};
        return BFM_POLE_FALL;
    }
    feet[1] = (int16_t)height;
    if (!touches(layout, row, prior, feet)) {
        *grip = (BfmWoodsPoleGrip){0};
        return BFM_POLE_FALL;
    }
    feet[0] = layout->origin[row][0];
    feet[2] = layout->origin[row][2];
    return move;
}
