/* SPDX-License-Identifier: MIT */
#ifndef BFM_VALLEY_DEBRIS_ART_H
#define BFM_VALLEY_DEBRIS_ART_H

#include "forest_dust.h"

enum { BFM_VALLEY_DEBRIS_CELS = 8 };

typedef struct {
    int16_t left, top;
    uint8_t width, height, duration[BFM_VALLEY_DEBRIS_CELS];
    struct {
        uint16_t page;
        uint8_t u, v;
    } frame[BFM_VALLEY_DEBRIS_CELS];
} BfmValleyDebrisArt;

/* Read the sprite and its animation from the mounted Valley field. */
int bfm_valley_debris_art_load(BfmValleyDebrisArt *art, const uint8_t *scene, size_t bytes);

/* Gate smoke uses a random whole scale from two through five. */
int bfm_valley_debris_art_project_scaled(const BfmValleyDebrisArt *art, unsigned cel, unsigned scale,
    const int16_t xyz[3], const BfmForestCamera *camera, unsigned slot, BfmForestDustCard *out);

/* The hinge supplies each effect's world anchor. Each cel faces the camera. */
int bfm_valley_debris_art_project(const BfmValleyDebrisArt *art, unsigned cel,
    const int16_t xyz[3], const BfmForestCamera *camera, unsigned slot, BfmForestDustCard *out);

#endif
