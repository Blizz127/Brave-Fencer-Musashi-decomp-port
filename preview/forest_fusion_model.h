/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_FUSION_MODEL_H
#define BFM_FOREST_FUSION_MODEL_H
#include "forest_flight.h"
#include "forest_terrain.h"

typedef struct BfmFusionModel BfmFusionModel;

/* Load the separately authored thrown sword and its single pose from the
 * mounted player archive and US executable. The model owns the decoded data. */
BfmFusionModel *bfm_fusion_model_open(const MusashiDiscMedia *disc);
void bfm_fusion_model_free(BfmFusionModel *model);

/* Once per simulation update. The returned actor and its transform remain
 * valid until the next update. A successful Square edge during a live
 * absorption makes the model pulse. The owner submits it in ordinary depth
 * order and can copy its transform when buffering a frame. */
const BfmForestActor *bfm_fusion_model_tick(BfmFusionModel *model, const BfmFusionFlight *flight,
                                           int absorption_square_edge);
#endif
