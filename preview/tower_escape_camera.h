/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_ESCAPE_CAMERA_H
#define BFM_TOWER_ESCAPE_CAMERA_H

#include "forest_pose.h"
#include "forest_terrain.h"

typedef struct {
    int projection, distance, pitch, offset_y, offset_z;
    int center_x, upper_pitch, upper_offset_z;
    unsigned phase, woodland;
} BfmTowerEscapeCamera;

/* Advance once per field update. The initial eye/view are inherited from
 * the summit acquisition and held during the retreat from the pedestal. */
int bfm_tower_escape_lens(BfmTowerEscapeCamera *lens, unsigned phase, unsigned age,
                          const int16_t player[3], BfmForestCamera *view, int32_t eye[3]);

#endif
