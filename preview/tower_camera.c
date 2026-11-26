/* SPDX-License-Identifier: MIT */
#include "tower_camera.h"
#include "forest_shore_motion.h"
#include "../pc_port/forest_player_bytes.h"
#include <math.h>

enum { ONE = 4096, FOCAL = 500, ORBIT = 700, PITCH = 56, PLATE_PITCH = 910, FOCUS_Y = -140 };
static const double angle_unit = 6.28318530717958647693 / ONE;

int bfm_tower_camera_load(BfmTowerCamera *camera, const uint8_t *entrance, size_t size)
{
    if (!camera || !entrance || size < 0x1c || u32le(entrance) != 0x3009 ||
        u32le(entrance + 0x18) != UINT32_C(0x8017e394)) return 0;
    camera->yaw = (uint16_t)bfm_forest_shore_bearing(s16le(entrance + 0x10),
                                                     s16le(entrance + 0x14));
    camera->projection = FOCAL;
    camera->pitch = camera->desired_pitch = PITCH;
    return 1;
}

void bfm_tower_camera_plate_hint(BfmTowerCamera *camera, int active)
{
    if (camera) camera->desired_pitch = active ? PLATE_PITCH : PITCH;
}

static uint16_t approach_angle(unsigned current, unsigned target)
{
    int delta = (int)target - (int)current;
    if (delta > ONE / 2) delta -= ONE;
    if (delta < -ONE / 2) delta += ONE;
    int step = delta / 4;
    if (!step) step = delta / 2;
    if (!step) step = delta;
    return (uint16_t)((current + step) & (ONE - 1));
}

void bfm_tower_camera_tick(BfmTowerCamera *camera, const int16_t player[3])
{
    if (!camera || !camera->projection || !player) return;
    camera->yaw = approach_angle(camera->yaw, bfm_forest_shore_bearing(player[0], player[2]));
    camera->pitch = approach_angle(camera->pitch, camera->desired_pitch);
}

static int floor_div(int value, int divisor)
{
    return value / divisor - (value % divisor < 0);
}

/* The field reduces squared length in powers of four before normalization. */
static void normalize(int axis[3])
{
    unsigned square = (unsigned)(axis[0]*axis[0] + axis[1]*axis[1] + axis[2]*axis[2]);
    int divisor = 8;
    while (square > 255) { square /= 4; divisor *= 2; }
    int gain = (int)sqrt((double)((UINT32_C(1) << 30) / square));
    for (unsigned i = 0; i < 3; ++i) axis[i] = floor_div(axis[i] * gain, divisor);
}

int bfm_tower_camera_view(const BfmTowerCamera *camera, const int16_t player[3],
                          BfmForestCamera *view, int32_t eye_out[3])
{
    if (!camera || !camera->projection || !player || !view) return 0;
    double yaw = camera->yaw * angle_unit;
    int axis[3] = {(int)lround(sin(yaw)*ONE), 0, (int)lround(cos(yaw)*ONE)};
    normalize(axis);
    int horizontal = (int)lround(cos(camera->pitch * angle_unit) * ONE);
    axis[0] = floor_div(axis[0] * horizontal, ONE);
    axis[1] = -(int)lround(sin(camera->pitch * angle_unit) * ONE);
    axis[2] = floor_div(axis[2] * horizontal, ONE);
    normalize(axis);
    int32_t target[3] = {player[0], player[1] + FOCUS_Y, player[2]}, eye[3];
    for (unsigned i = 0; i < 3; ++i) eye[i] = target[i] + floor_div(axis[i] * ORBIT, ONE);
    if (!bfm_forest_camera_look_at(eye, target, camera->projection, view)) return 0;
    if (eye_out)
        for (unsigned i=0; i<3; ++i) eye_out[i]=eye[i];
    return 1;
}
