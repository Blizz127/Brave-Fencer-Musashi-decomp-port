/* SPDX-License-Identifier: MIT */
#ifndef BFM_MIPS_MOUNTED_SCALAR_H
#define BFM_MIPS_MOUNTED_SCALAR_H

#include "mips_scalar_memory.h"
#include <string.h>

/* The adapter authenticates the original instruction against the live mount,
 * the executing CPU and owner. It samples operands at issue. Its revision must
 * change when a binding is replaced, including replacement with equal bytes.
 * A clocked sample must also attest that the same instruction reservation
 * actually paid its required cycles in the active scheduler.
 * It must not advance any device, perform a data read, or initiate guest I/O. */
struct bfm_scalar_issue {
    const void *cpu;
    const void *image;
    const void *provider;
    const void *owner;
    uint64_t revision;
    uint32_t pc;
    uint32_t next_pc;
    uint32_t instruction;
    bool delay_slot;
    unsigned delayed_target; /* Zero denotes no pending GPR write. */
    uint32_t delayed_value;
    uint32_t regs[32];
    void *clock_context;
    int (*advance)(void *, uint32_t);
    uint32_t cycles;
};

struct bfm_scalar_mapping {
    const void *region;
    uint64_t revision;
    uint32_t physical_address;
    uint32_t first; /* Inclusive physical grant for this region. */
    uint32_t last;
    void *bus_context;
    int (*admit)(void *, uint32_t address, unsigned width, int writing);
    int (*read)(void *, uint32_t address, unsigned width, uint32_t *value);
    int (*write)(void *, uint32_t address, unsigned width, uint32_t value,
                 unsigned byte_enables);
};

struct bfm_scalar_owner {
    void *context;
    bool (*sample)(void *, bool clocked, struct bfm_scalar_issue *);
    /* Pure translation and permission lookup. Prove the entire guest access
     * maps contiguously to physical_address within one permitted region. */
    bool (*map)(void *, const struct bfm_scalar_issue *,
                const struct bfm_scalar_memory *, struct bfm_scalar_mapping *);
};

enum bfm_scalar_status {
    BFM_SCALAR_DENIED,        /* No charge and no bus transaction. */
    BFM_SCALAR_OTHER_OPCODE,  /* Dispatch to the other authorized handler. */
    BFM_SCALAR_LOAD_ADDRESS,  /* Caller handles a misaligned guest load. */
    BFM_SCALAR_STORE_ADDRESS, /* Caller handles a misaligned guest store. */
    BFM_SCALAR_CLOCK_VETO,    /* No bus transaction. */
    BFM_SCALAR_CLOCKED_STALE, /* Charge succeeded, no bus transaction. */
    BFM_SCALAR_CLOCKED_BUS_FAILURE,
    BFM_SCALAR_COMPLETE,
    BFM_SCALAR_PREPARED,
    BFM_SCALAR_NO_RESERVATION
};

/* Operate in the single CPU executor. A COMPLETE result retires exactly once
 * with the caller's normal PC, branch-delay and delayed-write pipeline. No new
 * load is queued for target zero. A read of target zero still accesses the
 * bus. On other statuses no new GPR result is produced. A clocked failure
 * must retain the paid clock in the scheduler; it cannot blindly retry. */
struct bfm_scalar_result {
    enum bfm_scalar_status status;
    uint32_t address; /* Original virtual byte address, including on a trap. */
    unsigned delayed_target;
    uint32_t delayed_value;
};

static inline bool bfm_scalar_same_issue(const struct bfm_scalar_issue *a,
                                         const struct bfm_scalar_issue *b)
{
    return a->cpu == b->cpu && a->image == b->image &&
           a->provider == b->provider && a->owner == b->owner &&
           a->revision == b->revision &&
           a->pc == b->pc && a->next_pc == b->next_pc &&
           a->instruction == b->instruction && a->delay_slot == b->delay_slot &&
           a->delayed_target == b->delayed_target &&
           (!a->delayed_target || a->delayed_value == b->delayed_value) &&
           memcmp(a->regs, b->regs, sizeof(a->regs)) == 0 &&
           a->clock_context == b->clock_context && a->advance == b->advance &&
           a->cycles == b->cycles;
}

static inline bool bfm_scalar_allowed(const struct bfm_scalar_mapping *mapping,
                                      const struct bfm_scalar_memory *access)
{
    const uint32_t address = mapping->physical_address;
    const unsigned width = access->width;
    if (!mapping->region || !mapping->admit ||
        (access->store ? mapping->write == NULL : mapping->read == NULL) ||
        (address & (width - 1u)) != 0 ||
        address < mapping->first || address > mapping->last ||
        width - 1u > mapping->last - address)
        return false;
    return mapping->admit(mapping->bus_context, address, width, access->store) != 0;
}

static inline bool bfm_scalar_same_mapping(const struct bfm_scalar_mapping *a,
                                           const struct bfm_scalar_mapping *b)
{
    return a->region == b->region && a->revision == b->revision &&
           a->physical_address == b->physical_address &&
           a->first == b->first && a->last == b->last &&
           a->bus_context == b->bus_context && a->admit == b->admit &&
           a->read == b->read && a->write == b->write;
}

/* A single-executor reservation. Keep it local to one active scheduler issue.
 * Consume it once after the real clock pays the bound instruction. Discard it
 * on a veto or on cancellation. The adapter's clocked sample authenticates
 * that payment against the current scheduler, even for the split entry point. */
struct bfm_scalar_prepared {
    bool ready;
    const struct bfm_scalar_owner *executor;
    struct bfm_scalar_owner bound;
    struct bfm_scalar_issue issue;
    struct bfm_scalar_mapping mapping;
    struct bfm_scalar_memory access;
};

static inline struct bfm_scalar_result
bfm_prepare_mounted_scalar(const struct bfm_scalar_owner *owner,
                           struct bfm_scalar_prepared *step)
{
    struct bfm_scalar_result result = { BFM_SCALAR_DENIED, 0, 0, 0 };
    if (!step)
        return result;
    step->ready = false;
    if (!owner || !owner->sample || !owner->map)
        return result;
    step->executor = owner;
    step->bound = *owner;
    struct bfm_scalar_issue first = {0};
    if (!step->bound.sample(step->bound.context, false, &first) ||
        !first.cpu || !first.image || !first.provider || !first.owner ||
        !first.advance || !first.cycles || (first.pc & 3u) != 0)
        return result;

    if (!bfm_decode_scalar_memory(first.instruction, first.regs, &step->access)) {
        result.status = BFM_SCALAR_OTHER_OPCODE;
        return result;
    }
    result.address = step->access.address;
    if ((result.address & (step->access.width - 1u)) != 0) {
        result.status = step->access.store ? BFM_SCALAR_STORE_ADDRESS : BFM_SCALAR_LOAD_ADDRESS;
        return result;
    }

    struct bfm_scalar_mapping mapping = {0};
    if (!step->bound.map(step->bound.context, &first, &step->access, &mapping) ||
        !bfm_scalar_allowed(&mapping, &step->access))
        return result;
    step->issue = first;
    step->mapping = mapping;
    step->ready = true;
    result.status = BFM_SCALAR_PREPARED;
    return result;
}

/* A device transaction is one width-specific operation. The bus backend
 * checks its own authority and must fail atomically. It may not reenter
 * guest execution between the validated transfer and normal retirement. */
static inline struct bfm_scalar_result
bfm_issue_mounted_scalar(const struct bfm_scalar_owner *owner,
                         struct bfm_scalar_prepared *step)
{
    struct bfm_scalar_result result = { BFM_SCALAR_NO_RESERVATION, 0, 0, 0 };
    if (!step || !step->ready)
        return result;
    step->ready = false;
    result.address = step->access.address;
    struct bfm_scalar_memory access;
    const struct bfm_scalar_owner *bound = &step->bound;
    struct bfm_scalar_issue current = {0};
    struct bfm_scalar_mapping after = {0};
    result.status = BFM_SCALAR_CLOCKED_STALE;
    if (owner != step->executor || owner->context != bound->context ||
        owner->sample != bound->sample || owner->map != bound->map ||
        !bound->sample(bound->context, true, &current) ||
        !bfm_scalar_same_issue(&step->issue, &current) ||
        !bfm_decode_scalar_memory(current.instruction, current.regs, &access) ||
        access.address != step->access.address || access.width != step->access.width ||
        access.target != step->access.target || access.store != step->access.store ||
        access.signed_load != step->access.signed_load ||
        access.store_value != step->access.store_value ||
        !bound->map(bound->context, &current, &access, &after) ||
        !bfm_scalar_same_mapping(&step->mapping, &after) ||
        !bfm_scalar_allowed(&after, &access))
        return result;

    const uint32_t physical = after.physical_address;
    const unsigned width = access.width;
    result.status = BFM_SCALAR_CLOCKED_BUS_FAILURE;
    if (access.store) {
        /* The byte-device backend accepts the full GPR payload. Its real
         * width-specific method applies RAM lane masking or device semantics. */
        const unsigned byte_enables = (1u << width) - 1u;
        if (!after.write(after.bus_context, physical, width, access.store_value, byte_enables))
            return result;
    } else {
        uint32_t value;
        if (!after.read(after.bus_context, physical, width, &value))
            return result;
        if (access.target) {
            result.delayed_target = access.target;
            result.delayed_value = bfm_scalar_load_value(&access, value);
        }
    }
    result.status = BFM_SCALAR_COMPLETE;
    return result;
}

static inline struct bfm_scalar_result
bfm_run_mounted_scalar(const struct bfm_scalar_owner *owner)
{
    struct bfm_scalar_prepared step;
    struct bfm_scalar_result result = bfm_prepare_mounted_scalar(owner, &step);
    if (result.status != BFM_SCALAR_PREPARED)
        return result;
    if (!step.issue.advance(step.issue.clock_context, step.issue.cycles)) {
        result.status = BFM_SCALAR_CLOCK_VETO;
        return result;
    }
    return bfm_issue_mounted_scalar(owner, &step);
}
#endif
