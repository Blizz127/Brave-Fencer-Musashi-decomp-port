/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_DUST_H
#define BFM_FOREST_DUST_H

#include "forest_raster.h"
#include "forest_terrain.h"
#include "musashi_disc_media.h"

enum { BFM_FOREST_DUST_CELLS = 10, BFM_FOREST_FOOT_DUST_CARDS = 9,
       BFM_FOREST_DUST_CARDS = BFM_FOREST_FOOT_DUST_CARDS + 1,
       BFM_FOREST_IMPACT_CARDS = 2, BFM_FOREST_SWEEP_CARDS = 15,
       BFM_FOREST_EFFECT_CARDS = BFM_FOREST_DUST_CARDS + 1 + BFM_FOREST_IMPACT_CARDS + BFM_FOREST_SWEEP_CARDS };

typedef enum {
    BFM_FOREST_DUST_NONE,
    BFM_FOREST_DUST_BRAKE,
    BFM_FOREST_DUST_TRAIL
} BfmForestDustEmission;

typedef struct {
    int64_t position[3];
    int32_t velocity[3];
    unsigned cell;
} BfmForestDustParticle;

typedef struct {
    BfmForestDustParticle particles[BFM_FOREST_DUST_CARDS];
    uint16_t page[BFM_FOREST_DUST_CELLS];
    uint8_t u[BFM_FOREST_DUST_CELLS], v[BFM_FOREST_DUST_CELLS];
    int16_t left, top;
    uint8_t width, height;
} BfmForestDust;

typedef struct BfmForestDustCard {
    BfmForestRasterVertex vertices[4];
    BfmForestRasterMaterial material;
    unsigned bucket, slot;
} BfmForestDustCard;

int bfm_forest_dust_load(BfmForestDust *dust, const MusashiDiscMedia *disc);

/* Call once per actor update, after resolving the player's world position.
 * The camera eye and yaw belong to the update that requests the effect. */
void bfm_forest_dust_tick(BfmForestDust *dust, BfmForestDustEmission event,
                          const int16_t player[3], const int32_t eye[3], int yaw);

/* Guard recoil starts on the struck actor and advances on subsequent updates. */
void bfm_forest_dust_recoil_start(BfmForestDust *dust, const int32_t root[3], int attacker_yaw);
void bfm_forest_dust_recoil_advance(BfmForestDust *dust);

/* Output uses the field's ordering buckets and ascending particle slots. */
unsigned bfm_forest_dust_project(const BfmForestDust *dust, const BfmForestCamera *camera,
                                 BfmForestDustCard out[BFM_FOREST_FOOT_DUST_CARDS]);
unsigned bfm_forest_dust_recoil_project(const BfmForestDust *dust, const BfmForestCamera *camera,
                                        BfmForestDustCard *out);
#endif
