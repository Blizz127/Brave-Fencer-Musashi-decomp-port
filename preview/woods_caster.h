/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_CASTER_H
#define BFM_WOODS_CASTER_H

#include "bedroom_figure.h"
#include "bedroom_travel.h"

/* The woods scene stays mounted until the casters are freed. */
typedef struct BfmWoodsCaster BfmWoodsCaster;
enum { BFM_WOODS_CASTERS = 2 };

BfmWoodsCaster *bfm_woods_caster_load(const uint8_t *scene, size_t bytes);
void bfm_woods_caster_free(BfmWoodsCaster *casters);
int bfm_woods_caster_step(BfmWoodsCaster *casters, const BfmBedroomView *view,
                          const BfmBedroomRoot *player);
/* Append to the prepared field figure queue. The field draws these faces
 * at their normal depths alongside Musashi, the sign and the scenery. */
int bfm_woods_caster_add(const BfmWoodsCaster *casters, BfmBedroomFigure *figure,
                         const BfmBedroomView *view);

/* Return the nearest target on the supplied (wall-clipped) blade segment.
 * The positive return value identifies this placement for the current room.
 * A miss leaves both outputs unchanged. */
int bfm_woods_caster_contact(const BfmWoodsCaster *casters, const int16_t start[3],
                             const int16_t end[3], int16_t contact[3], int16_t center[3]);
int bfm_woods_caster_target(const BfmWoodsCaster *casters, int target, int16_t center[3]);
int bfm_woods_caster_root(const BfmWoodsCaster *casters, int target, int16_t root[3]);
int bfm_woods_caster_latch(BfmWoodsCaster *casters, int target);
/* Update the visible tethered body from the live absorption meter. */
void bfm_woods_caster_meter(BfmWoodsCaster *casters, int target, unsigned meter);
void bfm_woods_caster_release(BfmWoodsCaster *casters, int target);
int bfm_woods_caster_absorb(BfmWoodsCaster *casters, int target);

#endif
