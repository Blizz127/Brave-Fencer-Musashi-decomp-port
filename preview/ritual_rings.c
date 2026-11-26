/* SPDX-License-Identifier: MIT */
#include "ritual_light.h"
#include <math.h>
#include <string.h>

/* Two tilted hoops cross above the floor ring. Each hoop has a black
 * inner rim, so adding its shade reveals the room through its center. */
typedef struct { int axis[3][3]; } Rotation;

static int floor_ratio(int64_t value, int divisor)
{
    return (int)(value / divisor) - (value % divisor < 0);
}

static int sine(int angle)
{
    return (int)lround(4096 * sin(angle * (6.2831853071795864769 / 4096)));
}

static Rotation rotation(unsigned axis, int angle)
{
    Rotation result = {{{4096,0,0}, {0,4096,0}, {0,0,4096}}};
    unsigned a = (axis + 1) % 3, b = (axis + 2) % 3;
    int c = sine(angle + 1024), s = sine(angle);
    result.axis[a][a] = result.axis[b][b] = c;
    result.axis[a][b] = -s;
    result.axis[b][a] = s;
    return result;
}

static Rotation compose(Rotation left, Rotation right)
{
    Rotation result;
    for (unsigned row = 0; row < 3; ++row) {
        for (unsigned column = 0; column < 3; ++column) {
            int sum = 0;
            for (unsigned k = 0; k < 3; ++k)
                sum += left.axis[row][k] * right.axis[k][column];
            result.axis[row][column] = floor_ratio(sum + 2048, 4096);
        }
    }
    return result;
}

static int perspective_scale(int z)
{
    if (z <= 128) return 131071;
    return (256 * 65536 + z / 2) / z;
}

static void project(const int view[3], int screen[3])
{
    int scale = perspective_scale(view[2]);
    screen[0] = 160 + floor_ratio((int64_t)view[0] * scale, 65536);
    screen[1] = 120 + floor_ratio((int64_t)view[1] * scale, 65536);
    screen[2] = view[2];
}

static void view_center(const int camera[3][3], const int translation[3],
                        int y, int z, int center[3])
{
    for (unsigned row = 0; row < 3; ++row)
        center[row] = translation[row] +
                      floor_ratio(camera[row][1] * y + camera[row][2] * z, 4096);
}

/* All three light shapes use the same reflected 12-point rim. */
static void fan_rim(unsigned vertex, int radius, int *x, int *z)
{
    unsigned step = vertex <= 6 ? vertex : 12 - vertex;
    if (step > 3) step = 6 - step;
    int angle = (int)step * 341;
    int horizontal = radius * sine(angle) / 4096;
    int vertical = radius * sine(1024 + angle) / 4096;
    *x = vertex <= 6 ? -horizontal : horizontal;
    *z = vertex >= 3 && vertex <= 9 ? -vertical : vertical;
}

static void crossed_hoops(int age, Rotation view, const int center[3],
                          const unsigned tint[3], BfmRitualEmit emit, void *context)
{
    int radius = 16 + 40 * age;
    int anchor[3];
    unsigned rgb[4][3] = {{0}};
    memcpy(rgb[0], tint, sizeof(rgb[0]));
    memcpy(rgb[1], tint, sizeof(rgb[1]));
    project(center, anchor);

    for (unsigned side = 0; side < 2; ++side) {
        Rotation tilted = compose(view, rotation(2, side ? 384 : -384));
        Rotation spun = compose(tilted, rotation(1, -16 * age));
        int outer[12][3], inner[12][3];
        for (unsigned k = 0; k < 12; ++k) {
            int x, z, point[3];
            fan_rim(k, radius, &x, &z);
            for (unsigned row = 0; row < 3; ++row)
                point[row] = center[row] +
                             floor_ratio(spun.axis[row][0] * x + spun.axis[row][2] * z, 4096);
            project(point, outer[k]);
            for (unsigned axis = 0; axis < 2; ++axis)
                inner[k][axis] = floor_ratio(3 * outer[k][axis] + anchor[axis], 4);
            inner[k][2] = outer[k][2];
        }
        for (unsigned k = 0; k < 12; ++k) {
            unsigned next = (k + 1) % 12;
            int xyz[4][3];
            memcpy(xyz[0], outer[next], sizeof(xyz[0]));
            memcpy(xyz[1], outer[k], sizeof(xyz[1]));
            memcpy(xyz[2], inner[next], sizeof(xyz[2]));
            memcpy(xyz[3], inner[k], sizeof(xyz[3]));
            emit(context, 4, xyz, rgb, center[2] / 4 + 1);
        }
    }
}

static void emit_fan(const int rim[12][3], const int center[3],
                     const unsigned outer[3], const unsigned core[3],
                     BfmRitualEmit emit, void *context)
{
    unsigned rgb[4][3];
    for (unsigned i = 0; i < 4; ++i)
        memcpy(rgb[i], i == 2 ? core : outer, sizeof(rgb[i]));
    for (unsigned k = 0; k < 12; k += 2) {
        int xyz[4][3];
        memcpy(xyz[0], rim[(k + 2) % 12], sizeof(xyz[0]));
        memcpy(xyz[1], rim[k + 1], sizeof(xyz[1]));
        memcpy(xyz[2], center, sizeof(xyz[2]));
        memcpy(xyz[3], rim[k], sizeof(xyz[3]));
        emit(context, 4, xyz, rgb, center[2] / 4 + 1);
    }
}

static void floor_ring(int age, const int camera[3][3], const int translation[3],
                       const unsigned tint[3], BfmRitualEmit emit, void *context)
{
    int center[3], anchor[3], rim[12][3];
    view_center(camera, translation, -658, 0, center);
    project(center, anchor);
    for (unsigned k = 0; k < 12; ++k) {
        int x, z, point[3];
        fan_rim(k, 16 + age * 16, &x, &z);
        for (unsigned row = 0; row < 3; ++row)
            point[row] = center[row] +
                         floor_ratio(camera[row][0] * x + camera[row][2] * z, 4096);
        project(point, rim[k]);
    }
    unsigned outer[3], dimming = (unsigned)age * 8;
    const unsigned black[3] = {0};
    for (unsigned i = 0; i < 3; ++i)
        outer[i] = tint[i] > dimming ? tint[i] - dimming : 0;
    emit_fan(rim, anchor, outer, black, emit, context);
}

static void approaching_glow(int age, const int camera[3][3], const int translation[3],
                             const unsigned tint[3], BfmRitualEmit emit, void *context)
{
    int travel = age < 11 ? age : 11;
    int radius = 16 + 32 * (age < 12 ? age : 12);
    int center[3], anchor[3], rim[12][3];
    view_center(camera, translation, -779, -48 - 32 * travel, center);
    project(center, anchor);
    int scale = perspective_scale(center[2]);
    for (unsigned k = 0; k < 12; ++k) {
        int x, y;
        fan_rim(k, radius, &x, &y);
        rim[k][0] = anchor[0] + floor_ratio((int64_t)x * scale, 65536);
        rim[k][1] = anchor[1] + floor_ratio((int64_t)y * scale, 65536);
        rim[k][2] = anchor[2];
    }
    unsigned outer[3], core[3];
    for (unsigned i = 0; i < 3; ++i) {
        outer[i] = (unsigned)age * 8;
        unsigned sum = outer[i] + tint[i];
        core[i] = sum < 255 ? sum : 255;
    }
    emit_fan(rim, anchor, outer, core, emit, context);
}

void bfm_ritual_rings(unsigned field, const int camera[3][3], const int translation[3],
                      const unsigned tint[3], BfmRitualEmit emit, void *context)
{
    if (field < 4568 || field >= 4632) return;
    int age = (int)((field - 4568) / 2);
    if (age < 20) {
        int center[3];
        Rotation view;
        memcpy(view.axis, camera, sizeof(view.axis));
        view_center(camera, translation, -745, 0, center);
        if (age >= 8) crossed_hoops(age - 7, view, center, tint, emit, context);
        if (age <= 12) crossed_hoops(age, view, center, tint, emit, context);
    }
    floor_ring(age, camera, translation, tint, emit, context);
    /* On the first draw tick the centered regular halo is still visible. */
    if (age) approaching_glow(age, camera, translation, tint, emit, context);
}
