/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_GUARD_CONTACT_H
#define BFM_FOREST_GUARD_CONTACT_H

#include "forest_field.h"

typedef struct {
    int16_t low[3], high[3];
} BfmForestGuardBox;

/* Load the single body box belonging to the mounted creek guard. */
int bfm_forest_guard_box_load(const BfmForestField *field, BfmForestGuardBox *box);

/* Test one world-space blade segment against the guard's last completed,
 * unit-scale model transform. On success return its world-space box center. */
int bfm_forest_guard_contact(const BfmForestGuardBox *box, const BfmForestTransform *root,
                             const int16_t start[3], const int16_t end[3], int16_t center[3]);

/* A hit can also return its world-space contact and outward Q12 normal.
 * A start inside uses the horizontal direction away from the box center.
 * A miss preserves all outputs. The blade endpoint may be updated in place. */
int bfm_forest_guard_contact_point(const BfmForestGuardBox *box, const BfmForestTransform *root,
                                   const int16_t start[3], const int16_t end[3],
                                   int16_t center[3], int16_t contact[3], int16_t normal[3]);

/* The movement query slides the requested endpoint along the contacted face. */
int bfm_forest_body_slide(const BfmForestGuardBox *box, const BfmForestTransform *root,
                          const int16_t start[3], const int16_t end[3], int16_t contact[3]);

#endif
