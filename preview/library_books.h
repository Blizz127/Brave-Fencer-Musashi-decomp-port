/* SPDX-License-Identifier: MIT */
#ifndef BFM_LIBRARY_BOOKS_H
#define BFM_LIBRARY_BOOKS_H
#include "forest_message_art.h"
typedef struct BfmLibraryBooks BfmLibraryBooks;
typedef struct BfmCastleMenuArt BfmCastleMenuArt;
enum { BFM_MENU_LABEL_NORMAL, BFM_MENU_LABEL_DIM, BFM_MENU_LABEL_EQUIPPED };
typedef struct {
    const uint8_t *bytes;
    size_t size;
    int ink;
} BfmCastleMenuLabel;

/* Retains the room data and reads the resident menu font maps from the disc. */
BfmCastleMenuArt *bfm_castle_menu_art_open(const MusashiDiscMedia *disc,
                                           const uint8_t *scene, size_t bytes);
void bfm_castle_menu_art_free(BfmCastleMenuArt *art);
/* Field menus use the same atlas, but carry their frame, fill and label tables separately. */
void bfm_castle_menu_window(const uint8_t frames[96], const uint8_t fills[32], int active,
                            const uint16_t tiles[512][1024], uint32_t pixels[240][320]);
int bfm_castle_menu_label(const BfmCastleMenuArt *art, BfmCastleMenuLabel name,
                          const uint16_t tiles[512][1024], uint32_t pixels[240][320],
                          int x, int y, int active);
void bfm_castle_menu_sprite(const uint16_t tiles[512][1024], uint32_t pixels[240][320],
                           int x, int y, int w, int h, int u, int v, int uw, int vh,
                           int active, int ink);
/* The selector indexes a visible row. NULL names use the descriptor's labels.
 * A scrolling list may supply bounded labels borrowed from another member. */
int bfm_castle_menu_art_draw_panel(const BfmCastleMenuArt *art, uint32_t panel_address,
                                   const BfmCastleMenuLabel *visible_names, unsigned visible_count,
                                   unsigned selected, int active, int can_scroll_up, int can_scroll_down,
                                   const uint16_t tiles[512][1024], uint32_t pixels[240][320]);
enum { BFM_BOOKS_ACTIVE, BFM_BOOKS_QUIT, BFM_BOOKS_SELECTED, BFM_COMMAND_SELECTED };
BfmLibraryBooks *bfm_castle_menu_open(const MusashiDiscMedia *disc, const uint8_t *scene, size_t bytes, int command);
unsigned bfm_castle_command_selection(const BfmLibraryBooks *books);
/* The mounted book and topic indices are each one-based in the room result. */
unsigned bfm_library_books_selection(const BfmLibraryBooks *books);
void bfm_library_books_resume(BfmLibraryBooks *books, int contents);
void bfm_library_books_free(BfmLibraryBooks *books);
int bfm_library_books_input(BfmLibraryBooks *books, int direction, int held, int cross,
                            int back, int available, unsigned *effect);
int bfm_library_books_draw(const BfmLibraryBooks *books, const BfmForestMessageArt *art,
                          const uint16_t tiles[512][1024], uint32_t pixels[240][320]);
#endif
