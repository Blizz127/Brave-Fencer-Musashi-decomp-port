/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_3026_BOX_HIT_H
#define BFM_WOODS_3026_BOX_HIT_H

#include "field_input.h"
#include "forest_guard_contact.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
    BfmForestTransform root;
    BfmForestGuardBox bounds;
    BfmForestTransform lid_root;
    BfmForestGuardBox lid_bounds;
    int16_t use_segment[2][3];
    unsigned yaw;
    const uint8_t *destination;
} BfmWoods3026BoxHit;

/* Load the registered Box, its base collider and the player's use segment from
 * the mounted SC02/11 scene. The 36-byte destination borrows the scene's lifetime.
 * Failure leaves out untouched. */
int bfm_woods_3026_box_hit_mount(const uint8_t *scene, size_t bytes, BfmWoods3026BoxHit *out);

int bfm_woods_3026_box_hit_mount_shaft(const uint8_t *scene, size_t bytes, BfmWoods3026BoxHit *out);

/* Use the last completed player model for the ray and the ordinary field
 * position for the Box's facing check. interaction_busy is the active field
 * interaction lock. A successful use opens the offer; it does not accept it. */
int bfm_woods_3026_box_can_use(const BfmWoods3026BoxHit *box, const BfmFieldInput *input,
                              const BfmForestTransform *player_model, const int16_t player_xyz[3],
                              uint32_t player_flags, int interaction_busy);

/* Sweep the ordinary character's previous and requested feet against the two
 * parts of the closed box. The shared outdoor actor clock supplies the
 * sideways push when the actor already stands inside the solid. */
int bfm_woods_3026_box_body(const BfmWoods3026BoxHit *box, unsigned field_tick,
                           const int16_t previous[3], int16_t requested[3]);

#endif
