/* SPDX-License-Identifier: MIT */
#ifndef BFM_FORMATTER_MOUNTED_DISPATCH_INC
#define BFM_FORMATTER_MOUNTED_DISPATCH_INC
#include "formatter_mounted_issue.inc"
#include "mips_mounted_host_bus.inc"
#include "mips_mounted_opening_scalar.inc"

struct bfm_generic_bus {
    void *context;
    int (*admit)(void *, uint32_t, unsigned, int);
    int (*read)(void *, uint32_t, unsigned, uint32_t *);
    int (*write)(void *, uint32_t, unsigned, uint32_t, unsigned);
};
#include "mips_formatter_gte.h"

/* Local to one invocation of the real formatter. Leave the scheduler's
 * original fetch, grants, pre-existing debit, fault check and retirement in
 * place. A control ticket is stored by the real dispatcher's enclosing frame. */
struct bfm_formatter_step {
    struct bfm_formatter_issue source;
    const struct bfm_mounted_host_bus *bus;
    struct bfm_host_bus_slot slot;
    struct bfm_mounted_host_bus opening_bus;
    struct bfm_scalar_owner scalar_owner;
    struct bfm_merge_owner merge_owner;
    struct bfm_generic_bus geometry_bus;
    BfmControlTicket *control;
    const BfmControlAuthority *branch_owner;
    union {
        struct bfm_scalar_prepared scalar;
        struct bfm_merge_ticket merge;
        struct bfm_formatter_gte_request gte;
    } operation;
    bool ready;
};

static inline bool bfm_formatter_data_sample(void *ctx, bool paid, struct bfm_scalar_issue *out)
{
    struct bfm_formatter_step *s = ctx;
    return bfm_formatter_sample(&s->source, paid, out);
}

static inline bool bfm_formatter_map(struct bfm_formatter_step *s,
    const struct bfm_host_bus_request *request, struct bfm_scalar_mapping *mapping)
{
    const struct bfm_mounted_host_bus *bus = s->bus;
    return bus && bus->memory == s->source.memory && bus->cpu == s->source.cpu &&
        bus->family == s->source.first.issuer && bfm_host_bus_map(&s->slot, bus, request, mapping);
}

static inline bool bfm_formatter_scalar_map(void *ctx, const struct bfm_scalar_issue *issue,
    const struct bfm_scalar_memory *access, struct bfm_scalar_mapping *mapping)
{
    struct bfm_formatter_step *s = ctx;
    const struct bfm_host_bus_request req = { access->address, access->address, access->width,
        access->store, (1u << access->width) - 1u };
    return issue->owner == s->source.first.issuer && bfm_formatter_map(s, &req, mapping);
}

static inline bool bfm_formatter_merge_map(void *ctx, const struct bfm_scalar_issue *issue,
    const struct bfm_word_merge *access, unsigned enables, struct bfm_scalar_mapping *mapping)
{
    struct bfm_formatter_step *s = ctx;
    const struct bfm_host_bus_request req = { access->byte_address, access->aligned_address, 4,
        access->store, enables };
    return issue->owner == s->source.first.issuer && bfm_formatter_map(s, &req, mapping);
}

static inline int bfm_formatter_gte_admit(void *ctx, uint32_t address, unsigned width, int writing)
{
    struct bfm_scalar_mapping map;
    const struct bfm_host_bus_request req = { address, address, 4, writing != 0, 15 };
    return width == 4 && bfm_formatter_map(ctx, &req, &map) &&
        map.admit(map.bus_context, map.physical_address, width, writing);
}

static inline int bfm_formatter_gte_read(void *ctx, uint32_t address, unsigned width, uint32_t *value)
{
    struct bfm_formatter_step *s = ctx;
    return s->source.clocked && bfm_formatter_gte_admit(s, address, width, 0) &&
        bfm_host_bus_read(&s->slot, s->slot.grant.physical_address, width, value);
}

static inline int bfm_formatter_gte_write(void *ctx, uint32_t address, unsigned width,
                                         uint32_t value, unsigned enables)
{
    struct bfm_formatter_step *s = ctx;
    return s->source.clocked && enables == 15 && bfm_formatter_gte_admit(s, address, width, 1) &&
        bfm_host_bus_write(&s->slot, s->slot.grant.physical_address, width, value, enables);
}

static inline bool bfm_formatter_control_matches(const struct bfm_formatter_issue *s, const BfmControlTicket *t)
{
    return t->owner.cpu == s->cpu && t->owner.dispatch == s->first.issuer &&
        t->owner.advance == s->before.clock_value.advance &&
        t->owner.clock_context == s->before.clock_value.userdata && t->remaining == s->first.remaining &&
        t->issue.pc == s->source.pc && t->issue.next_pc == s->source.npc &&
        t->issue.branch_pc == s->source.branch_pc && t->issue.delay_slot == (unsigned)s->source.delay_slot &&
        !memcmp(t->issue.gpr, s->source.gpr, sizeof t->issue.gpr) &&
        t->proof.instruction.value == s->word && t->proof.instruction.image == s->first.mounted.image &&
        t->proof.instruction.generation == s->first.mounted.generation;
}

/* This narrow entry is also the production opening caller's link boundary.
 * It carries no control-flow or GTE dependencies into that caller. */
static inline struct bfm_scalar_result bfm_formatter_prepare_scalar(struct bfm_formatter_step *s)
{
    if (s->source.scope.lane == BFM_OPENING_PITCH_SCALAR) {
        s->opening_bus = (struct bfm_mounted_host_bus){ .memory=s->source.memory, .cpu=s->source.cpu,
            .family=s->source.cpu->opening_pitch, .context=(void *)s->source.cpu->opening_pitch,
            .resolve=bfm_opening_scalar_resolve };
        s->bus = &s->opening_bus;
    }
    s->scalar_owner = (struct bfm_scalar_owner){ s, bfm_formatter_data_sample, bfm_formatter_scalar_map };
    return bfm_prepare_mounted_scalar(&s->scalar_owner, &s->operation.scalar);
}

/* Call after original family admission, after cost is calculated, immediately
 * before the original clock. Keep s at a single address through issue. */
static inline struct bfm_scalar_result bfm_formatter_prepare(struct bfm_formatter_step *s,
    MusashiBootMemory *memory, FormatterCpu *cpu, const struct bfm_formatter_scope *scope,
    const struct bfm_mounted_host_bus *bus, const BfmControlAuthority *branch_owner,
    BfmControlTicket *branch_ticket, uint32_t word, uint32_t cost)
{
    struct bfm_scalar_result r = { .status = BFM_SCALAR_DENIED };
    if (!s) return r;
    *s = (struct bfm_formatter_step){ .bus=bus, .control=branch_ticket, .branch_owner=branch_owner };
    if (!bfm_formatter_begin(&s->source, memory, cpu, scope, word, cost)) return r;
    switch (scope->lane) {
    case BFM_OPENING_PITCH_SCALAR:
    case BFM_FAMILY_SCALAR:
        r = bfm_formatter_prepare_scalar(s);
        break;
    case BFM_FAMILY_MERGE:
        s->merge_owner = (struct bfm_merge_owner){ s, bfm_formatter_data_sample, bfm_formatter_merge_map };
        r.status = bfm_mounted_merge_prepare(&s->merge_owner, &s->operation.merge);
        r.address = s->operation.merge.access.byte_address;
        break;
    case BFM_RESIDENT_ROOM_GTE:
        s->geometry_bus = (struct bfm_generic_bus){ s, bfm_formatter_gte_admit,
            bfm_formatter_gte_read, bfm_formatter_gte_write };
        r = bfm_formatter_gte_prepare(cpu, &s->source.source, &s->geometry_bus, word, &s->operation.gte);
        break;
    case BFM_FAMILY_CONTROL:
        if (branch_owner && branch_ticket && bfm_control_prepare(branch_owner, branch_ticket) &&
            bfm_formatter_control_matches(&s->source, branch_ticket)) r.status = BFM_SCALAR_PREPARED;
        break;
    }
    s->ready = r.status == BFM_SCALAR_PREPARED;
    return r;
}

/* At the success edge of the ORIGINAL clock/fault check. Capture payment
 * without advancing the clock or moving any observable instruction effect. */
static inline enum bfm_scalar_status bfm_formatter_clock_succeeded(struct bfm_formatter_step *s,
    MusashiBootMemory *memory, FormatterCpu *cpu, uint32_t word, uint32_t cost)
{
    if (!s || !s->ready || s->source.clocked) return BFM_SCALAR_NO_RESERVATION;
    if (memory != s->source.memory || cpu != s->source.cpu || word != s->source.word ||
        cost != s->source.cycles || !bfm_formatter_capture(&s->source)) {
        s->ready = false;
        return BFM_SCALAR_CLOCKED_STALE;
    }
    s->source.clocked = true;
    return BFM_SCALAR_PREPARED;
}

static inline enum bfm_scalar_status bfm_formatter_take(struct bfm_formatter_step *s)
{
    if (!s || !s->ready || !s->source.clocked) return BFM_SCALAR_NO_RESERVATION;
    s->ready = false;
    return bfm_formatter_capture(&s->source) ? BFM_SCALAR_PREPARED : BFM_SCALAR_CLOCKED_STALE;
}

static inline struct bfm_scalar_result bfm_formatter_opening_effect(struct bfm_formatter_step *s)
{
    struct bfm_scalar_result r = bfm_issue_mounted_scalar(&s->scalar_owner, &s->operation.scalar);
    /* This family writes the load in the ordinary switch arm. The enclosing
     * CPU and all other original retirement obligations continue afterwards. */
    if (r.status == BFM_SCALAR_COMPLETE && r.delayed_target) {
        s->source.cpu->r[r.delayed_target] = r.delayed_value;
        r.delayed_target = 0;
        r.delayed_value = 0;
    }
    return r;
}

/* At the ORIGINAL per-family instruction-effect site, after intervening owner
 * checks, and before normal retirement. Never use this entry point to skip
 * another host gate or issue a second data/provider transaction. */
static inline struct bfm_scalar_result bfm_formatter_issue(struct bfm_formatter_step *s)
{
    struct bfm_scalar_result r = { .status = bfm_formatter_take(s) };
    if (r.status != BFM_SCALAR_PREPARED) return r;
    r.status = BFM_SCALAR_CLOCKED_STALE;
    FormatterCpu *cpu = s->source.cpu;
    const uint32_t word = s->source.word;
    switch (s->source.scope.lane) {
    case BFM_OPENING_PITCH_SCALAR:
        return bfm_formatter_opening_effect(s);
    case BFM_FAMILY_SCALAR: return bfm_issue_mounted_scalar(&s->scalar_owner, &s->operation.scalar);
    case BFM_FAMILY_MERGE: return bfm_mounted_merge_issue(&s->merge_owner, &s->operation.merge);
    case BFM_RESIDENT_ROOM_GTE:
        return bfm_formatter_gte_issue(cpu, &s->source.source, &s->geometry_bus, word, &s->operation.gte);
    case BFM_FAMILY_CONTROL:
        if (bfm_control_after_clock(s->branch_owner, s->control) &&
            bfm_formatter_control_matches(&s->source, s->control)) r.status = BFM_SCALAR_COMPLETE;
        break;
    }
    return r;
}
#endif
