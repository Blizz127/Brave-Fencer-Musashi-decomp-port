/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_LILY_H
#define BFM_WOODS_LILY_H

#include "woods_jump_pose.h"
#include "woods_lily_art.h"
#include "bedroom_travel.h"

typedef struct BfmWoodsLilies BfmWoodsLilies;

BfmWoodsLilies *bfm_woods_lilies_open(const MusashiDiscMedia *disc, const uint8_t *scene,
                                      size_t bytes);
void bfm_woods_lilies_free(BfmWoodsLilies *lilies);
int bfm_woods_lilies_add(const BfmWoodsLilies *lilies, BfmBedroomFigure *figure,
                        const BfmBedroomView *view);
const BfmWoodsJumpPose *bfm_woods_lilies_jump_poses(const BfmWoodsLilies *lilies);

/* Positive support IDs belong to their mounted room. Update the platforms once
 * per field update. A supported rider receives the change of platform origin. */
void bfm_woods_lilies_tick(BfmWoodsLilies *lilies, unsigned rider, const int16_t feet[3],
                          int16_t carry[3]);
unsigned bfm_woods_lilies_support(const BfmWoodsLilies *lilies, unsigned rider,
                                 const int16_t from[3], int16_t to[3]);
/* A fresh descent tests the player's swept segment or, when regular
 * movement does not advance the root, a short ray at the proposed feet. */
unsigned bfm_woods_lilies_land(const BfmWoodsLilies *lilies, const int16_t from[3],
                              int16_t feet[3], int short_probe);
void bfm_woods_lilies_shadow(const BfmWoodsLilies *lilies, BfmBedroomRoot *root);

#endif
