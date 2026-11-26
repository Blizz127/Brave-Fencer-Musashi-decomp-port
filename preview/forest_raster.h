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

/* Projected terrain corners are relative to the viewport center. Validate
 * their submitted size before applying the frame buffer's drawing offset. */
void bfm_forest_raster_buffer_face(const BfmForestRaster *surface,
                                   const BfmForestRasterMaterial *material,
                                   const BfmForestRasterVertex corners[4], unsigned buffer);

/* For packets stored relative to a nonzero GPU drawing offset, supply the
 * submitted vertex coordinates. The offset result wraps to signed 11 bits. */
void bfm_forest_raster_offset_face(const BfmForestRaster *surface,
                                   const BfmForestRasterMaterial *material,
                                   const BfmForestRasterVertex corners[4],
                                   int offset_x, int offset_y);

/* Use the RGB555 expansion of the field renderer for objects in the town mesh. */
void bfm_field_raster_face(const BfmForestRaster *surface, const BfmForestRasterMaterial *material,
                           const BfmForestRasterVertex corners[4]);

#endif
