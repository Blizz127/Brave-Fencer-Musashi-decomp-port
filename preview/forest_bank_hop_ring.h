/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_BANK_HOP_RING_H
#define BFM_FOREST_BANK_HOP_RING_H

#include "bedroom_figure.h"
#include "woods_foot_dust.h"

typedef struct {
    BfmWoodsFootDust dust;
    BfmSteamChargeEffects pool;
} BfmForestBankHopRing;

int bfm_forest_bank_hop_ring_load(BfmForestBankHopRing *effect, const MusashiDiscMedia *disc);
/* Advance once per room update, even after Hop ends. Ordinary floor contact
 * emits the first puff; the initial launch or a relaunch emits the other. */
void bfm_forest_bank_hop_ring_step(BfmForestBankHopRing *effect, const BfmBedroomRoot *root,
                                   const int eye[3], int contact_puff, int launch);
/* Submit while the field's depth queue is accepting actors and effects. */
int bfm_forest_bank_hop_ring_append(const BfmForestBankHopRing *effect,
                                    BfmBedroomFigure *figure, const BfmBedroomView *view);

#endif
