/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MAYOR_CAMERA_H
#define BFM_VILLAGE_MAYOR_CAMERA_H

#include "village_talk_camera.h"

typedef struct {
    BfmVillageTalkCamera orbit;
    unsigned release_delay;
} BfmVillageMayorCamera;

/* The scene is the lower village field. The mayor member is its decoded
 * encounter payload with the PAC header removed. */
int bfm_village_mayor_camera_begin(BfmVillageMayorCamera *camera,
                                   const uint8_t *scene, size_t bytes,
                                   const uint8_t *mayor, size_t mayor_bytes,
                                   const BfmVillageCamera *normal);

/* Both clocks count completed 30 Hz intervals. Elapsed is zero on the
 * accepted porch interaction. Release age is zero when the final box clears. */
void bfm_village_mayor_camera_step(BfmVillageMayorCamera *camera, unsigned elapsed,
                                  unsigned release_age, int returning,
                                  BfmVillageCamera *normal);
#endif
