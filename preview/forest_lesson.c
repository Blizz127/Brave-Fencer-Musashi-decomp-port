/* SPDX-License-Identifier: MIT */
#include "forest_lesson.h"
#include "forest_lesson_guard.h"
#include "forest_lesson_pages.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_bytes.h"

int bfm_forest_lesson_enter(BfmForestLesson *lesson, const BfmForestField *field, int player_z)
{
    if (lesson->stage != BFM_LESSON_UPSTREAM || player_z < 1536) return 1;
    /* The room monitor admits the player by signed Z. Its subsequent mark
     * comes from the mounted event and is independent of the arrival X. */
    const uint8_t *goal = bfm_forest_room_bytes(field, UINT32_C(0x801d90f0), 8);
    if (!goal) return 0;
    for (unsigned i = 0; i < 3; ++i) lesson->mark[i] = s16le(goal + 2*i);
    lesson->stage = BFM_LESSON_APPROACH;
    return 1;
}

int bfm_forest_lesson_owns_player(const BfmForestLesson *lesson)
{
    return lesson->stage != BFM_LESSON_UPSTREAM && lesson->stage != BFM_LESSON_PRACTICE &&
           lesson->stage != BFM_LESSON_COMPLETE;
}

static int start_page(BfmForestLesson *lesson, const BfmForestField *field,
                      const MusashiDiscMedia *disc, BfmForestLessonPage page,
                      BfmForestLessonStage stage)
{
    lesson->page = bfm_forest_lesson_open(field, disc, page);
    if (!lesson->page) return 0;
    lesson->stage = stage;
    return 1;
}

static int turn_toward(int16_t *yaw, int facing)
{
    int delta = (facing - *yaw) & 4095;
    if (delta > 2048) delta -= 4096;
    if (!delta) return 1;
    int step = delta / 4;
    if (!step) step = delta / 2;
    if (!step) step = delta;
    *yaw = (int16_t)((*yaw + step) & 4095);
    return 0;
}

int bfm_forest_lesson_step(BfmForestLesson *lesson, BfmForestField *field,
    const MusashiDiscMedia *disc, BfmForestStatus *status, ForestVoice *voice,
    unsigned *voice_generation, int at_mark, int16_t *yaw, int cross, int reveal)
{
    if (!bfm_forest_lesson_guard_tick(bfm_forest_field_lesson_guard(field))) return 0;
    if (lesson->page) {
        const BfmForestMessageFrame *frame = bfm_forest_voice_page(
            voice, lesson->page, voice_generation, cross, reveal);
        if (!frame) return 0;
        if (lesson->stage == BFM_LESSON_DISCOVERY_TALK) turn_toward(yaw, 1024);
        if (!(frame->events & BFM_FOREST_MESSAGE_CLOSED)) return 1;
        if (lesson->stage == BFM_LESSON_AWARDING) lesson->award_closed = 1;
        bfm_forest_message_free(lesson->page);
        lesson->page = NULL;
    }

    switch (lesson->stage) {
    case BFM_LESSON_APPROACH:
        if (at_mark) lesson->stage = BFM_LESSON_TURN;
        break;
    case BFM_LESSON_TURN:
        if (!turn_toward(yaw, 2048)) break;
        if (bfm_forest_status_values(status).ability == 8)
            return start_page(lesson, field, disc, BFM_FOREST_LESSON_ALREADY_KNOWN, BFM_LESSON_KNOWN_TALK);
        lesson->stage = BFM_LESSON_LOOK;
        lesson->delay = 33;
        break;
    case BFM_LESSON_LOOK:
        if (!--lesson->delay)
            return start_page(lesson, field, disc, BFM_FOREST_LESSON_CLIFF, BFM_LESSON_CLIFF_TALK);
        break;
    case BFM_LESSON_CLIFF_TALK:
        lesson->stage = BFM_LESSON_WAIT_SOLDIER;
        lesson->delay = 33;
        break;
    case BFM_LESSON_WAIT_SOLDIER:
        if (!--lesson->delay) {
            if (!bfm_forest_field_summon_gun_guard(field)) return 0;
            ++lesson->target_generation;
            lesson->stage = BFM_LESSON_WAIT_DISCOVERY;
            lesson->delay = 65;
        }
        break;
    case BFM_LESSON_WAIT_DISCOVERY:
        if (!--lesson->delay)
            return start_page(lesson, field, disc, BFM_FOREST_LESSON_DISCOVERY, BFM_LESSON_DISCOVERY_TALK);
        break;
    case BFM_LESSON_DISCOVERY_TALK:
        return start_page(lesson, field, disc, BFM_FOREST_LESSON_INSTRUCTIONS, BFM_LESSON_TEACH);
    case BFM_LESSON_TEACH:
        lesson->stage = BFM_LESSON_PRACTICE;
        break;
    case BFM_LESSON_PRACTICE:
        if (lesson->award_ready && lesson->tether.phase == BFM_TETHER_EARNED)
            return start_page(lesson, field, disc, BFM_FOREST_LESSON_AWARD, BFM_LESSON_AWARDING);
        break;
    case BFM_LESSON_AWARDING:
        if (lesson->tether.phase != BFM_TETHER_COMMITTED) break;
        return start_page(lesson, field, disc, BFM_FOREST_LESSON_LOGS, BFM_LESSON_LOG_TALK);
    case BFM_LESSON_KNOWN_TALK:
    case BFM_LESSON_LOG_TALK:
        lesson->stage = BFM_LESSON_COMPLETE;
        break;
    default:
        break;
    }
    return 1;
}
