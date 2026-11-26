/* SPDX-License-Identifier: MIT */
#ifndef BFM_LIBRARY_BOOKS_H
#define BFM_LIBRARY_BOOKS_H
#include "forest_message_art.h"
typedef struct BfmLibraryBooks BfmLibraryBooks;
enum { BFM_BOOKS_ACTIVE, BFM_BOOKS_QUIT, BFM_BOOKS_SELECTED };
BfmLibraryBooks *bfm_library_books_open(const MusashiDiscMedia *disc, const uint8_t *scene, size_t bytes);
void bfm_library_books_free(BfmLibraryBooks *books);
int bfm_library_books_input(BfmLibraryBooks *books, int direction, int held, int cross,
                            int back, int available, unsigned *effect);
int bfm_library_books_draw(const BfmLibraryBooks *books, const BfmForestMessageArt *art,
                          const uint16_t tiles[512][1024], uint32_t pixels[240][320]);
#endif
