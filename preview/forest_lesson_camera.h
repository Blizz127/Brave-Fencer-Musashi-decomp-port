/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_LESSON_CAMERA_H
#define BFM_FOREST_LESSON_CAMERA_H
#include "forest_terrain.h"

typedef struct {
    int rest, distance, wanted;
    int lead[2], wanted_lead[2];
    unsigned pull_ticks, pan_ticks;
    int soldier_faced;
} BfmForestLessonCamera;

typedef enum {
    BFM_FOREST_LENS_HOLD,
    BFM_FOREST_LENS_CLIFF,
    BFM_FOREST_LENS_RETURN,
    BFM_FOREST_LENS_SOLDIER,
    BFM_FOREST_LENS_CENTER
} BfmForestLensCue;

/* Suspend heading-driven lead when the scene takes the camera.
 * Actual and desired X/Z offsets belong to the active field camera. */
void bfm_forest_lesson_camera_begin(BfmForestLessonCamera *lens, int distance,
                                    int x, int z, int wanted_x, int wanted_z);
/* One effective field update. The discovery turn includes the update that
 * starts already facing its goal. Input yaw is from before the actor update. */
void bfm_forest_lesson_camera_tick(BfmForestLessonCamera *lens, BfmForestLensCue cue,
                                   int incoming_yaw);
/* Use the ordinary field's normalized orbit axis, projection and player.
 * On release return lead[0] and lead[1] to the normal heading tracker. */
int bfm_forest_lesson_camera_view(const BfmForestLessonCamera *lens, const int16_t position[3],
                                  const int orbit_q12[3], uint16_t projection,
                                  int32_t eye[3], int32_t target[3], BfmForestCamera *view);
#endif
