/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_LIFT_H
#define BFM_FOREST_LIFT_H
#include "forest_field.h"

struct BfmForestShoreFigures;
struct BfmForestShoreFigures *bfm_forest_lift_figures(BfmForestField *field);

typedef enum {
    BFM_LIFT_BRACE, BFM_LIFT_RAISE, BFM_LIFT_IDLE, BFM_LIFT_WALK,
    BFM_LIFT_THROW, BFM_LIFT_CLIPS
} BfmLiftClip;
enum { BFM_LIFT_KEYS = 32 };
typedef struct {
    unsigned count, event;
    uint32_t frames[BFM_LIFT_KEYS];
} BfmLiftMotion;
typedef struct {
    BfmLiftMotion motion[BFM_LIFT_CLIPS];
    const uint8_t *raise_offsets, *toss_offsets;
    int32_t walking_speed;
    uint16_t grab_base, grab_spread;
} BfmForestLiftData;
typedef struct {
    unsigned age;
    BfmLiftClip clip;
    int target;
    int16_t grip[3];
} BfmForestLift;
typedef enum {
    BFM_LIFT_PLAY, BFM_LIFT_ATTACH, BFM_LIFT_WALK_ATTACH,
    BFM_LIFT_RELEASE, BFM_LIFT_FINISHED
} BfmLiftStep;

/* The walk owner claims the selected body before initialization. Advance once
 * per effective field update. A release event consumes the existing held root. */
BfmLiftStep bfm_forest_lift_tick(BfmForestLift *lift, const BfmForestLiftData *data,
                                 int direction, int square_edge);
/* Decode the shared player clips. The room supplies its attachment offsets and speed. */
int bfm_forest_lift_motions(const MusashiDiscMedia *disc, uint32_t animation_table,
                            BfmForestLiftData *data);
int bfm_forest_lift_load(const BfmForestField *field, const MusashiDiscMedia *disc,
                         uint32_t animation_table, BfmForestLiftData *data);
void bfm_forest_lift_world(const BfmForestTransform *root, const int16_t local[3],
                           int16_t world[3]);
/* Resolve the held anchor against the player's last published root. */
int bfm_forest_lift_grip(const BfmForestLiftData *data, const BfmForestLift *lift,
                         const BfmForestTransform *root, int16_t world[3]);
#endif
