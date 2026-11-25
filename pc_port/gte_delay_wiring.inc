/* SPDX-License-Identifier: MIT */
/* Included beside the real GTE owner. All state below is local to its lease. */
enum { GTE_DELAY_IR1 = 1, GTE_DELAY_CONTROL2 = 2 };

static struct bfm_gte_transfer_delay *owner_delay_lane(MusashiGteOwner *owner, unsigned lane)
{
    return lane == GTE_DELAY_IR1 ? &owner->ir1_delay : &owner->control2_delay;
}

static int owner_transfer_form(uint32_t word, unsigned operation, unsigned reg)
{
    return word >> 26 == 0x12u && ((word >> 21) & 31u) == operation &&
           ((word >> 11) & 31u) == reg && !(word & 0x7ffu);
}

static void owner_forget_staged(MusashiGteOwner *owner)
{
    if (owner->delay_write_lane) {
        bfm_gte_delay_retire(owner_delay_lane(owner, owner->delay_write_lane),
                             BFM_GTE_DELAY_UNCERTAIN_WRITE, 0);
        owner->delay_write_lane = 0;
    }
}

static void owner_delay_discontinuity(MusashiGteOwner *owner)
{
    owner_forget_staged(owner);
    bfm_gte_delay_retire(&owner->ir1_delay, BFM_GTE_DELAY_DISTURBED, 0);
    bfm_gte_delay_retire(&owner->control2_delay, BFM_GTE_DELAY_DISTURBED, 0);
}

/* Only the authenticated hardware transfer can start a measured interval.
 * Other writes still reach the canonical bank under the existing guards. */
static void owner_stage_transfer(MusashiGteOwner *owner, const MusashiCpuContext *context,
                                 unsigned lane, uint32_t prior)
{
    const unsigned op = lane == GTE_DELAY_IR1 ? 4u : 6u;
    const unsigned reg = lane == GTE_DELAY_IR1 ? 9u : 2u;
    owner_forget_staged(owner);
    if (!context->instruction_valid || !context->gpr ||
        !owner_transfer_form(context->instruction, op, reg)) {
        bfm_gte_delay_retire(owner_delay_lane(owner, lane), BFM_GTE_DELAY_DISTURBED, 0);
        return;
    }
    owner->delay_write_lane = (uint8_t)lane;
    owner->delay_write_pc = context->pc;
    owner->delay_write_word = context->instruction;
    owner->delay_write_prior = prior;
}

/* Returns 1 when the old value was supplied, 0 for the ordinary bank read,
 * -1 when the early read is outside the observed timing profile. */
static int owner_early_read(MusashiGteOwner *owner, const MusashiCpuContext *context,
                            unsigned lane, uint32_t *value)
{
    const struct bfm_gte_transfer_delay *delay = owner_delay_lane(owner, lane);
    if (!delay->remaining && owner->delay_write_lane != lane) return 0;
    if (!context->instruction_valid || !context->gpr) return -1;
    const uint32_t word = context->instruction;
    const int store_ir1 = lane == GTE_DELAY_IR1 && word >> 26 == 0x3au &&
                          ((word >> 16) & 31u) == 9u;
    const int cpu_read = lane == GTE_DELAY_IR1 ? owner_transfer_form(word, 0, 9)
                                              : owner_transfer_form(word, 2, 2);
    if (!store_ir1 && !cpu_read) return 0;
    if (owner->delay_write_lane == lane) return -1;
    /* The controlled cached profile is available only with an independent
     * source witness. A refetch or a cached-segment address does not prove it. */
    const int immediate_ir1 = lane == GTE_DELAY_IR1 && delay->remaining == 2u;
    return bfm_gte_visible_transfer(delay, store_ir1 || immediate_ir1 || context->gte_cached_read == 1,
                                    0, value) ? 1 : -1;
}

static void owner_retire_lane(struct bfm_gte_transfer_delay *delay, unsigned lane, int writer,
                              uint32_t prior, uint32_t word)
{
    if (writer) {
        bfm_gte_delay_retire(delay, BFM_GTE_DELAY_MEASURED_WRITE, prior);
    } else if (delay->remaining) {
        const unsigned primary = word >> 26;
        const unsigned rt = (word >> 16) & 31u;
        const int data_write = lane == GTE_DELAY_IR1 &&
            (owner_transfer_form(word, 4, 28) || (primary == 0x32u && (rt == 9u || rt == 28u)));
        if ((word >> 25) == 0x25u || data_write) {
            bfm_gte_delay_retire(delay, BFM_GTE_DELAY_DISTURBED, 0);
            return;
        }
        /* The early hardware experiment interposes a NOP. A different
         * retired instruction counts, but its elapsed timing is unproven. */
        if (word != 0) delay->prior_known = false;
        bfm_gte_delay_retire(delay, BFM_GTE_DELAY_QUIET, 0);
    }
}

void musashi_gte_owner_cpu_retired(MusashiGteOwner *owner, const void *identity,
                                   uint32_t pc, uint32_t word, int ordinary)
{
    if (!owner || !owner->initialized || owner->faulted || owner->executing ||
        identity != owner->context_identity ||
        (!owner->delay_write_lane && !owner->ir1_delay.remaining && !owner->control2_delay.remaining)) return;
    if (!owner_begin(owner, 0)) return;
    if (owner->faulted || owner->reentry_violation) {
        owner->executing = 0;
        return;
    }
    if (!ordinary || (owner->delay_write_lane &&
        (owner->delay_write_pc != pc || owner->delay_write_word != word))) {
        owner_delay_discontinuity(owner);
    } else {
        const unsigned lane = owner->delay_write_lane;
        owner_retire_lane(&owner->ir1_delay, GTE_DELAY_IR1, lane == GTE_DELAY_IR1,
                          owner->delay_write_prior, word);
        owner_retire_lane(&owner->control2_delay, GTE_DELAY_CONTROL2, lane == GTE_DELAY_CONTROL2,
                          owner->delay_write_prior, word);
        owner->delay_write_lane = 0;
    }
    owner->executing = 0;
}
