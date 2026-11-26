/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_CAMERA_H
#define BFM_TOWER_CAMERA_H

#include "forest_terrain.h"

typedef struct {
    uint16_t yaw, projection, pitch, desired_pitch;
    int orbit, eye_offset[3];
} BfmTowerCamera;

/* Select the ordinary exterior camera from the mounted entrance record. */
int bfm_tower_camera_load(BfmTowerCamera *camera, const uint8_t *entrance, size_t size);

/* While the first exposed plate is being explained, the radial camera
 * pitches overhead. Closing the message restores its ordinary orbit. */
void bfm_tower_camera_plate_hint(BfmTowerCamera *camera, int active);
/* Advance the orbital angles once per field update. */
void bfm_tower_camera_tick(BfmTowerCamera *camera, const int16_t player[3]);

/* The renderer supplies the player sample belonging to this submission. */
void bfm_tower_camera_rise_tick(BfmTowerCamera *camera, unsigned age, int facing);
int bfm_tower_camera_rise_view(const BfmTowerCamera *camera, const int16_t player[3],
                               unsigned age, BfmForestCamera *view, int32_t eye_out[3]);
int bfm_tower_camera_view(const BfmTowerCamera *camera, const int16_t player[3],
                          BfmForestCamera *view, int32_t eye_out[3]);

#endif
