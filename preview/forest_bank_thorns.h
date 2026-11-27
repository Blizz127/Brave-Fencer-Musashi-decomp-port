/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_BANK_THORNS_H
#define BFM_FOREST_BANK_THORNS_H

#include "woods_caster_ring.h"

enum { BFM_FOREST_BANK_THORN_REASON = 22 };

/* Feed each actual floor or wall contact in collision order. An unmarked face
 * leaves the preceding contact in place. Other surface events share this latch. */
static inline void bfm_forest_bank_latch_material(uint16_t *reason, uint16_t material)
{
    uint8_t tag = (uint8_t)material;
    if (tag) *reason = tag;
}

/* Dispatch the latched thorn to the field's incoming hit slot. Pass the raw
 * equipped ability and the source root yaw. An event lock consumes the contact without posting a hit.
 * A return of one means the receiver has a new hit pending. With no new hit,
 * both the incumbent hit and any unrelated surface reason are preserved. */
int bfm_forest_bank_thorn_hit(uint16_t *reason, uint32_t event_state,
                             uint8_t equipped_ability, uint16_t source_yaw,
                             BfmWoodsCasterRingHit *incoming);

#endif
