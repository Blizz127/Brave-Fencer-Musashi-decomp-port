/* SPDX-License-Identifier: MIT */
#ifndef BFM_BAKERY_PURCHASE_H
#define BFM_BAKERY_PURCHASE_H

#include "gameplay_run.h"
#include <stddef.h>

typedef struct {
    unsigned item, drans, sell_drans;
    const uint8_t *name;
    size_t name_bytes;
} BfmBakeryGood;

typedef struct {
    unsigned count;
    BfmBakeryGood goods[7];
} BfmBakeryPurchase;

typedef enum {
    BFM_BAKERY_BUY_ERROR = 0,
    BFM_BAKERY_BOUGHT = 1,
    BFM_BAKERY_BAG_FULL = 2,
    BFM_BAKERY_NO_FUNDS = 3
} BfmBakeryBuyResult;

typedef enum {
    BFM_BAKERY_SELL_ERROR = 0,
    BFM_BAKERY_SOLD = 4,
    BFM_BAKERY_NOT_WANTED = 5,
    BFM_BAKERY_SELL_EMPTY = 6
} BfmBakerySellResult;

/* Names borrow fullwidth text from the mounted MAIN member 10 player data.
 * name_bytes excludes the terminator. The item catalogue gives the base price. */
int bfm_bakery_item(const uint8_t *main, size_t main_bytes, unsigned item, BfmBakeryGood *out);

/* scene is the expanded SC03 member 1. Opening Jam's buy counter snapshots
 * the available goods and the prices at the current time of day. */
int bfm_bakery_stock(BfmBakeryPurchase *shop, const uint8_t *scene, size_t scene_bytes,
                     const uint8_t *main, size_t main_bytes, const BfmSaveRecord *state);

/* Jam pays the mounted resale price only for items this counter accepts.
 * slot is the physical bag position, column * 6 + row, including holes.
 * An empty cell has an error tone and no response text. */
BfmBakerySellResult bfm_bakery_sell(const uint8_t *main, size_t main_bytes, unsigned slot,
                                    BfmSaveRecord *state, BfmGameplayRun *run);

/* Pay for one unit from a displayed row. The three ordinary results select
 * Jam's corresponding shop speech. Rejected purchases preserve both records. */
BfmBakeryBuyResult bfm_bakery_buy(const BfmBakeryPurchase *shop, unsigned selected,
                                  BfmSaveRecord *state, BfmGameplayRun *run);

#endif
