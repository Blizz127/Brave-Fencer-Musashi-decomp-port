#ifndef MUSASHI_MDU_H
#define MUSASHI_MDU_H

#include "musashi_boot_memory.h"

/* The physical CPU owns HI/LO readiness across interrupted guest frames.
 * An existing relative cycle counter may supply ordinary paid time. Report
 * separately paid time outside that counter once with musashi_mdu_elapsed. */
typedef struct MusashiMduTiming {
    const MusashiBootMemory *memory;
    const MusashiExecutionClock *clock;
    MusashiExecutionClock issuer;
    const uint64_t *cycles;
    uint64_t additional_cycles;
    uint64_t produced_at;
    uint64_t generation;
    uint32_t duration;
    struct MusashiMduTiming *previous;
} MusashiMduTiming;

/* Bind on the machine's execution thread for the lifetime of a run. Distinct
 * nested machines may have independent bindings. Legacy callers need none. */
int musashi_mdu_bind(MusashiMduTiming *, const MusashiBootMemory *,
    const MusashiExecutionClock *, const uint64_t *cycles);
void musashi_mdu_unbind(MusashiMduTiming *);

static inline void musashi_mdu_elapsed(MusashiMduTiming *timing, uint64_t cycles) {
    timing->additional_cycles += cycles;
}

static inline uint64_t musashi_mdu_now(const MusashiMduTiming *timing) {
    return timing->additional_cycles + (timing->cycles ? *timing->cycles : 0);
}

static inline uint32_t musashi_mdu_remaining(const MusashiMduTiming *timing) {
    const uint64_t elapsed = musashi_mdu_now(timing) - timing->produced_at;
    return elapsed < timing->duration ? timing->duration - (uint32_t)elapsed : 0u;
}

#endif
