/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MINE_CORRIDOR_408_H
#define BFM_VILLAGE_MINE_CORRIDOR_408_H

#include "bedroom_figure.h"
#include "forest_guard_contact.h"

enum { BFM_MINE_WHEEL_COUNT = 3, BFM_MINE_WHEEL_DECKS = 12 };
typedef struct BfmVillageMineWheels BfmVillageMineWheels;

/* A positive gain requests the loop at this level for the wheel's channel.
 * A negative gain stops the channel; zero emits no command. */
typedef struct { int gain[BFM_MINE_WHEEL_COUNT]; } BfmMineWheelAudio;

BfmVillageMineWheels *bfm_village_mine_wheels_open(const uint8_t *scene, size_t bytes);
void bfm_village_mine_wheels_free(BfmVillageMineWheels *wheels);

/* Update after the player has tested the last completed platforms. reference_x
 * is the field's published proximity X. A rider is a deck index or -1. */
BfmMineWheelAudio bfm_village_mine_wheels_step(BfmVillageMineWheels *wheels,
    int16_t reference_x, int rider, int32_t foot_q16[3]);
/* Returns NULL if this deck is outside the field's active distance. */
const BfmForestTransform *bfm_village_mine_wheels_deck(const BfmVillageMineWheels *wheels,
    unsigned index, const BfmForestGuardBox **box);
int bfm_village_mine_wheels_draw(void *wheels, BfmBedroomFigure *figure, const BfmBedroomView *view);

#endif
