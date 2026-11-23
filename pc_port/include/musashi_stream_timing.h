#ifndef MUSASHI_STREAM_TIMING_H
#define MUSASHI_STREAM_TIMING_H

#include "musashi_stream_decode.h"

/* A new sector starts with zero-initialized timing. Query before the proposed
 * event; observe only after a successful physical effect. A count is the
 * number of separate ordinary 2-cycle charges preceding the event's 3-cycle
 * charge. Tail counts are entirely 2-cycle charges after the final effect. */
typedef struct MusashiStreamTiming {
    MusashiStreamEvent previous;
    uint32_t entry_phase, saved_phase;
    int started, previous_write;
} MusashiStreamTiming;

int musashi_stream_computations_before(const MusashiStreamTiming *, MusashiStreamEvent next,
    int writing, uint32_t write_value, unsigned *count);
void musashi_stream_timing_observe(MusashiStreamTiming *, MusashiStreamEvent event,
    int writing, uint32_t value);
int musashi_stream_tail_computations(const MusashiStreamTiming *, uint32_t guest_v0, unsigned *count);

#endif
