/* SPDX-License-Identifier: MIT */
#ifndef BFM_CHAPTER_COMPLETION_VIEW_H
#define BFM_CHAPTER_COMPLETION_VIEW_H

#include "chapter_card.h"
#include "chapter_completion.h"

/* Fields count from the start of the displayed page. The bedroom supplies
 * its own field scene after the Chapter 2 crest. */
int bfm_chapter_completion_draw(const BfmChapterCompletion *state, const BfmSaveArt *art,
                                const BfmChapterCard *title, unsigned fields,
                                uint32_t pixels[240][320]);

#endif
