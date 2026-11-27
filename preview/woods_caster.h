/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_CASTER_H
#define BFM_WOODS_CASTER_H

#include <limits.h>
#include "bedroom_figure.h"
#include "bedroom_travel.h"
#include "gameplay_run.h"
#include "woods_caster_ring.h"

/* The woods scene stays mounted until the casters are freed. */
typedef struct BfmWoodsCaster BfmWoodsCaster;
typedef struct BfmWoodsChargedHit BfmWoodsChargedHit;
enum { BFM_WOODS_CASTERS = 2, BFM_WOODS_SATIATE_TARGET,
       BFM_WOODS_FUSION_TARGETS = BFM_WOODS_SATIATE_TARGET };

BfmWoodsCaster *bfm_woods_caster_load(const uint8_t *scene, size_t bytes);
void bfm_woods_caster_free(BfmWoodsCaster *casters);
int bfm_woods_caster_step(BfmWoodsCaster *casters, const BfmBedroomView *view,
                          const BfmBedroomRoot *player, BfmGameplayRun *run);
int bfm_woods_caster_faces(const BfmWoodsCaster *casters, uint16_t tiles[512][1024]);
unsigned bfm_woods_caster_west_strike(const BfmWoodsCaster *casters);
/* A zero yaw is valid. Read after a connected west strike on this update. */
unsigned bfm_woods_caster_west_strike_yaw(const BfmWoodsCaster *casters);
/* Latest overlapping spell after actor update. Player reception decides its effects. */
const BfmWoodsCasterRingHit *bfm_woods_caster_spell(const BfmWoodsCaster *casters);
unsigned bfm_woods_caster_west_sound(const BfmWoodsCaster *casters);
unsigned bfm_woods_caster_cast_sound(const BfmWoodsCaster *casters);
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
unsigned bfm_woods_caster_ability(const BfmWoodsCaster *casters, int target);
/* A positive damage return belongs to the purple casters. The west-bank
 * creature responds to a charged hit with a tumble at its existing HP. */
#define BFM_WOODS_CASTER_RECOIL_ONLY UINT_MAX
unsigned bfm_woods_caster_sword(BfmWoodsCaster *casters, unsigned target,
                                unsigned event, unsigned raw_damage, unsigned hero_yaw,
                                const int16_t start[3], const int16_t end[3], int16_t impact[3]);
/* Real blade contact supplies its outward normal and the target's hit cue. */
unsigned bfm_woods_caster_charged_blade(BfmWoodsCaster *casters, unsigned target, unsigned strength,
                                       unsigned heading, const int16_t from[3], const int16_t to[3],
                                       int16_t impact[3], int16_t normal[3], unsigned *sound);
/* The travelling charge measures the distance between actor roots on all
 * three axes. A miss preserves the random stream and leaves impact untouched. */
unsigned bfm_woods_caster_charged_wave(BfmWoodsCaster *casters, unsigned target,
                                      const BfmWoodsChargedHit *hit, BfmGameplayRun *run,
                                      const int16_t wave_xyz[3], unsigned heading, int16_t impact[3]);
void bfm_woods_caster_number(BfmWoodsCaster *casters, const int16_t impact[3], unsigned amount);
/* Single-update notifications for the ordinary death burst and subsequent drop. */
int bfm_woods_caster_burst(const BfmWoodsCaster *casters, unsigned target, BfmBedroomRoot *where);
int bfm_woods_caster_defeat(const BfmWoodsCaster *casters, unsigned target,
                            int16_t where[3], unsigned *body_exp);
int bfm_woods_caster_latch(BfmWoodsCaster *casters, int target);
/* Update the visible tethered body from the live absorption meter. */
void bfm_woods_caster_meter(BfmWoodsCaster *casters, int target, unsigned meter);
void bfm_woods_caster_release(BfmWoodsCaster *casters, int target);
int bfm_woods_caster_absorb(BfmWoodsCaster *casters, int target);

#endif
