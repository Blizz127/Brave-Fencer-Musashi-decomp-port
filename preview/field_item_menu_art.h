/* SPDX-License-Identifier: MIT */
#ifndef BFM_FIELD_ITEM_MENU_ART_H
#define BFM_FIELD_ITEM_MENU_ART_H

#include "bedroom_record.h"

typedef struct BfmFieldItemMenuArt BfmFieldItemMenuArt;
enum { BFM_FIELD_MENU_ITEMS, BFM_FIELD_MENU_ACTIONS };

typedef struct {
    unsigned phase, item_slot, action_row, ink_age, hand_age, hint_columns, hint_lines;
} BfmFieldItemMenuView;

/* The room and resident MAIN/10 payloads remain mounted for the life of the menu.
 * inset is the room-relative start of its field-menu layout. */
BfmFieldItemMenuArt *bfm_field_item_menu_art_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, size_t inset, const uint8_t *main, size_t main_bytes,
    const uint16_t tiles[512][1024]);
void bfm_field_item_menu_art_free(BfmFieldItemMenuArt *art);
/* A marked slot displays a second hand; pass -1 when there is none. */
int bfm_field_forced_items_draw(const BfmFieldItemMenuArt *art, const BfmFieldItemMenuView *view,
    const BfmSaveRecord *save, int marked, const uint16_t tiles[512][1024],
    uint32_t pixels[240][320]);

#endif
