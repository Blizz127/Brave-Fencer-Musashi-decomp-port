/* SPDX-License-Identifier: MIT */
#include "forest_lesson_camera.h"
#include <limits.h>

void bfm_forest_lesson_camera_begin(BfmForestLessonCamera *lens, int distance,
                                    int x, int z, int wanted_x, int wanted_z)
{
    *lens = (BfmForestLessonCamera){
        .rest = distance, .distance = distance, .wanted = distance,
        .lead = {x, z}, .wanted_lead = {wanted_x, wanted_z}
    };
}

static int approach(int now, int goal, int divisor)
{
    int delta = goal - now, step = delta / divisor;
    if (!step && delta) step = delta > 0 ? 1 : -1;
    return now + step;
}

void bfm_forest_lesson_camera_tick(BfmForestLessonCamera *lens, BfmForestLensCue cue,
                                   int incoming_yaw)
{
    if (cue == BFM_FOREST_LENS_CLIFF && lens->pull_ticks < 32) {
        ++lens->pull_ticks;
        lens->distance += 8;
        lens->wanted = lens->distance;
    } else if (cue == BFM_FOREST_LENS_RETURN) {
        lens->wanted = lens->rest;
    } else if (cue == BFM_FOREST_LENS_SOLDIER && !lens->soldier_faced &&
               lens->pan_ticks < 32) {
        ++lens->pan_ticks;
        for (unsigned i = 0; i < 2; ++i) {
            lens->lead[i] -= 4;
            lens->wanted_lead[i] = lens->lead[i];
        }
        if ((incoming_yaw & 4095) == 1024) lens->soldier_faced = 1;
    } else if (cue == BFM_FOREST_LENS_CENTER) {
        lens->wanted_lead[0] = lens->wanted_lead[1] = 0;
    }
    lens->distance = approach(lens->distance, lens->wanted, 4);
    for (unsigned i = 0; i < 2; ++i)
        lens->lead[i] = approach(lens->lead[i], lens->wanted_lead[i], 16);
}

int bfm_forest_lesson_camera_view(const BfmForestLessonCamera *lens, const int16_t position[3],
                                  const int orbit_q12[3], uint16_t projection,
                                  int32_t eye[3], int32_t target[3], BfmForestCamera *view)
{
    if (!lens || !position || !orbit_q12 || !eye || !target || !view ||
        !projection || lens->distance <= 0 || lens->distance > INT16_MAX) return 0;
    target[0] = position[0] + lens->lead[0];
    target[1] = position[1];
    target[2] = position[2] + lens->lead[1];
    for (unsigned i = 0; i < 3; ++i) {
        int64_t offset = (int64_t)orbit_q12[i] * lens->distance;
        eye[i] = target[i] + (int32_t)(offset / 4096 - (offset % 4096 < 0));
    }
    return bfm_forest_camera_look_at(eye, target, projection, view);
}
