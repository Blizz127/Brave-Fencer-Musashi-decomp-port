/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_RASTER_H
#define BFM_FOREST_RASTER_H

#include <stdint.h>

typedef struct {
    const uint16_t (*tiles)[1024];
    uint32_t (*pixels)[320];
    int top, bottom; /* Inclusive screen rows. */
} BfmForestRaster;

typedef struct {
    int x, y;
    uint8_t u, v;
    uint32_t rgb; /* Lighting components, red in the low byte. */
} BfmForestRasterVertex;

typedef struct {
    uint16_t page, clut;
    unsigned count;
    int textured, translucent, dither;
} BfmForestRasterMaterial;

/* The forest ordering table provides either three corners or four strip corners.
 * For flat shading callers supply the same lighting value at every corner. */
void bfm_forest_raster_face(const BfmForestRaster *surface,
                            const BfmForestRasterMaterial *material,
                            const BfmForestRasterVertex corners[4]);

#endif
