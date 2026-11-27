/* SPDX-License-Identifier: MIT */
#ifndef BFM_VALLEY_HINGE_H
#define BFM_VALLEY_HINGE_H

#include "forest_guard_contact.h"

typedef enum {
    BFM_VALLEY_HINGE_RAISED, BFM_VALLEY_HINGE_TIPPING, BFM_VALLEY_HINGE_DOWN
} BfmValleyHingePhase;

typedef struct {
    BfmForestGuardBox raised_box, down_box;
    BfmForestTransform body, model;
    int16_t pivot[3], down_pivot[3], down_part[3], strike_point[3];
    unsigned flag, yaw, pitch, impacts;
    int32_t angular_speed;
    BfmValleyHingePhase phase;
} BfmValleyHinge;

typedef struct {
    int impact, player_hit, settled;
    int16_t debris_origin[3], hit_position[3], hit_normal[3];
} BfmValleyHingeEvent;

/* Load the north ramp's placement, contact shapes and part offsets from
 * SC03/31. A previously earned flag starts with the final solid installed. */
int bfm_valley_hinge_load(BfmValleyHinge *hinge, const uint8_t *scene,
                         size_t bytes, const uint8_t flags[64]);

/* quake_in_view combines the Earth signal with the ordinary field projection
 * test of this placement. Supply the current foot for the impact hazard. */
BfmValleyHingeEvent bfm_valley_hinge_step(BfmValleyHinge *hinge, int quake_in_view,
                                         const int16_t foot[3]);

/* The field uses the last completed actor transform for the finite box.
 * This query also applies while the slab is upright or rebounding. */
int bfm_valley_hinge_contact(const BfmValleyHinge *hinge, const int16_t from[3],
                             const int16_t to[3], int16_t hit[3], int16_t normal[3]);

/* New foot attachments retain the proposed travel along a hit face. An
 * existing attachment or a shadow uses the point query above. */
int bfm_valley_hinge_slide(const BfmValleyHinge *hinge, const int16_t from[3],
                           const int16_t to[3], int16_t resolved[3]);

/* The grounded body resolves twice and airborne motion resolves once. The
 * field counter selects the asymmetric separation when the body is inside. */
int bfm_valley_box_block_body(const BfmForestGuardBox *box, const BfmForestTransform *body,
    const int16_t previous[3], int16_t proposed[3], unsigned field_tick, int airborne);
int bfm_valley_hinge_block_body(const BfmValleyHinge *hinge, const int16_t previous[3],
                                int16_t proposed[3], unsigned field_tick, int airborne);

#endif
