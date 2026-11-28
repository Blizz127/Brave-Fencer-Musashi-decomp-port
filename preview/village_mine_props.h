/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MINE_PROPS_H
#define BFM_VILLAGE_MINE_PROPS_H

#include "bedroom_figure.h"
#include "forest_message.h"

typedef struct BfmVillageMineProps BfmVillageMineProps;

/* Both direction boards borrow their reading programs from the mounted room. */
BfmVillageMineProps *bfm_village_mine_props_open(const uint8_t *scene, size_t bytes);
/* SC03/109 uses its own two message pointers and sign placements. */
BfmVillageMineProps *bfm_village_mine_props_open_shaft(const uint8_t *scene, size_t bytes);
void bfm_village_mine_props_free(BfmVillageMineProps *props);
int bfm_village_mine_props_draw(void *props, BfmBedroomFigure *figure, const BfmBedroomView *view);
int bfm_village_mine_props_body(const BfmVillageMineProps *props, const int16_t previous[3],
                                int16_t foot[3], unsigned tick, int airborne);
/* Called for a fresh ordinary reading action. In SC03/107 a nearby live class 548
 * takes precedence at a sign. Pass NULL for its position when the actor is absent. */
BfmForestMessage *bfm_village_mine_props_read(const BfmVillageMineProps *props,
    const int16_t foot[3], int square_pressed, const int16_t actor548[3], int16_t point[3]);

#endif
