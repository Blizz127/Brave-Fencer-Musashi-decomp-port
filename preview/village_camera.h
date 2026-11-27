/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_CAMERA_H
#define BFM_VILLAGE_CAMERA_H

#include <stdint.h>

typedef struct {
    int eye[3], target[3], focal;
    int radius, yaw, following, woods_pitch;
} BfmVillageCamera;

typedef struct {
    int goal, direction, held_ticks;
} BfmVillageCameraTurn;

/* The descending cable entrance prepares the initial view before the first
 * field update. The player coordinates are in room space. */
void bfm_village_camera_begin(BfmVillageCamera *camera, const int16_t xyz[3]);

/* Call once per 30 Hz update, after player movement. The room's handoff
 * event switches the camera from the cable to the player. */
void bfm_village_camera_step(BfmVillageCamera *camera, const int16_t xyz[3],
                             int follow_event);

/* Normal follow view on the northern ramp and the lower village street.
 * Start at the transferred player position, then update once per world tick. */
void bfm_village_camera_lower(BfmVillageCamera *camera, const int16_t xyz[3],
                              int entering);
/* Seed turn->goal with the entry yaw. One field update consumes the held
 * shoulder direction (-1 for L2, +1 for R2), then lower places the eye. */
void bfm_village_camera_turn(BfmVillageCamera *camera, BfmVillageCameraTurn *turn,
                             int direction);

/* Step the trail camera after movement. The zone uses the player's Z even
 * while the camera target is still catching up. */
void bfm_village_camera_woods_orbit(BfmVillageCamera *camera, int bearing,
                                    int player_z, int entering);

#endif
