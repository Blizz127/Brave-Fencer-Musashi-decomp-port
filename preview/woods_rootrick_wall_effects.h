/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_ROOTRICK_WALL_EFFECTS_H
#define BFM_WOODS_ROOTRICK_WALL_EFFECTS_H

#include "bedroom_figure.h"
#include "gameplay_run.h"

enum { BFM_ROOTRICK_WALL_CHIPS = 30 };

typedef struct {
    uint32_t position[3];
    int32_t velocity[3];
    unsigned life, variant;
} BfmWoodsWallChip;

typedef struct {
    const uint8_t *sprite;
    BfmWoodsWallChip chips[BFM_ROOTRICK_WALL_CHIPS];
} BfmWoodsRootrickWallArt;

/* The borrowed SC02/11 payload must outlive the effect pool. */
int bfm_woods_rootrick_wall_art_load(BfmWoodsRootrickWallArt *art,
                                     const uint8_t *scene, size_t bytes);

/* Once per field following both actors' movement. Pass the issued hero site
 * (already 20 above his root) only on attachment or a hand contact, and the
 * two transformed rival sites only on their own emission. Orientations are the unscaled model rotations at emission.
 * Continue updating surviving chips when all emitters are idle. */
int bfm_woods_rootrick_wall_art_step(BfmWoodsRootrickWallArt *art, BfmGameplayRun *run,
                                     const int16_t hero_site[3], const BfmForestTransform *hero_root,
                                     const int16_t sites[][3], unsigned site_count,
                                     const BfmForestTransform *rival_root);

/* Append to the field's prepared actor queue before the room drains it. */
int bfm_woods_rootrick_wall_art_add(const BfmWoodsRootrickWallArt *art,
                                    BfmBedroomFigure *figure, const BfmBedroomView *view);

#endif
