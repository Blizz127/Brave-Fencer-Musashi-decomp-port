/* SPDX-License-Identifier: MIT */
#ifndef BFM_SAVE_MENU_H
#define BFM_SAVE_MENU_H

#include <stdint.h>
#include "musashi_disc_media.h"
#include "bedroom_record.h"

typedef enum { BFM_SAVE_EMPTY, BFM_SAVE_VALID, BFM_SAVE_DAMAGED } BfmSaveState;
typedef struct {
    BfmSaveState state;
    unsigned chapter, hours, minutes, day, hour, minute, hp, max_hp, bp, max_bp, rescued;
    uint64_t money;
} BfmSaveRow;

typedef enum { BFM_SAVE_PICK, BFM_SAVE_CONFIRM, BFM_SAVE_BOUNDARY, BFM_SAVE_LOADED } BfmSavePage;
typedef struct {
    BfmSaveRow rows[4];
    BfmSaveRecord records[4];
    unsigned selected;
    BfmSavePage page;
    const char *issue;
} BfmSaveMenu;

/* Read the explicitly selected raw US card once per visit. The menu retains
   a snapshot only for each validated slot and closes the card before returning. */
void bfm_save_menu_open(BfmSaveMenu *menu, const char *path);
/* PlayStation wire-order button edges. Returns one when leaving for title. */
int bfm_save_menu_input(BfmSaveMenu *menu, uint16_t pressed);
typedef struct BfmSaveArt BfmSaveArt;
BfmSaveArt *bfm_save_art_load(const MusashiDiscMedia *disc);
void bfm_save_menu_draw(const BfmSaveArt *art, const BfmSaveMenu *menu, unsigned fields,
                        uint32_t pixels[240][320]);

#endif
