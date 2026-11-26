/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_UPPER_TRUNKS_H
#define BFM_FOREST_UPPER_TRUNKS_H

#include "forest_bridge_floor.h"

/* The three trail trunks between the creek and the Gunshot clearing share
 * a rigid model. Each one starts falling when Musashi nears its own row. */
typedef struct BfmForestUpperTrunks BfmForestUpperTrunks;
enum { BFM_FOREST_UPPER_TRUNKS = 3 };

BfmForestUpperTrunks *bfm_forest_upper_trunks_open(const BfmForestField *field);
void bfm_forest_upper_trunks_free(BfmForestUpperTrunks *trunks);
void bfm_forest_upper_trunks_tick(BfmForestUpperTrunks *trunks, int player_z);
const BfmForestActor *bfm_forest_upper_trunk(const BfmForestUpperTrunks *trunks,
                                            unsigned index);
int bfm_forest_upper_trunks_surface(const BfmForestUpperTrunks *trunks,
    BfmForestBridgeProbe probe, const int16_t from[3], const int16_t to[3],
    BfmForestBridgeSurface *out);

#endif
