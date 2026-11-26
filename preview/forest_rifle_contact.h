/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_RIFLE_CONTACT_H
#define BFM_FOREST_RIFLE_CONTACT_H

#include "forest_field.h"

/* Test the creek shot's last completed model transform against the player's
 * active default body box. The caller checks room surfaces first. A nonzero
 * projectile contact-suppression field disables the body test. */
int bfm_forest_rifle_hits_player(const BfmForestField *field,
                                 const BfmForestTransform *projectile,
                                 const BfmForestTransform *player,
                                 int contact_suppressed);

/* Test a room's two mounted attack endpoints against the player's current
 * 16-byte type-zero body descriptor. */
int bfm_forest_attack_segment_hits_player(const BfmForestField *field, uint32_t descriptor,
                                         uint32_t body_address, const BfmForestTransform *attacker,
                                         const BfmForestTransform *player);

/* The normal creek swordsman's local blade segment comes from its room. */
int bfm_forest_soldier_hits_player(const BfmForestField *field,
                                   const BfmForestTransform *soldier,
                                   const BfmForestTransform *player);

int bfm_forest_mouth_hits_player(const BfmForestField *field,
                                 const BfmForestTransform *figure,
                                 const BfmForestTransform *player);

#endif
