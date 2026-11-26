/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_LESSON_PAGES_H
#define BFM_FOREST_LESSON_PAGES_H
#include "forest_field.h"
#include "forest_message.h"

typedef enum {
    BFM_FOREST_LESSON_CLIFF,
    BFM_FOREST_LESSON_ALREADY_KNOWN,
    BFM_FOREST_LESSON_DISCOVERY,
    BFM_FOREST_LESSON_INSTRUCTIONS,
    BFM_FOREST_LESSON_AWARD,
    BFM_FOREST_LESSON_LOGS
} BfmForestLessonPage;

/* The room/assimilation owner selects a page at its observed transition.
 * Scene pages borrow the live room. The award reads the mounted ability
 * catalogue and keeps its own copy. Free either with bfm_forest_message_free. */
BfmForestMessage *bfm_forest_lesson_open(const BfmForestField *field, const MusashiDiscMedia *disc,
                                        BfmForestLessonPage page);
#endif
