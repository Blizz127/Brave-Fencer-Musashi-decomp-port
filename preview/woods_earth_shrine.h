/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_EARTH_SHRINE_H
#define BFM_WOODS_EARTH_SHRINE_H

#include "woods_box_view.h"
#include "village_arrival_motion.h"
#include "forest_message.h"

typedef struct BfmEarthShrine BfmEarthShrine;

/* Bind the altar's registration, approach volume, destination and message
 * to this visit's mounted room. Its scene must remain alive until free. */
BfmEarthShrine *bfm_earth_shrine_open(const uint8_t *scene, size_t bytes, int already_earned);
void bfm_earth_shrine_free(BfmEarthShrine *shrine);
int bfm_earth_shrine_on_approach(BfmEarthShrine *shrine, const BfmBedroomRoot *player);
int bfm_earth_shrine_in_scene(const BfmEarthShrine *shrine);
int bfm_earth_shrine_update(BfmEarthShrine *shrine, BfmVillageMotion *motion, BfmWoodsBoxView *view,
                            BfmForestMessage **opened);
int bfm_earth_shrine_closed(BfmEarthShrine *shrine);
unsigned bfm_earth_shrine_score(const BfmEarthShrine *shrine, unsigned field_score);
void bfm_earth_shrine_score_applied(BfmEarthShrine *shrine, unsigned selector);

#endif
