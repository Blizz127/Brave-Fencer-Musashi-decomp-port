/* SPDX-License-Identifier: MIT */
#include "inn_menu.h"

enum { UP = 1u << 4, RIGHT = 1u << 5, DOWN = 1u << 6, LEFT = 1u << 7,
       TRIANGLE = 1u << 12, CROSS = 1u << 14 };
enum { SLEEP, SAVE, CHAT, QUIT };

void bfm_inn_menu_begin(BfmInnMenu *menu, const BfmSaveMenu *card)
{
    *menu = (BfmInnMenu){ .page = BFM_INN_SERVICE };
    /* The writable profile supplies either a decoded file or a validated,
     * unallocated card. Continue handles its no-data screen separately. */
    if (card && card->selected < 4 && !card->issue &&
        (card->page == BFM_SAVE_PICK || card->page == BFM_SAVE_CARD_LOADING)) {
        menu->card = *card;
        menu->card.page = BFM_SAVE_PICK;
        menu->writable = 1;
    } else if (card && card->issue) {
        menu->card.issue = card->issue;
    } else {
        menu->card.issue = card ? "The writable card could not be read." :
                                  "Open a writable card copy to save.";
    }
}

BfmInnAction bfm_inn_menu_input(BfmInnMenu *menu, uint16_t pressed)
{
    switch (menu->page) {
    case BFM_INN_SERVICE:
        if (pressed & TRIANGLE) return BFM_INN_LEAVE;
        if ((pressed & (UP | DOWN)) == UP) menu->service = (menu->service + 3) % 4;
        if ((pressed & (UP | DOWN)) == DOWN) menu->service = (menu->service + 1) % 4;
        if (pressed & CROSS) {
            if (menu->service == QUIT) return BFM_INN_LEAVE;
            if (menu->service == SLEEP) {
                menu->room = 0;
                menu->page = BFM_INN_ROOMS;
            } else if (menu->service == SAVE && menu->writable) {
                menu->page = BFM_INN_FILES;
            } else {
                menu->page = BFM_INN_SERVICE_ERROR;
                menu->notice = menu->service == SAVE ? menu->card.issue :
                                                      "This service is unavailable.";
            }
        }
        break;
    case BFM_INN_ROOMS:
        if (pressed & TRIANGLE) {
            menu->page = BFM_INN_SERVICE;
            break;
        }
        if ((pressed & (UP | DOWN)) == UP) menu->room = (menu->room + 5) % 6;
        if ((pressed & (UP | DOWN)) == DOWN) menu->room = (menu->room + 1) % 6;
        if (pressed & CROSS) {
            if (!menu->room) return BFM_INN_STAY;
            menu->notice = menu->room >= 4 ? "This room is occupied." :
                                             "This room is unavailable in the preview.";
            menu->page = BFM_INN_ROOM_ERROR;
        }
        break;
    case BFM_INN_ROOM_ERROR:
        if (pressed & (CROSS | TRIANGLE)) menu->page = BFM_INN_ROOMS;
        break;
    case BFM_INN_REST:
        ++menu->rest_age;
        if (menu->rest_age == 56) return BFM_INN_RECOVER;
        if (menu->rest_age >= 111) menu->page = BFM_INN_SERVICE;
        break;
    case BFM_INN_FILES: {
        if (pressed & TRIANGLE) { menu->page = BFM_INN_SERVICE; break; }
        unsigned horizontal = pressed & (LEFT | RIGHT), vertical = pressed & (UP | DOWN);
        if (horizontal == LEFT || horizontal == RIGHT) menu->card.selected ^= 1u;
        if (vertical == UP || vertical == DOWN) menu->card.selected ^= 2u;
        if (pressed & CROSS) {
            BfmSaveState state = menu->card.rows[menu->card.selected].state;
            if (state == BFM_SAVE_EMPTY || state == BFM_SAVE_VALID) {
                menu->overwriting = state == BFM_SAVE_VALID;
                menu->page = BFM_INN_CONFIRM;
            } else {
                menu->page = BFM_INN_FILE_ERROR;
                menu->notice = "This file is damaged.";
            }
        }
        break;
    }
    case BFM_INN_CONFIRM:
        if (pressed & TRIANGLE) menu->page = BFM_INN_FILES;
        else if (pressed & CROSS) {
            menu->page = BFM_INN_WRITING;
            return BFM_INN_WRITE;
        }
        break;
    case BFM_INN_WRITING:
        break;
    case BFM_INN_SAVED:
    case BFM_INN_SERVICE_ERROR:
        if (pressed & (CROSS | TRIANGLE)) menu->page = BFM_INN_SERVICE;
        break;
    case BFM_INN_FILE_ERROR:
        if (pressed & (CROSS | TRIANGLE)) menu->page = BFM_INN_FILES;
        break;
    }
    return BFM_INN_NONE;
}

void bfm_inn_menu_stay(BfmInnMenu *menu, int paid)
{
    if (menu->page != BFM_INN_ROOMS || menu->room) return;
    menu->notice = NULL;
    menu->rest_age = 0;
    menu->page = paid ? BFM_INN_REST : BFM_INN_ROOM_ERROR;
}

void bfm_inn_menu_complete(BfmInnMenu *menu, const BfmSaveMenu *readback, const char *error)
{
    if (menu->page != BFM_INN_WRITING) return;
    if (error || !readback || readback->issue || readback->page != BFM_SAVE_PICK ||
        readback->selected != menu->card.selected ||
        readback->rows[menu->card.selected].state != BFM_SAVE_VALID) {
        menu->page = BFM_INN_FILE_ERROR;
        menu->notice = error ? "Save unconfirmed. Retry or reopen." :
                               "Could not verify the saved file.";
        return;
    }
    menu->card = *readback;
    menu->page = BFM_INN_SAVED;
}
