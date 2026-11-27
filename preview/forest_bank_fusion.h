/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_BANK_FUSION_H
#define BFM_FOREST_BANK_FUSION_H
#include "woods_fusion.h"

typedef struct BfmBankHoppers BfmBankHoppers;

/* Owns the blade, tether and award state for this visit to the forest bank. */
BfmWoodsFusion *bfm_forest_bank_fusion_open(const MusashiDiscMedia *disc,
                                           const uint8_t *scene, size_t bytes);
int bfm_forest_bank_fusion_step(BfmWoodsFusion *fusion, BfmBankHoppers *hoppers,
    BfmGameplayRun *run, const BfmBedroomRoot *root, unsigned field_tick,
    const BfmWoodsFusionInput *input, BfmWoodsFusionFrame *frame);
#endif
