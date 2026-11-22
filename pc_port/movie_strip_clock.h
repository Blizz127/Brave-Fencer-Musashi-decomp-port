#ifndef MUSASHI_MOVIE_STRIP_CLOCK_H
#define MUSASHI_MOVIE_STRIP_CLOCK_H

#include "musashi_guest_irq.h"

/* Each entry is the clock cost of an instruction in the independently
 * compiled game callback. Arithmetic, scalar access and paired stores have
 * different costs. Keeping the charges separate preserves flush boundaries
 * in the shared device clock. Neither device helpers nor trap paths are
 * included here. */
typedef enum MovieStripPart {
    MOVIE_STRIP_ENTER,
    MOVIE_STRIP_CHECK_CD_PENDING,
    MOVIE_STRIP_CALL_CD,
    MOVIE_STRIP_RETURN_CD,
    MOVIE_STRIP_SNAPSHOT,
    MOVIE_STRIP_FLIP_BUFFER,
    MOVIE_STRIP_DIVIDE,
    MOVIE_STRIP_CHECK_DIVISOR,
    MOVIE_STRIP_DIVISOR_MINUS_ONE,
    MOVIE_STRIP_CHECK_REMAINDER,
    MOVIE_STRIP_ADVANCE_REMAINDER,
    MOVIE_STRIP_ADVANCE_WIDTH,
    MOVIE_STRIP_CHECK_PAGE,
    MOVIE_STRIP_PREPARE_REARM,
    MOVIE_STRIP_REARM_NONPOSITIVE_HEIGHT,
    MOVIE_STRIP_CALL_REARM,
    MOVIE_STRIP_RETURN_REARM,
    MOVIE_STRIP_FINISH_PAGE,
    MOVIE_STRIP_CALL_UPLOAD,
    MOVIE_STRIP_RETURN_UPLOAD,
    MOVIE_STRIP_PART_COUNT
} MovieStripPart;

typedef struct MovieStripTiming {
    uint32_t start_pc;
    uint8_t length;
    uint8_t cycles[19];
} MovieStripTiming;

static const MovieStripTiming movie_strip_timing[MOVIE_STRIP_PART_COUNT] = {
    [MOVIE_STRIP_ENTER] = {0x800d24d0u, 6,
        {2, 3, 2, 3, 2, 3}},
    [MOVIE_STRIP_CHECK_CD_PENDING] = {0x800d24e8u, 5,
        {2, 3, 2, 2, 2}},
    [MOVIE_STRIP_CALL_CD] = {0x800d24fcu, 2,
        {2, 2}},
    [MOVIE_STRIP_RETURN_CD] = {0x800d2504u, 2,
        {2, 3}},
    [MOVIE_STRIP_SNAPSHOT] = {0x800d250cu, 10,
        {2, 2, 3, 3, 3, 3, 4, 4, 4, 4}},
    [MOVIE_STRIP_FLIP_BUFFER] = {0x800d2534u, 9,
        {2, 2, 3, 3, 2, 3, 2, 2, 3}},
    [MOVIE_STRIP_DIVIDE] = {0x800d2558u, 12,
        {2, 3, 2, 3, 2, 2, 2, 3, 2, 2, 2, 2}},
    [MOVIE_STRIP_CHECK_DIVISOR] = {0x800d258cu, 3,
        {2, 2, 2}},
    [MOVIE_STRIP_DIVISOR_MINUS_ONE] = {0x800d2598u, 2,
        {2, 2}},
    [MOVIE_STRIP_CHECK_REMAINDER] = {0x800d25a4u, 4,
        {2, 2, 2, 2}},
    [MOVIE_STRIP_ADVANCE_REMAINDER] = {0x800d25b4u, 9,
        {2, 3, 2, 3, 2, 2, 3, 2, 2}},
    [MOVIE_STRIP_ADVANCE_WIDTH] = {0x800d25d8u, 8,
        {2, 2, 3, 2, 3, 2, 2, 3}},
    [MOVIE_STRIP_CHECK_PAGE] = {0x800d25f8u, 16,
        {2, 3, 2, 2, 2, 2, 2, 3, 2, 2, 3, 3, 2, 2, 2, 2}},
    [MOVIE_STRIP_PREPARE_REARM] = {0x800d2638u, 12,
        {2, 2, 3, 2, 3, 2, 3, 2, 2, 2, 2, 2}},
    [MOVIE_STRIP_REARM_NONPOSITIVE_HEIGHT] = {0x800d2668u, 1,
        {2}},
    [MOVIE_STRIP_CALL_REARM] = {0x800d266cu, 7,
        {2, 2, 2, 3, 2, 2, 2}},
    [MOVIE_STRIP_RETURN_REARM] = {0x800d2688u, 2,
        {2, 2}},
    [MOVIE_STRIP_FINISH_PAGE] = {0x800d2690u, 19,
        {2, 2, 3, 2, 2, 3, 2, 2, 3, 2, 3, 2, 2, 3, 2, 3, 2, 3, 2}},
    [MOVIE_STRIP_CALL_UPLOAD] = {0x800d26dcu, 5,
        {2, 2, 3, 2, 2}},
    [MOVIE_STRIP_RETURN_UPLOAD] = {0x800d26f0u, 5,
        {3, 3, 2, 2, 2}},
};

/* Called only for an admitted part of the callback's selected path. The
 * caller coordinates game state and each actual device service separately.
 * A rejected charge leaves the current instruction unretired. */
static int movie_strip_charge(const MusashiExecutionClock *clock,
                              MusashiGuestIrqStop *stop, MovieStripPart part)
{
    if (!stop || (unsigned)part >= MOVIE_STRIP_PART_COUNT)
        return 0;
    const MovieStripTiming *timing = &movie_strip_timing[part];
    for (unsigned i = 0; i < timing->length; ++i) {
        stop->pc = timing->start_pc + 4u * i;
        if (stop->instructions >= 8192u)
            return 0;
        if (clock && (!clock->advance ||
                      !clock->advance(clock->userdata, timing->cycles[i])))
            return 0;
        ++stop->instructions;
    }
    return 1;
}

#endif
