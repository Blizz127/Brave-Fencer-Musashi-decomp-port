/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_SUMMIT_DEPARTURE_H
#define BFM_TOWER_SUMMIT_DEPARTURE_H

#include "forest_dust.h"

enum { BFM_TOWER_SUMMIT_DEPARTURE_CARDS = 50 };

/* The departure controller switches directly from follow to this fixed shot. */
int bfm_tower_summit_departure_camera(BfmForestCamera *camera, int32_t eye[3]);

/* Age zero is the outgoing pad light's creation update. Apply to a fresh
 * copy of the visual root each frame. The collision root stays on the pad. */
void bfm_tower_summit_departure_hero(unsigned beam_age, BfmForestTransform *render_root);

/* Project the mounted room's pad light into the ordinary ordering table.
 * scene_tick is the rendering update; its parity controls the cyan flicker.
 * Supply one complete light's worth of space. */
size_t bfm_tower_summit_departure_cards(const uint8_t *scene, size_t bytes,
                                        const int16_t pad[3], unsigned beam_age, unsigned scene_tick,
                                        const BfmForestCamera *camera,
                                        BfmForestDustCard *out, size_t capacity);

#endif
