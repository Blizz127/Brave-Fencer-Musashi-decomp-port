/* SPDX-License-Identifier: MIT */
/* Include after FormatterCpu, formatter_fetch and musashi_boot_cpu_context.
 * The family supplies the mounted executable reader, independent dispatch
 * authority and protected stack. This projection establishes no edge grant. */
#include "mips_mounted_control.h"
#include <string.h>

typedef struct BfmFormatterControlSource {
    BfmControlSite site;
    const MusashiBootMemory *memory;
    const FormatterCpu *cpu;
    const MusashiExecutionClock *clock;
    MusashiExecutionClock clock_value;
    const MusashiCpuTransferDevice *transfer;
    BfmControlWord instruction;
    uint32_t hi, lo;
    uint32_t merge_value, merge_next_pc, gte_load_value, gte_load_next_pc;
    uint8_t merge_reg, merge_kind, merge_pending, gte_load_reg, gte_load_pending;
} BfmFormatterControlSource;

static inline int formatter_control_capture(MusashiBootMemory *memory, const FormatterCpu *cpu,
                                           const BfmControlAuthority *family,
                                           BfmFormatterControlSource *out)
{
    uint32_t word;
    MusashiCpuContext context;
    BfmControlWord mounted = {0};
    if (!memory || !cpu || !family || !family->read_executable || !out ||
        !cpu->clock || !cpu->clock->advance || (cpu->pc & 3) ||
        cpu->r[0] || cpu->delay_slot > 1 || cpu->merge_pending > 1 ||
        cpu->gte_load_pending > 1 || (cpu->merge_pending && cpu->merge_reg >= 32) ||
        (cpu->gte_load_pending && cpu->gte_load_reg >= 32) ||
        !formatter_fetch(memory, cpu, &word) ||
        !musashi_boot_cpu_context(cpu, MUSASHI_CPU_CONTEXT_SOURCE, &context) ||
        context.identity != cpu || context.gpr != cpu->r ||
        context.provenance != MUSASHI_CPU_CONTEXT_SOURCE || !context.instruction_valid ||
        context.instruction != word || context.pc != cpu->pc || context.npc != cpu->npc ||
        context.branch_pc != cpu->branch_pc || context.delay_slot != cpu->delay_slot ||
        !family->read_executable(family->context, cpu->pc, &mounted) ||
        !mounted.image || mounted.value != word)
        return 0;

    BfmFormatterControlSource s = {0};
    s.memory = memory;
    s.cpu = cpu;
    s.clock = cpu->clock;
    s.clock_value = *cpu->clock;
    s.transfer = cpu->cpu_transfer;
    s.instruction = mounted;
    s.site.pc = context.pc;
    s.site.next_pc = context.npc;
    s.site.branch_pc = context.branch_pc;
    s.site.delay_slot = cpu->delay_slot;
    memcpy(s.site.gpr, context.gpr, sizeof(s.site.gpr));
    s.hi = cpu->hi;
    s.lo = cpu->lo;
    /* The real issuer has two independent pending pipelines. Capture each
     * separately, including the forwarding horizon and merge direction. */
    if (cpu->merge_pending) {
        s.merge_pending = 1;
        s.merge_reg = cpu->merge_reg;
        s.merge_kind = cpu->merge_kind;
        s.merge_value = cpu->merge_value;
        s.merge_next_pc = cpu->merge_next_pc;
    }
    if (cpu->gte_load_pending) {
        s.gte_load_pending = 1;
        s.gte_load_reg = cpu->gte_load_reg;
        s.gte_load_value = cpu->gte_load_value;
        s.gte_load_next_pc = cpu->gte_load_next_pc;
    }
    *out = s;
    return 1;
}

/* Call from the family's trusted observe hook. The family must separately
 * fill mapping, mapping_generation, dispatch, stack identity/top and budget. */
static inline void formatter_control_project(const BfmFormatterControlSource *s,
                                            BfmControlView *view)
{
    view->site = s->site;
    view->owner.cpu = s->cpu;
    view->owner.clock_context = s->clock_value.userdata;
    view->owner.advance = s->clock_value.advance;
}

/* Compare a fresh post-clock capture with the pre-clock capture at the same
 * issue boundary, before the real issuer changes the CPU. No time is charged. */
static inline int formatter_control_same_source(const BfmFormatterControlSource *a,
                                               const BfmFormatterControlSource *b)
{
    return a && b && a->memory == b->memory && a->cpu == b->cpu && a->clock == b->clock &&
           a->clock_value.advance == b->clock_value.advance &&
           a->clock_value.userdata == b->clock_value.userdata && a->transfer == b->transfer &&
           a->instruction.value == b->instruction.value && a->instruction.image == b->instruction.image &&
           a->instruction.generation == b->instruction.generation &&
           a->site.pc == b->site.pc && a->site.next_pc == b->site.next_pc &&
           a->site.branch_pc == b->site.branch_pc && a->site.delay_slot == b->site.delay_slot &&
           memcmp(a->site.gpr, b->site.gpr, sizeof(a->site.gpr)) == 0 &&
           a->hi == b->hi && a->lo == b->lo && a->merge_pending == b->merge_pending &&
           a->merge_reg == b->merge_reg && a->merge_kind == b->merge_kind &&
           a->merge_value == b->merge_value && a->merge_next_pc == b->merge_next_pc &&
           a->gte_load_pending == b->gte_load_pending && a->gte_load_reg == b->gte_load_reg &&
           a->gte_load_value == b->gte_load_value && a->gte_load_next_pc == b->gte_load_next_pc;
}
