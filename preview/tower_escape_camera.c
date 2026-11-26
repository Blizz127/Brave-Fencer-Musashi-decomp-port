/* SPDX-License-Identifier: MIT */
#include "tower_escape_camera.h"
#include "tower_escape.h"

static int approach(int from, int to, int divisor)
{
    int delta = to - from;
    while (divisor > 1 && delta / divisor == 0) divisor /= 2;
    return from + delta / divisor;
}

static int32_t q12(int64_t value)
{
    return (int32_t)(value / 4096 - (value % 4096 < 0));
}

static void shift_held_view(BfmForestCamera *view, int32_t eye[3])
{
    if (eye[0] <= 1000) return;
    eye[0] -= 20;
    for (unsigned row = 0; row < 3; ++row) {
        int64_t offset = 0;
        for (unsigned axis = 0; axis < 3; ++axis)
            offset -= (int64_t)view->rotation[row][axis] * eye[axis];
        view->translation[row] = q12(offset);
    }
}

static void set_shot(BfmTowerEscapeCamera *lens, int projection, int distance,
                     int pitch, int y, int z)
{
    lens->projection = projection;
    lens->distance = distance;
    lens->pitch = pitch;
    lens->offset_y = y;
    lens->offset_z = z;
}

void bfm_tower_escape_lens_resume(BfmTowerEscapeCamera *lens, int lower, int center_x)
{
    *lens = (BfmTowerEscapeCamera){.center_x = center_x};
    if (lower) set_shot(lens, 300, 400, 288, -128, -200);
    else {
        set_shot(lens, 100, 400, 341, -128, 1024);
        lens->upper_rise = 1;
    }
}

static int orbit(BfmTowerEscapeCamera *lens, const int16_t player[3],
                 BfmForestCamera *view, int32_t eye[3])
{
    int y = player[1], z = player[2];
    if (z >= -3327 && y > -384) y = -384;
    if (z < -6528) z = -6528;
    int32_t *target = lens->target;
    target[0] = lens->center_x;
    target[1] = y + lens->offset_y;
    target[2] = z + lens->offset_z;
    BfmForestTransform arm;
    bfm_forest_actor_pitch_yaw((unsigned)lens->pitch & 4095, 2048, player, &arm);
    for (unsigned axis = 0; axis < 3; ++axis)
        eye[axis] = target[axis] + q12(lens->distance * arm.rotation[axis][2]);
    return bfm_forest_camera_look_at(eye, target, (uint16_t)lens->projection, view);
}

int bfm_tower_escape_lens(BfmTowerEscapeCamera *lens, unsigned phase, unsigned age,
                          const int16_t player[3], BfmForestCamera *view, int32_t eye[3])
{
    if (!lens || !player || !view || !eye || phase == BFM_ESCAPE_IDLE) return 0;
    if (phase < BFM_ESCAPE_REVEAL) return view->projection != 0;
    /* The head's collapse timer expires partway through the hero's separate
     * reveal. The preceding pan keeps the original shot's orientation. */
    if (phase == BFM_ESCAPE_REVEAL && age < 19) {
        if (age) shift_held_view(view, eye);
        return view->projection != 0;
    }
    if (phase == BFM_ESCAPE_REVEAL) {
        if (lens->phase != phase) {
            set_shot(lens, 100, 400, 341, -128, 1024);
            lens->center_x = player[0];
        }
    } else if (phase == BFM_ESCAPE_UPPER) {
        if (lens->phase != phase) {
            lens->upper_pitch = 341;
            lens->upper_offset_z = 200;
            lens->center_x = player[0];
        }
        if (player[2] < 10752) {
            lens->upper_pitch += 34;
            if (lens->upper_pitch > 910) lens->upper_pitch = 910;
            lens->upper_offset_z += 10;
            if (lens->upper_offset_z > 0) lens->upper_offset_z = 0;
            lens->pitch = lens->upper_pitch;
            lens->offset_z = lens->upper_offset_z;
        } else if (!lens->upper_rise || age >= 44) {
            lens->offset_z = approach(lens->offset_z, lens->upper_offset_z, 16);
        }
    } else if (phase == BFM_ESCAPE_DROP && lens->phase != phase) {
        set_shot(lens, 300, 400, 128, -128, -200);
        lens->center_x = player[0];
    } else if (phase == BFM_ESCAPE_CAUGHT && lens->phase == BFM_ESCAPE_UPPER) {
        return orbit(lens, player, view, eye);
    } else if (phase >= BFM_ESCAPE_LOWER) {
        if (!lens->woodland && player[2] < 4992) {
            lens->woodland = 1;
            lens->center_x = player[0];
        }
        if (player[2] < -5888) lens->center_x = approach(lens->center_x, 0, 10);
        int woods = lens->woodland != 0;
        int elevated = !woods && player[2] < 5888;
        lens->projection = approach(lens->projection, woods ? 150 : 300, 4);
        lens->distance = approach(lens->distance, woods ? 100 : 80, 4);
        lens->pitch = approach(lens->pitch, woods ? 288 : elevated ? 512 : 113, 4);
        lens->offset_y = approach(lens->offset_y, woods ? -96 : elevated ? -448 : -128, 16);
    }
    lens->phase = phase;
    return orbit(lens, player, view, eye);
}
