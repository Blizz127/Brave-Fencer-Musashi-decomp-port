/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_WATER_EFFECT_H
#define BFM_FOREST_WATER_EFFECT_H

#include "forest_dust.h"
#include "gameplay_run.h"

struct BfmForestField;

enum {
    BFM_FOREST_WATER_FLECKS = 3,
    BFM_FOREST_WATER_EFFECT_CARDS = 2 + 2 * BFM_FOREST_WATER_FLECKS
};

typedef struct {
    int16_t left, top;
    uint8_t width, height, u, v;
    BfmForestRasterMaterial material;
} BfmForestWaterSprite;

typedef struct {
    int32_t position[3];
    int16_t velocity[3];
} BfmForestWaterFleck;

typedef struct {
    BfmForestWaterSprite column, ring;
    BfmForestTransform column_root, ring_root, fleck_face;
    BfmForestWaterFleck flecks[BFM_FOREST_WATER_FLECKS];
    int height_scale, height_delta;
    unsigned age;
    int active;
} BfmForestWaterEffect;

/* Decode the two mounted sprite records. The terrain renderer owns the VRAM
 * texture pages and palettes; the decoded geometry has no borrowed pointers. */
int bfm_forest_water_effect_load(BfmForestWaterEffect *effect,
                                const struct BfmForestField *field);

/* The column and ring use two adjacent 12-byte mounted sprite records. */
int bfm_forest_water_effect_load_cells(BfmForestWaterEffect *effect,
                                      const uint8_t *cells, size_t bytes);

/* Emit at first water contact. The first projection uses the birth state.
 * Each subsequent simulation update advances the effect once. */
int bfm_forest_water_effect_begin(BfmForestWaterEffect *effect,
                                 const int16_t xyz[3], const int32_t eye[3],
                                 BfmGameplayRun *run);
void bfm_forest_water_effect_tick(BfmForestWaterEffect *effect);

/* Append in effect order. Clipped geometry uses no space in the output. */
size_t bfm_forest_water_effect_project(const BfmForestWaterEffect *effect,
                                      const BfmForestCamera *camera,
                                      BfmForestDustCard *out, size_t capacity);
#endif
