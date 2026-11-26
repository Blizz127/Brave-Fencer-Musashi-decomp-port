/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_CAMERA_H
#define BFM_TOWER_CAMERA_H

#include "forest_terrain.h"

typedef struct {
    uint16_t yaw, projection;
} BfmTowerCamera;

/* Select the ordinary exterior camera from the mounted entrance record. */
int bfm_tower_camera_load(BfmTowerCamera *camera, const uint8_t *entrance, size_t size);

/* Advance the radial follow angle once per field update. */
void bfm_tower_camera_tick(BfmTowerCamera *camera, const int16_t player[3]);

/* The renderer supplies the player sample belonging to this submission. */
int bfm_tower_camera_view(const BfmTowerCamera *camera, const int16_t player[3],
                          BfmForestCamera *view, int32_t eye_out[3]);

#endif
