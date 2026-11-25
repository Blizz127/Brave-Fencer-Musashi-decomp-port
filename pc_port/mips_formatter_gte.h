/* SPDX-License-Identifier: MIT */
#ifndef BFM_MIPS_FORMATTER_GTE_H
#define BFM_MIPS_FORMATTER_GTE_H

#include "mips_mounted_scalar.h"
#include "mounted_gte.h"

/* Local to one CPU issue, across exactly one outer scheduler charge. Source
 * is the same caller-owned object refilled using musashi_boot_cpu_context.
 * It borrows the real formatter CPU and register file. */
struct bfm_formatter_gte_request {
    bool ready;
    MusashiCpuContext source_before;
    struct bfm_gte_request operation;
};

static inline bool bfm_formatter_gte_source(const FormatterCpu *cpu,
                                            const MusashiCpuContext *source,
                                            uint32_t word)
{
    return cpu && source && source->identity == cpu && source->gpr == cpu->r &&
           source->provenance == MUSASHI_CPU_CONTEXT_SOURCE && source->instruction_valid &&
           source->instruction == word && source->pc == cpu->pc && source->npc == cpu->npc &&
           source->delay_slot == cpu->delay_slot && source->branch_pc == cpu->branch_pc;
}

static inline bool bfm_formatter_gte_uses_bus(const struct bfm_gte_request *q)
{
    return q->operation == BFM_GTE_LWC2 || q->operation == BFM_GTE_SWC2;
}

/* The common caller supplies the genuine image, family-specific source,
 * provider and mapped bus grants. In particular the builder alone cannot
 * grant an arbitrary coprocessor or memory operation. */
static inline struct bfm_scalar_result
bfm_formatter_gte_prepare(FormatterCpu *cpu, const MusashiCpuContext *source,
                          const struct bfm_generic_bus *bus, uint32_t word,
                          struct bfm_formatter_gte_request *plan)
{
    struct bfm_scalar_result result = { BFM_SCALAR_DENIED, 0, 0, 0 };
    if (!plan) return result;
    plan->ready = false;
    if (!bfm_formatter_gte_source(cpu, source, word)) return result;
    enum bfm_gte_status status = bfm_gte_prepare(word, cpu->r, source, cpu->cpu_transfer,
                                                bus, &plan->operation);
    switch (status) {
    case BFM_GTE_OK:
        if (bfm_formatter_gte_uses_bus(&plan->operation)) result.address = plan->operation.operand;
        plan->source_before = *source;
        plan->ready = true;
        result.status = BFM_SCALAR_PREPARED;
        break;
    case BFM_GTE_UNHANDLED:
        result.status = BFM_SCALAR_OTHER_OPCODE;
        break;
    case BFM_GTE_LOAD_ADDRESS_ERROR:
    case BFM_GTE_STORE_ADDRESS_ERROR:
        result.address = plan->operation.operand;
        result.status = status == BFM_GTE_LOAD_ADDRESS_ERROR ? BFM_SCALAR_LOAD_ADDRESS : BFM_SCALAR_STORE_ADDRESS;
        break;
    default:
        break;
    }
    return result;
}

/* Consume the reservation on all clocked exits. The caller reauthenticates
 * owner, mount, mapped region and hazards before invoking this half. */
static inline struct bfm_scalar_result
bfm_formatter_gte_issue(FormatterCpu *cpu, const MusashiCpuContext *source,
                        const struct bfm_generic_bus *bus, uint32_t word,
                        struct bfm_formatter_gte_request *plan)
{
    struct bfm_scalar_result result = { BFM_SCALAR_NO_RESERVATION, 0, 0, 0 };
    if (!plan || !plan->ready) return result;
    plan->ready = false;
    if (bfm_formatter_gte_uses_bus(&plan->operation)) result.address = plan->operation.operand;
    result.status = BFM_SCALAR_CLOCKED_STALE;
    const MusashiCpuContext *prior = &plan->source_before;
    if (!bfm_formatter_gte_source(cpu, source, word) || prior->identity != source->identity ||
        prior->pc != source->pc || prior->npc != source->npc ||
        prior->delay_slot != source->delay_slot || prior->branch_pc != source->branch_pc)
        return result;
    struct bfm_gte_delayed_result pending;
    enum bfm_gte_status status = bfm_gte_issue(&plan->operation, word, cpu->r, source,
                                              cpu->cpu_transfer, bus, &pending);
    if (status == BFM_GTE_OK) {
        result.status = BFM_SCALAR_COMPLETE;
        result.delayed_target = pending.reg;
        result.delayed_value = pending.value;
    } else if (status == BFM_GTE_CALLBACK_FAILED) {
        /* A read may have reached MMIO or a provider before failure. */
        result.status = BFM_SCALAR_CLOCKED_BUS_FAILURE;
    }
    return result;
}

#endif
