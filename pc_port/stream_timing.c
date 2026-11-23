#include "musashi_stream_timing.h"

static int input_token(MusashiStreamEvent event) {
    return event == MUSASHI_STREAM_LITERAL || event == MUSASHI_STREAM_MATCH_LOW;
}

int musashi_stream_computations_before(const MusashiStreamTiming *t, MusashiStreamEvent next,
    int writing, uint32_t value, unsigned *count) {
    unsigned issues = 0;
    if (!t || !count) return 0;
    if (!t->started) {
        if (!writing && next == MUSASHI_STREAM_SAVED_CURSOR) issues = 4;
    } else if (t->previous_write) {
        if (t->previous == MUSASHI_STREAM_LITERAL_RING && writing && next == MUSASHI_STREAM_LITERAL_OUTPUT)
            issues = 1;
        else if (t->previous == MUSASHI_STREAM_MATCH_RING && writing && next == MUSASHI_STREAM_MATCH_OUTPUT)
            issues = 1;
        else if (t->previous == MUSASHI_STREAM_LITERAL_OUTPUT || t->previous == MUSASHI_STREAM_MATCH_OUTPUT) {
            if (!writing && (next == MUSASHI_STREAM_MATCH_SOURCE || next == MUSASHI_STREAM_NEXT_FLAGS))
                issues = 9;
            else if (writing && next == MUSASHI_STREAM_SAVED_PHASE && value == 4) issues = 8;
            else if (!writing && input_token(next)) issues = 11;
        } else if (writing && t->saved_phase >= 2 && t->saved_phase <= 4) {
            switch (t->previous) {
            case MUSASHI_STREAM_SAVED_PHASE: if (next == MUSASHI_STREAM_SAVED_CURSOR) issues = 2; break;
            case MUSASHI_STREAM_SAVED_CURSOR: if (next == MUSASHI_STREAM_SAVED_MASK) issues = 2; break;
            case MUSASHI_STREAM_SAVED_MASK: if (next == MUSASHI_STREAM_SAVED_FLAGS) issues = 2; break;
            case MUSASHI_STREAM_SAVED_FLAGS: if (next == MUSASHI_STREAM_SAVED_DESTINATION) issues = 2; break;
            case MUSASHI_STREAM_SAVED_DESTINATION: if (next == MUSASHI_STREAM_SAVED_CODE) issues = 2; break;
            default: break;
            }
        }
    } else {
        switch (t->previous) {
        case MUSASHI_STREAM_SAVED_CURSOR: if (!writing && next == MUSASHI_STREAM_SAVED_MASK) issues = 2; break;
        case MUSASHI_STREAM_SAVED_MASK: if (!writing && next == MUSASHI_STREAM_SAVED_FLAGS) issues = 2; break;
        case MUSASHI_STREAM_SAVED_FLAGS: if (!writing && next == MUSASHI_STREAM_SAVED_DESTINATION) issues = 2; break;
        case MUSASHI_STREAM_SAVED_DESTINATION: if (!writing && next == MUSASHI_STREAM_SAVED_PHASE) issues = 2; break;
        case MUSASHI_STREAM_SAVED_PHASE: if (!writing && next == MUSASHI_STREAM_SAVED_CODE) issues = 2; break;
        case MUSASHI_STREAM_SAVED_CODE: if (!writing && next == MUSASHI_STREAM_DISPATCH) issues = 7; break;
        case MUSASHI_STREAM_DISPATCH:
            if (writing) break;
            if (t->entry_phase == 1 && next == MUSASHI_STREAM_FIRST_FLAGS) issues = 6;
            else if (t->entry_phase == 3 && next == MUSASHI_STREAM_MATCH_HIGH) issues = 4;
            else if (t->entry_phase == 4 && next == MUSASHI_STREAM_NEXT_FLAGS) issues = 9;
            else if (input_token(next) && (t->entry_phase == 2 || t->entry_phase == 4))
                issues = t->entry_phase == 2 ? 7u : 11u;
            break;
        case MUSASHI_STREAM_FIRST_FLAGS: if (!writing && input_token(next)) issues = 6; break;
        case MUSASHI_STREAM_NEXT_FLAGS:
            if ((!writing && input_token(next)) || (writing && next == MUSASHI_STREAM_SAVED_PHASE && value == 2))
                issues = 7;
            break;
        case MUSASHI_STREAM_LITERAL: if (writing && next == MUSASHI_STREAM_LITERAL_RING) issues = 5; break;
        case MUSASHI_STREAM_MATCH_SOURCE: if (writing && next == MUSASHI_STREAM_MATCH_RING) issues = 2; break;
        case MUSASHI_STREAM_MATCH_LOW:
            if (!writing && next == MUSASHI_STREAM_MATCH_HIGH) issues = 6;
            else if (writing && next == MUSASHI_STREAM_SAVED_PHASE && value == 3) issues = 10;
            break;
        case MUSASHI_STREAM_MATCH_HIGH:
            if (!writing && next == MUSASHI_STREAM_MATCH_SOURCE) issues = 19;
            else if (writing && next == MUSASHI_STREAM_SAVED_PHASE && value == 0) issues = 10;
            break;
        default: break;
        }
    }
    if (!issues) return 0;
    *count = issues - 1u;
    return 1;
}

void musashi_stream_timing_observe(MusashiStreamTiming *t, MusashiStreamEvent event,
    int writing, uint32_t value) {
    t->started = 1;
    t->previous = event;
    t->previous_write = writing;
    if (event == MUSASHI_STREAM_SAVED_PHASE) {
        if (writing) t->saved_phase = value;
        else t->entry_phase = value;
    }
}

int musashi_stream_tail_computations(const MusashiStreamTiming *t, uint32_t guest_v0,
    unsigned *count) {
    if (!t || !count || !t->started) return 0;
    if (!guest_v0 && !t->previous_write && t->previous == MUSASHI_STREAM_SAVED_CODE &&
        t->entry_phase >= MUSASHI_STREAM_STATE_COUNT) *count = 5;
    else if (!guest_v0 && !t->previous_write && t->previous == MUSASHI_STREAM_DISPATCH &&
             t->entry_phase == 0) *count = 7;
    else if (!guest_v0 && t->previous_write && t->previous == MUSASHI_STREAM_SAVED_PHASE &&
             t->saved_phase == 0) *count = 4;
    else if (guest_v0 == 1 && t->previous_write && t->previous == MUSASHI_STREAM_SAVED_CODE &&
             t->saved_phase >= 2 && t->saved_phase <= 4) *count = t->saved_phase == 2 ? 2u : 4u;
    else return 0;
    return 1;
}
