/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_LARDWICK_MOTES_H
#define BFM_WOODS_LARDWICK_MOTES_H

#include "bedroom_figure.h"
#include "gameplay_run.h"

/* The departure beam owns one of the room's thirty effect slots. */
enum { BFM_WOODS_LARDWICK_MOTE_SLOTS = 29 };

typedef struct {
    int16_t origin[3];
    uint8_t age, color;
} BfmWoodsLardwickMote;

typedef struct {
    BfmWoodsLardwickMote slots[BFM_WOODS_LARDWICK_MOTE_SLOTS];
} BfmWoodsLardwickMotes;

/* Advance once per departure update, including the last fields after the
 * soldier vanishes. Beam age zero is the outgoing beam's creation. */
void bfm_woods_lardwick_motes_step(BfmWoodsLardwickMotes *motes, BfmGameplayRun *run,
                                   const int16_t root[3], unsigned beam_age);
int bfm_woods_lardwick_motes_add(const BfmWoodsLardwickMotes *motes, BfmBedroomFigure *figure,
                                 const BfmBedroomView *view);
#endif
