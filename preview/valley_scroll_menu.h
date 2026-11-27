/* SPDX-License-Identifier: MIT */
#ifndef BFM_VALLEY_SCROLL_MENU_H
#define BFM_VALLEY_SCROLL_MENU_H

#include "bedroom_record.h"
#include "field_input.h"

typedef struct BfmValleyScrollMenu BfmValleyScrollMenu;

/* The Valley and MAIN/10 payloads outlive the menu. */
BfmValleyScrollMenu *bfm_valley_scroll_menu_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, const uint8_t *main, size_t main_bytes,
    const uint16_t tiles[512][1024]);
void bfm_valley_scroll_menu_free(BfmValleyScrollMenu *menu);
int bfm_valley_scroll_menu_active(const BfmValleyScrollMenu *menu);
/* Call only after the field grants player control. Returns one while the menu
 * owns input, including the frame that returns control to the field. */
int bfm_valley_scroll_menu_input(BfmValleyScrollMenu *menu, int start_pressed,
                                 BfmFieldInput input, BfmSaveRecord *save);
int bfm_valley_scroll_menu_draw(const BfmValleyScrollMenu *menu,
    const uint16_t tiles[512][1024], uint32_t pixels[240][320]);

#endif
