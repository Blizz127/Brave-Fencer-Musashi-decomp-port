/* SPDX-License-Identifier: MIT */
#include "forest_guard_contact.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_bytes.h"
#include <math.h>

int bfm_forest_guard_box_load(const BfmForestField *field, BfmForestGuardBox *box)
{
    if (!field || !box) return 0;
    const uint8_t *record = bfm_forest_room_bytes(field, UINT32_C(0x8018ad90), 12);
    if (!record) return 0;
    const uint8_t *shape = bfm_forest_room_bytes(field, u32le(record + 8), 16);
    if (!shape || u32le(shape)) return 0;
    BfmForestGuardBox loaded;
    for (unsigned axis = 0; axis < 3; ++axis) {
        loaded.low[axis] = s16le(shape + 4 + 4 * axis);
        loaded.high[axis] = s16le(shape + 6 + 4 * axis);
        if (loaded.low[axis] > loaded.high[axis]) return 0;
    }
    *box = loaded;
    return 1;
}

/* Signed Q12 matrix multiplication discards low bits before translation. */
static int floor_div(int64_t value, int divisor)
{
    return (int)(value / divisor - (value % divisor < 0));
}

static void to_local(const BfmForestTransform *root, const int16_t world[3], int16_t local[3])
{
    for (unsigned axis = 0; axis < 3; ++axis) {
        int64_t dot = 0;
        for (unsigned row = 0; row < 3; ++row)
            dot += ((int64_t)world[row] - root->translation[row]) * root->rotation[row][axis];
        local[axis] = (int16_t)floor_div(dot, 4096);
    }
}

static int rotated_axis(const BfmForestTransform *root, const int16_t local[3], unsigned row)
{
    int64_t dot = 0;
    for (unsigned axis = 0; axis < 3; ++axis)
        dot += (int64_t)root->rotation[row][axis] * local[axis];
    return floor_div(dot, 4096);
}

/* Interior contacts point horizontally away from the box center. The
 * reciprocal uses the top four pairs of bits of the squared radius. */
static void radial_normal(const int16_t point[3], const int16_t center[3], int16_t normal[3])
{
    int dx = point[0] - center[0], dz = point[2] - center[2];
    uint32_t radius2 = (uint32_t)((int64_t)dx * dx + (int64_t)dz * dz);
    if (!radius2) return;
    int divisor = 8;
    while (radius2 < 64) {
        radius2 *= 4;
        divisor /= 2;
    }
    while (radius2 > 255) {
        radius2 >>= 2;
        divisor *= 2;
    }
    int gain = (int)sqrt((double)((UINT32_C(1) << 30) / radius2));
    normal[0] = (int16_t)floor_div((int64_t)dx * gain, divisor);
    normal[2] = (int16_t)floor_div((int64_t)dz * gain, divisor);
}

enum { CONTACT_POINT, SLIDE, SLIDE_ON_ENTRY };

static int resolve_segment(const BfmForestGuardBox *box, const int16_t start[3],
                           const int16_t end[3], int16_t hit[3], int16_t normal[3], int slide)
{
    int face[3];
    int outside[3];
    for (unsigned axis = 0; axis < 3; ++axis) {
        int below = start[axis] < box->low[axis], above = start[axis] > box->high[axis];
        if ((below && end[axis] < box->low[axis]) || (above && end[axis] > box->high[axis]))
            return 0;
        outside[axis] = below || above;
        face[axis] = below ? box->low[axis] : box->high[axis];
    }
    if (!outside[0] && !outside[1] && !outside[2]) {
        if (slide == SLIDE_ON_ENTRY) return 0;
        for (unsigned axis = 0; axis < 3; ++axis) hit[axis] = start[axis];
        return 1;
    }
    /* At an edge, the lower Z face precedes the X faces, then upper Z,
     * then either Y face. The accepted contact lies two units outward. */
    const unsigned first = start[2] < box->low[2] ? 2 : 0;
    const unsigned order[3] = { first, 2 - first, 1 };
    for (unsigned i = 0; i < 3; ++i) {
        unsigned axis = order[i];
        if (!outside[axis]) continue;
        int denominator = (int16_t)(end[axis] - start[axis]);
        if (!denominator) continue;
        unsigned coordinate = 0;
        for (; coordinate < 3; ++coordinate) {
            int delta = (int16_t)(end[coordinate] - start[coordinate]);
            int position = (int16_t)(start[coordinate] +
                (int64_t)delta * (face[axis] - start[axis]) / denominator);
            if (position < box->low[coordinate] || position > box->high[coordinate]) break;
            hit[coordinate] = (int16_t)position;
        }
        if (coordinate == 3) {
            /* For body movement, keep the intended travel along an accepted face. */
            if (slide) {
                for (unsigned tangent = 0; tangent < 3; ++tangent)
                    hit[tangent] = end[tangent];
            }
            int sign = start[axis] < box->low[axis] ? -1 : 1;
            hit[axis] = (int16_t)(face[axis] + sign * 2);
            normal[axis] = (int16_t)(sign * 4095);
            return 1;
        }
    }
    return 0;
}

static int world_contact(const BfmForestGuardBox *box, const BfmForestTransform *root,
                                   const int16_t start[3], const int16_t end[3],
                                   int16_t center[3], int16_t contact[3], int16_t normal[3], int slide)
{
    if (!box || !root || !start || !end || !center) return 0;
    int64_t dx = (int64_t)start[0] - root->translation[0];
    int64_t dz = (int64_t)start[2] - root->translation[2];
    if (dx < -512 || dx > 512 || dz < -512 || dz > 512 || dx * dx + dz * dz > 512 * 512)
        return 0;

    int16_t local_start[3], local_end[3], hit[3], midpoint[3], local_normal[3] = {0};
    to_local(root, start, local_start);
    to_local(root, end, local_end);
    if (!resolve_segment(box, local_start, local_end, hit, local_normal, slide)) return 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        int sum = box->low[axis] + box->high[axis];
        midpoint[axis] = sum / 2 - (sum % 2 < 0);
    }
    if (normal && !local_normal[0] && !local_normal[1] && !local_normal[2])
        radial_normal(hit, midpoint, local_normal);
    for (unsigned row = 0; row < 3; ++row) {
        center[row] = (int16_t)(rotated_axis(root, midpoint, row) + root->translation[row]);
        if (contact) contact[row] = (int16_t)(rotated_axis(root, hit, row) + root->translation[row]);
        if (normal) normal[row] = (int16_t)rotated_axis(root, local_normal, row);
    }
    return 1;
}

int bfm_forest_guard_contact_point(const BfmForestGuardBox *box, const BfmForestTransform *root,
                                   const int16_t start[3], const int16_t end[3],
                                   int16_t center[3], int16_t contact[3], int16_t normal[3])
{
    return world_contact(box, root, start, end, center, contact, normal, CONTACT_POINT);
}

int bfm_forest_body_slide(const BfmForestGuardBox *box, const BfmForestTransform *root,
                          const int16_t start[3], const int16_t end[3], int16_t contact[3])
{
    int16_t center[3];
    return world_contact(box, root, start, end, center, contact, NULL, SLIDE);
}

int bfm_forest_body_slide_on_entry(const BfmForestGuardBox *box, const BfmForestTransform *root,
                                  const int16_t start[3], const int16_t end[3], int16_t contact[3])
{
    int16_t center[3];
    return world_contact(box, root, start, end, center, contact, NULL, SLIDE_ON_ENTRY);
}

int bfm_forest_guard_contact(const BfmForestGuardBox *box, const BfmForestTransform *root,
                             const int16_t start[3], const int16_t end[3], int16_t center[3])
{
    return bfm_forest_guard_contact_point(box, root, start, end, center, NULL, NULL);
}
