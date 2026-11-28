/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MINE_CREATURE_DUST_H
#define BFM_VILLAGE_MINE_CREATURE_DUST_H

#include "bedroom_figure.h"

typedef struct BfmMineCreatureDust BfmMineCreatureDust;

BfmMineCreatureDust *bfm_mine_creature_dust_open(const MusashiDiscMedia *disc);
void bfm_mine_creature_dust_free(BfmMineCreatureDust *dust);
/* The creature requests the first free room actor slot above its own.
 * A new puff advances when that slot is visited in the same field. */
int bfm_mine_creature_dust_spawn_at(BfmMineCreatureDust *dust, unsigned slot,
                                    const int16_t origin[3]);
int bfm_mine_creature_dust_occupied(const BfmMineCreatureDust *dust, unsigned slot);
void bfm_mine_creature_dust_step_at(BfmMineCreatureDust *dust, unsigned slot);
int bfm_mine_creature_dust_draw_at(const BfmMineCreatureDust *dust, unsigned slot,
                                   BfmBedroomFigure *figure, const BfmBedroomView *view);

#endif
