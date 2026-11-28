/* SPDX-License-Identifier: MIT */
#ifndef BFM_FIELD_FORCED_BAG_H
#define BFM_FIELD_FORCED_BAG_H

#include "field_item_menu_art.h"
#include "field_input.h"
#include "gameplay_run.h"

typedef enum { BFM_BAG_IDLE, BFM_BAG_OPEN, BFM_BAG_SELECT, BFM_BAG_WAIT, BFM_BAG_CLOSE } BfmForcedBagPhase;
typedef struct {
    BfmFieldItemMenuView view;
    BfmForcedBagPhase phase;
    unsigned fields, held, repeat, entry_page;
    int marked, draw;
} BfmFieldForcedBag;

void bfm_field_forced_bag_begin(BfmFieldForcedBag *bag);
/* Return to the field only after a slot has cleared and both menu buffers retire. */
int bfm_field_forced_bag_step(BfmFieldForcedBag *bag, BfmFieldInput input,
    const uint8_t *main, size_t bytes, BfmSaveRecord *save, BfmGameplayRun *run, unsigned sounds[2]);
int bfm_field_forced_bag_draw(const BfmFieldForcedBag *bag, const BfmFieldItemMenuArt *art,
    const BfmSaveRecord *save, const uint16_t tiles[512][1024], uint32_t pixels[240][320]);

#endif
