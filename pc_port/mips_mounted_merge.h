/* SPDX-License-Identifier: MIT */
#ifndef BFM_MIPS_MOUNTED_MERGE_H
#define BFM_MIPS_MOUNTED_MERGE_H

#include "mips_mounted_scalar.h"
#include "mips_word_merge.h"

struct bfm_merge_owner {
    void *context;
    bool (*sample)(void *, bool clocked, struct bfm_scalar_issue *);
    /* Resolve the ORIGINAL virtual byte address and all four aligned transfer
     * bytes in the same authorized region. Admit the exact enabled store lanes
     * (including for MMIO). Resolution must be side-effect free. */
    bool (*resolve)(void *, const struct bfm_scalar_issue *,
                    const struct bfm_word_merge *, unsigned enables,
                    struct bfm_scalar_mapping *);
};

struct bfm_merge_ticket {
    const struct bfm_merge_owner *issuer;
    struct bfm_merge_owner owner;
    struct bfm_scalar_issue issue;
    struct bfm_scalar_mapping grant;
    struct bfm_word_merge access;
    struct bfm_merge_store_word lanes;
};

static inline bool
bfm_merge_grant_allows(const struct bfm_scalar_mapping *grant, bool store)
{
    struct bfm_scalar_memory aligned = { .width = 4, .store = store };
    return bfm_scalar_allowed(grant, &aligned);
}

/* Prepare never charges the clock or accesses guest/device data. The caller
 * uses its ordinary clock and budget; only issue on successful payment. */
static inline enum bfm_scalar_status
bfm_mounted_merge_prepare(const struct bfm_merge_owner *owner,
                          struct bfm_merge_ticket *ticket)
{
    if (!ticket) return BFM_SCALAR_DENIED;
    *ticket = (struct bfm_merge_ticket){0};
    if (!owner || !owner->sample || !owner->resolve) return BFM_SCALAR_DENIED;
    ticket->owner = *owner;
    struct bfm_scalar_issue *issue = &ticket->issue;
    if (!owner->sample(owner->context, false, issue) || !issue->cpu ||
        !issue->image || !issue->owner || !issue->provider || !issue->advance ||
        !issue->cycles || (issue->pc & 3u)) return BFM_SCALAR_DENIED;
    const struct bfm_merge_pending_load pending = {
        .target = issue->delayed_target, .value = issue->delayed_value
    };
    if (!bfm_decode_word_merge(issue->instruction, issue->regs, &pending, &ticket->access))
        return BFM_SCALAR_OTHER_OPCODE;
    if (ticket->access.store) ticket->lanes = bfm_merge_store_lanes(&ticket->access);
    else ticket->lanes.byte_enable = 15;
    if (!owner->resolve(owner->context, issue, &ticket->access, ticket->lanes.byte_enable,
                        &ticket->grant) ||
        !bfm_merge_grant_allows(&ticket->grant, ticket->access.store))
        return BFM_SCALAR_DENIED;
    ticket->issuer = owner;
    return BFM_SCALAR_PREPARED;
}

/* Consume exactly once after the common scheduler clock returns. Paid refusals
 * never replay a bus action. Retirement remains with the issuing CPU. */
static inline struct bfm_scalar_result
bfm_mounted_merge_issue(const struct bfm_merge_owner *owner,
                        struct bfm_merge_ticket *ticket)
{
    struct bfm_scalar_result r = { .status = BFM_SCALAR_CLOCKED_STALE };
    if (!ticket || !ticket->issuer) {
        r.status = BFM_SCALAR_NO_RESERVATION;
        return r;
    }
    const struct bfm_merge_owner *issuer = ticket->issuer;
    ticket->issuer = NULL;
    r.address = ticket->access.byte_address;
    if (!owner || owner != issuer || owner->context != ticket->owner.context ||
        owner->sample != ticket->owner.sample || owner->resolve != ticket->owner.resolve)
        return r;

    struct bfm_scalar_issue live = {0};
    struct bfm_scalar_mapping grant = {0};
    if (!owner->sample(owner->context, true, &live) ||
        !bfm_scalar_same_issue(&ticket->issue, &live)) return r;
    const struct bfm_merge_pending_load pending = { live.delayed_target, live.delayed_value };
    struct bfm_word_merge access;
    if (!bfm_decode_word_merge(live.instruction, live.regs, &pending, &access) ||
        access.byte_address != ticket->access.byte_address ||
        access.aligned_address != ticket->access.aligned_address ||
        access.operand != ticket->access.operand || access.target != ticket->access.target ||
        access.shift != ticket->access.shift || access.store != ticket->access.store ||
        access.left != ticket->access.left) return r;
    const struct bfm_merge_store_word lanes = access.store ? bfm_merge_store_lanes(&access)
        : (struct bfm_merge_store_word){ .byte_enable = 15u };
    if (lanes.byte_enable != ticket->lanes.byte_enable || lanes.data != ticket->lanes.data ||
        !owner->resolve(owner->context, &live, &access, lanes.byte_enable, &grant) ||
        !bfm_scalar_same_mapping(&ticket->grant, &grant) ||
        !bfm_merge_grant_allows(&grant, ticket->access.store)) return r;

    r.status = BFM_SCALAR_CLOCKED_BUS_FAILURE;
    if (ticket->access.store) {
        if (!grant.write(grant.bus_context, grant.physical_address, 4,
                         ticket->lanes.data, ticket->lanes.byte_enable)) return r;
    } else {
        uint32_t word;
        if (!grant.read(grant.bus_context, grant.physical_address, 4, &word)) return r;
        if (ticket->access.target) {
            r.delayed_target = ticket->access.target;
            r.delayed_value = bfm_merge_loaded_word(&ticket->access, word);
        }
    }
    r.status = BFM_SCALAR_COMPLETE;
    return r;
}
#endif
