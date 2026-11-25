/* SPDX-License-Identifier: MIT */
#ifndef BFM_CHAPTER_CARD_H
#define BFM_CHAPTER_CARD_H
#include "musashi_disc_media.h"
#include <stdint.h>
typedef struct BfmChapterCard BfmChapterCard;
BfmChapterCard *bfm_chapter_card_load(const MusashiDiscMedia *disc);
void bfm_chapter_card_free(BfmChapterCard *card);
/* Timing follows the mounted exterior (0) or interior (1) that requested the scene change. */
int bfm_chapter_card_draw(const BfmChapterCard *card, unsigned elapsed, unsigned source_member, uint32_t *pixels);
#endif
