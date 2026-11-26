/* SPDX-License-Identifier: MIT */
#ifndef BFM_CHAPTER_CARD_H
#define BFM_CHAPTER_CARD_H
#include "musashi_disc_media.h"
#include <stdint.h>
enum { BFM_CHAPTER_FOREST_FIELD = 638 };
typedef struct BfmChapterCard BfmChapterCard;
/* Final black interval after the crest has faded. */
int bfm_chapter_card_cleared(unsigned elapsed, unsigned source_member);
BfmChapterCard *bfm_chapter_card_load(const MusashiDiscMedia *disc);
BfmChapterCard *bfm_chapter_two_card_load(const MusashiDiscMedia *disc);
void bfm_chapter_card_free(BfmChapterCard *card);
/* Timing follows the mounted exterior (0) or interior (1) that requested the scene change. */
int bfm_chapter_card_draw(const BfmChapterCard *card, unsigned elapsed, unsigned source_member, uint32_t *pixels);
typedef enum {
    BFM_CHAPTER_ERROR = -1, BFM_CHAPTER_FINISHED, BFM_CHAPTER_CARD, BFM_CHAPTER_OUTGOING_ROOM
} BfmChapterFrame;

/* Elapsed fields start at the outgoing scene's departure event. During the
 * outgoing room, leave pixels untouched and return its five-bit fade shade.
 * The caller draws the live room and applies that shade. CARD paints the
 * display; FINISHED paints black before the caller takes its selected door. */
BfmChapterFrame bfm_chapter_departure_frame(const BfmChapterCard *card, unsigned elapsed,
                                            unsigned source_member, uint32_t *pixels,
                                            unsigned *room_shade);
#endif
