/* SPDX-License-Identifier: MIT */
#ifndef BFM_GTE_TRANSFER_DELAY_H
#define BFM_GTE_TRANSFER_DELAY_H

#include <stdbool.h>
#include <stdint.h>

/* Keep one per-owner instance for data IR1 (9), and a separate instance for
 * the RT22/RT23 control word (2). The bank owns the current value. A drained
 * or hardware-reset instance is zero initialized. Store with device snapshots. */
struct bfm_gte_transfer_delay {
    uint32_t prior_value;
    uint8_t remaining;
    bool prior_known;
};

enum bfm_gte_delay_effect {
    BFM_GTE_DELAY_QUIET,           /* independent, successfully retired guest instruction */
    BFM_GTE_DELAY_MEASURED_WRITE,  /* successful MTC2 IR1 or CTC2 RT22/RT23 for this lane */
    BFM_GTE_DELAY_UNCERTAIN_WRITE, /* an observed write without a qualified old value */
    BFM_GTE_DELAY_DISTURBED,       /* invalidate an active window only */
    BFM_GTE_DELAY_CANCELLED        /* no guest retirement */
};

/* Classify each genuine retirement exactly once for each enrolled lane. The
 * writer's own retirement starts its window. Capture the architectural bank
 * value before committing that write. An unobserved or disturbed interval
 * requires two fresh, witnessed quiet retirements before readback resumes. */
static inline void bfm_gte_delay_retire(struct bfm_gte_transfer_delay *delay,
                                        enum bfm_gte_delay_effect effect, uint32_t prior)
{
    if (effect == BFM_GTE_DELAY_CANCELLED ||
        (effect == BFM_GTE_DELAY_DISTURBED && !delay->remaining)) return;
    if (effect == BFM_GTE_DELAY_QUIET) {
        if (delay->remaining && --delay->remaining == 0)
            *delay = (struct bfm_gte_transfer_delay){0};
        return;
    }
    delay->prior_known = effect == BFM_GTE_DELAY_MEASURED_WRITE && delay->remaining == 0;
    delay->prior_value = delay->prior_known ? prior : 0;
    delay->remaining = 2;
}

/* Only MFC2/SWC2 IR1, or CFC2 RT22/RT23 for the corresponding lane, enter
 * here. Confirm the measured early timing regime before returning the old
 * value. The published cold MFC2/IR1 one-NOP results vary across runs. */
static inline bool bfm_gte_visible_transfer(const struct bfm_gte_transfer_delay *delay,
                                            bool early_timing_supported, uint32_t current,
                                            uint32_t *out)
{
    if (delay->remaining) {
        if (!delay->prior_known || !early_timing_supported) return false;
        *out = delay->prior_value;
    } else {
        *out = current;
    }
    return true;
}

#endif
