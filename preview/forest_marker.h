/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_MARKER_H
#define BFM_FOREST_MARKER_H
#include "bedroom_reaction.h"
#include "forest_dust.h"
#include "forest_field.h"

typedef struct {
    BfmBedroomReaction reaction;
    int16_t owner[3];
    int16_t y_offset;
} BfmForestMarker;

/* The owner supplies its current world position every tick.
 * The field must outlive the effect. The issue tick includes initial growth. */
int bfm_forest_marker_start(BfmForestMarker *marker, const BfmForestField *field,
                            const int16_t owner[3], int16_t y_offset);
void bfm_forest_marker_advance(BfmForestMarker *marker, const int16_t owner[3]);
int bfm_forest_marker_project(const BfmForestMarker *marker, const BfmForestCamera *camera,
                              BfmForestDustCard *out);
#endif
