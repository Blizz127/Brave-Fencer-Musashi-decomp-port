/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_SHOT_FLASH_H
#define BFM_FOREST_SHOT_FLASH_H

#include "forest_dust.h"

typedef enum {
    BFM_SHOT_FLASH_EXPIRY,
    BFM_SHOT_FLASH_CONTACT
} BfmForestShotFlashKind;

typedef struct {
    int16_t x, z;
    uint32_t height_q16;
    int32_t velocity_q16;
    unsigned cell;
} BfmForestShotFlash;

/* Contact uses the final spark origin and the camera eye at birth. A shot
 * that expires in flight uses its own final position and needs no eye.
 * Advance existing effects before starting new ones on the same field tick. */
int bfm_forest_shot_flash_start(BfmForestShotFlash *flash, BfmForestShotFlashKind kind,
                                const int16_t origin[3], const int32_t eye[3]);
void bfm_forest_shot_flash_advance(BfmForestShotFlash *flash);

/* Reads the shared atlas loaded from the mounted player archive. */
int bfm_forest_shot_flash_project(const BfmForestShotFlash *flash, const BfmForestDust *atlas,
                                  const BfmForestCamera *camera, BfmForestDustCard *out);

#endif
