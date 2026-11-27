/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_POLE_PICKUP_ART_H
#define BFM_WOODS_POLE_PICKUP_ART_H

#include "bedroom_figure.h"
#include "forest_damage_digits.h"
#include "woods_pole_pickup.h"

typedef struct {
    const uint8_t *colors;
    uint8_t number_palette;
    BfmForestDamageDigits numbers[BFM_WOODS_POLE_PICKUP_COUNT];
} BfmWoodsPolePickupArt;

/* The art borrows the mounted SC02/11 scene for the lifetime of this visit. */
int bfm_woods_pole_pickup_art_load(BfmWoodsPolePickupArt *art,
                                   const uint8_t *scene, size_t bytes);

/* Once per field update. Supply the hero's collection position and the face
 * value of the award, or zero to advance existing numbers without a pickup. */
void bfm_woods_pole_pickup_art_step(BfmWoodsPolePickupArt *art, const int16_t hero[3],
                                    unsigned amount);

/* Append to the field's prepared actor queue, using the shared field clock. */
int bfm_woods_pole_pickup_art_add(const BfmWoodsPolePickupArt *art,
                                  const BfmWoodsPolePickups *pickups, BfmBedroomFigure *figure,
                                  const BfmBedroomView *view, unsigned field_tick);

#endif
