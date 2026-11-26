/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_LESSON_H
#define BFM_FOREST_LESSON_H
#include "forest_absorption.h"
#include "forest_message.h"
#include "forest_voice.h"

typedef enum {
    BFM_LESSON_UPSTREAM, BFM_LESSON_APPROACH, BFM_LESSON_TURN,
    BFM_LESSON_LOOK, BFM_LESSON_CLIFF_TALK, BFM_LESSON_WAIT_SOLDIER,
    BFM_LESSON_WAIT_DISCOVERY, BFM_LESSON_DISCOVERY_TALK, BFM_LESSON_TEACH,
    BFM_LESSON_PRACTICE, BFM_LESSON_AWARDING, BFM_LESSON_LOG_TALK,
    BFM_LESSON_KNOWN_TALK, BFM_LESSON_COMPLETE
} BfmForestLessonStage;

typedef struct {
    BfmForestLessonStage stage;
    int16_t mark[3];
    unsigned delay, target_generation;
    int award_ready, award_closed;
    BfmForestMessage *page;
    BfmForestTether tether;
} BfmForestLesson;

int bfm_forest_lesson_enter(BfmForestLesson *lesson, const BfmForestField *field, int player_z);
int bfm_forest_lesson_owns_player(const BfmForestLesson *lesson);
/* Called once after actor movement, with the live field, input and voice service. */
int bfm_forest_lesson_step(BfmForestLesson *lesson, BfmForestField *field,
    const MusashiDiscMedia *disc, BfmForestStatus *status, ForestVoice *voice,
    unsigned *voice_generation, int at_mark, int16_t *yaw, int cross, int reveal);
#endif
