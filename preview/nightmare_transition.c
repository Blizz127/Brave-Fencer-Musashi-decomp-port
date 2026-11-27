/* SPDX-License-Identifier: MIT */
#include "nightmare_transition.h"
#include "bedroom_score.h"

enum {
    IDLE, REVEAL, WAIT_FOR_VOICE, EXIT_SETUP, EXIT_DELAY, WAIT_FOR_PAGE,
    CAPTURE, SHUTTER, WAIT_FOR_MOUNT, TRANSFERRED
};
enum { STRIP_HEIGHT = 8, STRIP_PAIRS = 15, LAST_WAVE = 44 };

int bfm_nightmare_transition_open(const MusashiDiscMedia *disc, BfmNightmareTransition *t)
{
    uint8_t increment;
    if (!t || !disc || !bfm_bedroom_resident_data(disc, UINT32_C(0x80062ba0) + 2,
                                                   &increment, 1) || !increment) return 0;
    *t = (BfmNightmareTransition){.increment = increment};
    return 1;
}

void bfm_nightmare_transition_begin_arrival(BfmNightmareTransition *t)
{
    *t = (BfmNightmareTransition){.stage = REVEAL, .increment = t->increment};
}

void bfm_nightmare_transition_begin_departure(BfmNightmareTransition *t)
{
    *t = (BfmNightmareTransition){.stage = WAIT_FOR_VOICE, .increment = t->increment};
}

int bfm_nightmare_transition_live(const BfmNightmareTransition *t)
{
    return t->stage == IDLE;
}

static int advance_wipe(BfmNightmareTransition *t)
{
    t->progress += t->increment;
    if (t->progress < 255) return 0;
    t->progress = 255;
    return 1;
}

static BfmNightmareTransitionEvent advance_shutter(BfmNightmareTransition *t, int destination_ready)
{
    /* Capture four quadrants and install thirty eight-line strips before the
     * first active pass. Every other pass darkens the enabled strip pairs. */
    ++t->shutter_call;
    if (t->shutter_call < 4 || t->shutter_call % 2) return BFM_NIGHTMARE_TRANSITION_NONE;
    unsigned wave = (t->shutter_call - 4) / 2;
    if (wave == LAST_WAVE) {
        if (destination_ready) {
            /* The loader leaves before submitting this final strip list. */
            t->stage = TRANSFERRED;
            return BFM_NIGHTMARE_ENTER_VILLAGE;
        }
        t->stage = WAIT_FOR_MOUNT;
    }
    t->wave = wave;
    return BFM_NIGHTMARE_TRANSITION_NONE;
}

BfmNightmareTransitionEvent bfm_nightmare_transition_step(BfmNightmareTransition *t,
                int optical_idle, int capture_page_ready, int destination_ready)
{
    switch (t->stage) {
    case REVEAL:
        if (advance_wipe(t)) {
            t->stage = IDLE;
            return BFM_NIGHTMARE_REVEAL_COMPLETE;
        }
        break;
    case WAIT_FOR_VOICE:
        if (optical_idle) {
            t->stage = EXIT_SETUP;
            return BFM_NIGHTMARE_PREPARE_VILLAGE;
        }
        if (!t->pause_requested) {
            t->pause_requested = 1;
            return BFM_NIGHTMARE_STOP_OPTICAL;
        }
        break;
    case EXIT_SETUP:
        t->progress = 10;
        t->stage = EXIT_DELAY;
        return BFM_NIGHTMARE_EXIT_CAMERA;
    case EXIT_DELAY:
        if (t->progress) --t->progress;
        else {
            t->stage = WAIT_FOR_PAGE;
            return BFM_NIGHTMARE_RELEASE_ROOM_AUDIO;
        }
        break;
    case WAIT_FOR_PAGE:
        /* This branch of the common exit advances a counter without drawing
         * a uniform quad. The following loader owns the visible shutter. */
        if (advance_wipe(t)) {
            t->stage = CAPTURE;
            return BFM_NIGHTMARE_CAPTURE_OUTGOING;
        }
        break;
    case CAPTURE:
        if (!capture_page_ready) break;
        t->stage = SHUTTER;
        return advance_shutter(t, destination_ready);
    case SHUTTER:
        return advance_shutter(t, destination_ready);
    case WAIT_FOR_MOUNT:
        if (destination_ready) {
            t->stage = TRANSFERRED;
            return BFM_NIGHTMARE_ENTER_VILLAGE;
        }
        /* The last active strip list is resubmitted while loading. */
        if (t->wave < 31 + STRIP_PAIRS - 1) ++t->wave;
        break;
    }
    return BFM_NIGHTMARE_TRANSITION_NONE;
}

void bfm_nightmare_transition_draw(const BfmNightmareTransition *t, uint32_t pixels[240][320])
{
    int revealing = t->stage == REVEAL;
    if (!revealing && !t->wave) return;
    for (unsigned y = 0; y < 240; ++y) {
        unsigned loss;
        if (revealing) {
            loss = (255 - t->progress) >> 3;
        } else {
            unsigned edge = y < 120 ? y : 239 - y;
            unsigned pair = edge / STRIP_HEIGHT;
            if (t->wave <= pair) continue;
            loss = t->wave - pair;
        }
        if (!loss) continue;
        for (unsigned x = 0; x < 320; ++x) {
            uint32_t in = pixels[y][x], out = UINT32_C(0xff000000);
            for (unsigned shift = 0; shift <= 16; shift += 8) {
                unsigned level = ((in >> shift) & 255u) >> 3;
                if (level > loss) {
                    level -= loss;
                    out |= (level << 3 | level >> 2) << shift;
                }
            }
            pixels[y][x] = out;
        }
    }
}
