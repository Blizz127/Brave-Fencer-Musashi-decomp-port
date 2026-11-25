/* SPDX-License-Identifier: MIT */
#include "forest_pose.h"
#include "../pc_port/forest_player_archive.h"

#include <math.h>

enum { FULL_TURN = 4096, PACKED_POSE = 240 };

static int multiply(int a, int b)
{
    int32_t result = a * b;
    return result / FULL_TURN - (result % FULL_TURN < 0);
}

static void axis_angle(unsigned units, int *s, int *c)
{
    const double radians = 6.28318530717958647693 / FULL_TURN;
    unsigned quadrant = (units / 1024) & 3, phase = units & 1023;
    /* Pose packing mirrors two 1024-entry quarter waves, including their
     * shared endpoints. This affects near-axis bones even in an idle pose. */
    int direct = (int)lround(FULL_TURN * sin(phase * radians));
    int mirror = (int)lround(FULL_TURN * sin((1023-phase) * radians));
    *s = (quadrant & 1) ? mirror : direct;
    *c = (quadrant & 1) ? direct : mirror;
    if (quadrant >= 2) *s = -*s;
    if (quadrant == 1 || quadrant == 2) *c = -*c;
}

static void local_rotation(const uint8_t *angles, int16_t r[3][3])
{
    int sx, cx, sy, cy, sz, cz;
    axis_angle(angles[6] | (unsigned)angles[7] << 8, &sx, &cx);
    axis_angle(((angles[0] & 15u) << 8) | angles[1], &sy, &cy);
    axis_angle(((angles[0] & 240u) << 4) | angles[2], &sz, &cz);
    /* Euler angles rotate a model vector about X, Y, then Z. The signed
     * products are rounded down to Q12 between successive terms. */
    int xz = multiply(cx, sz), zz = multiply(cx, cz), xy = multiply(sx, sy);
    r[0][0] = (int16_t)multiply(cz, cy);
    r[1][0] = (int16_t)multiply(sz, cy);
    r[2][0] = (int16_t)-sy;
    r[0][1] = (int16_t)(multiply(xy, cz) - xz);
    r[1][1] = (int16_t)(multiply(xy, sz) + zz);
    r[2][1] = (int16_t)multiply(cy, sx);
    r[0][2] = (int16_t)(multiply(zz, sy) + multiply(sx, sz));
    r[1][2] = (int16_t)(multiply(xz, sy) - multiply(sx, cz));
    r[2][2] = (int16_t)multiply(cy, cx);
}

static int turn_root(int16_t r[3][3])
{
    for (unsigned col = 0; col < 3; ++col) {
        int squared = 0;
        for (unsigned row = 0; row < 3; ++row)
            squared += r[row][col] * r[row][col];
        if (squared < 0xff0000 || squared >= 0x1040000)
            return 0;
        int step = squared < 0x1000000 ? 0x10000 : 0x40000;
        unsigned length = (unsigned)(squared / step * step);
        int gain = (int)sqrt(281474976710656.0 / length);
        for (unsigned row = 0; row < 3; ++row)
            r[row][col] = (int16_t)multiply((col == 1 ? 1 : -1) * r[row][col], gain);
    }
    return 1;
}

int bfm_forest_pose_load(const MusashiDiscMedia *disc, uint32_t frame_address,
                         BfmForestPose *pose)
{
    ForestPlayerArchive archive;
    uint8_t bytes[PACKED_POSE];
    if (!disc || !pose || !forest_player_archive_open(disc, &archive) ||
        !forest_player_archive_read(&archive, frame_address, bytes, sizeof(bytes)))
        return 0;
    return bfm_forest_pose_decode(bytes, sizeof bytes, pose);
}

int bfm_forest_pose_decode(const uint8_t *bytes, size_t size, BfmForestPose *pose)
{
    if (!bytes || size < PACKED_POSE || !pose)
        return 0;
    BfmForestPose decoded;
    for (unsigned i = 0; i < MUSASHI_FOREST_PLAYER_PARTS; ++i) {
        const uint8_t *record = bytes + (i ? (i-1)*8 : 0);
        const uint8_t *xyz = record + (i ? 3 : 0);
        const uint8_t *angles = i < 2 ? bytes + 24 : record;
        BfmForestTransform *part = &decoded.local[i];
        for (unsigned k = 0; k < 3; ++k)
            part->translation[k] = (int8_t)xyz[k];
        local_rotation(angles, part->rotation);
        if (i == 0 && !turn_root(part->rotation))
            return 0;
    }
    *pose = decoded;
    return 1;
}
