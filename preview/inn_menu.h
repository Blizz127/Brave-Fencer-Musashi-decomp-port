/* SPDX-License-Identifier: MIT */
#ifndef BFM_INN_MENU_H
#define BFM_INN_MENU_H

#include "save_menu.h"

typedef enum {
    BFM_INN_SERVICE, BFM_INN_FILES, BFM_INN_CONFIRM, BFM_INN_WRITING,
    BFM_INN_SAVED, BFM_INN_FILE_ERROR, BFM_INN_SERVICE_ERROR
} BfmInnPage;

typedef enum { BFM_INN_NONE, BFM_INN_WRITE, BFM_INN_LEAVE } BfmInnAction;

typedef struct {
    BfmSaveMenu card;
    BfmInnPage page;
    unsigned service;
    const char *notice;
    int writable;
} BfmInnMenu;

/* Begin after Hotelo's hospitality dialogue. Supply a parsed snapshot only for
 * a distinct writable card with an existing Musashi file. NULL keeps the
 * normal read-only player's card outside the save flow. */
void bfm_inn_menu_begin(BfmInnMenu *menu, const BfmSaveMenu *card);
/* Each call consumes fresh PlayStation button edges. WRITE is emitted once;
 * the requested file number is card.selected + 1. */
BfmInnAction bfm_inn_menu_input(BfmInnMenu *menu, uint16_t pressed);
/* Call after the durable operation. A successful result includes a fresh
 * readback of the card. A non-NULL error always prevents acknowledgement. */
void bfm_inn_menu_complete(BfmInnMenu *menu, const BfmSaveMenu *readback, const char *error);
/* Overlay the current inn counter framebuffer; redraw that scene first. */
void bfm_inn_menu_draw(const BfmInnMenu *menu, const BfmSaveArt *art,
                       unsigned fields, uint32_t pixels[240][320]);

#endif
