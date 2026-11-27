/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_SUMMIT_CAMERA_H
#define BFM_TOWER_SUMMIT_CAMERA_H

#include "forest_terrain.h"

/* Summit entrance 0: the camera follows the player's position on the round
 * platform. The eye also provides the orientation of the sky panorama. */
void bfm_tower_summit_camera_target(const int16_t player[3], int32_t target[3]);
int bfm_tower_summit_camera(const int16_t player[3], BfmForestCamera *camera,
                            int32_t eye[3]);

#endif
