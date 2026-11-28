/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MINE_DECKS_H
#define BFM_VILLAGE_MINE_DECKS_H

#include "bedroom_travel.h"
#include "village_mine_basket.h"
#include "village_mine_corridor_408.h"

/* Probe the completed deck transforms during the player's movement pass.
 * rider is the occupied deck index or -1. An existing rider uses the
 * integer foot captured before this field's action as last_supported.
 * Pass previous=candidate when movement sweeps are disabled.
 * foot receives the proposed position with any actor correction applied.
 * TOUCH corrects position without boarding. The room still tests terrain. */
BfmMineBasketContact bfm_village_mine_decks_probe(const BfmVillageMineWheels *wheels,
    const int16_t previous[3], const int16_t candidate[3], const int16_t last_supported[3],
    int *rider, int16_t foot[3]);

/* Combine the room shadow at the carried foot with all active decks after
 * their actor update. The room wins a tie at the same signed world Y. */
void bfm_village_mine_decks_shadow(const BfmVillageMineWheels *wheels, const int16_t carried[3],
                                   BfmBedroomRoot *shadow);

#endif
