/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_FALL_H
#define BFM_FOREST_FALL_H

#include "forest_dust.h"
#include "gameplay_run.h"

struct BfmForestField;

typedef struct {
    int16_t position[3];
    uint16_t kind, angle, cell;
} BfmForestFallSprite;

typedef struct {
    BfmForestFallSprite sprite;
    int64_t position[3];
    int32_t velocity[3], acceleration_x, acceleration_z;
    int16_t floor;
    unsigned phase, ready;
} BfmForestFallingParticle;

typedef struct {
    const uint8_t *sprites;
    unsigned timer, count, other_effects, other_drawables;
    BfmForestFallingParticle particles[BFM_FOREST_FALL_SLOTS];
} BfmForestFall;

/* Load the permanent forest emitter and its four mounted sprite cells.
 * The field owns this memory for the lifetime of the effect system. */
int bfm_forest_fall_load(BfmForestFall *fall, const struct BfmForestField *field,
                         uint32_t placements);
void bfm_forest_fall_set_other_slots(BfmForestFall *fall, unsigned effects, unsigned drawables);

/* Actor pass: the selector and side have already been drawn by the requesting
 * actor. A successful birth chooses its position relative to the player. */
void bfm_forest_fall_emit(BfmForestFall *fall, BfmGameplayRun *run, const int16_t player[3],
                          unsigned selector, unsigned side);
void bfm_forest_fall_tick_emitter(BfmForestFall *fall, BfmGameplayRun *run,
                                  const int16_t player[3]);

/* Room effects run after all actors; initialization consumes the birth draws. */
void bfm_forest_fall_step(BfmForestFall *fall, BfmGameplayRun *run);
int bfm_forest_fall_project_sprite(const BfmForestFallSprite *sprite, const uint8_t *descriptor,
                                   const BfmForestCamera *camera, BfmForestDustCard *out);
unsigned bfm_forest_fall_project(const BfmForestFall *fall, const BfmForestCamera *camera,
                                 BfmForestDustCard *out, unsigned capacity);
#endif
