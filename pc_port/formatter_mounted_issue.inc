/* SPDX-License-Identifier: MIT */
/* Same translation unit as the formatter and its audited family predicates. */
#include "formatter_mounted_control.inc"
#include "mips_mounted_merge.h"
#include "formatter_mounted_family.inc"
#include <limits.h>

enum bfm_formatter_lane {
    BFM_OPENING_PITCH_SCALAR, BFM_RESIDENT_ROOM_GTE,
    BFM_FAMILY_SCALAR, BFM_FAMILY_MERGE, BFM_FAMILY_CONTROL
};

struct bfm_formatter_witness {
    BfmControlWord mounted;
    const void *issuer;
    const void *provider;
    uint64_t issuer_revision;
    unsigned remaining; /* Includes the outer scheduler's reserved current step. */
};

struct bfm_formatter_scope {
    enum bfm_formatter_lane lane;
    const unsigned *remaining;
    const uint32_t *opening_retired; /* Live opening_pitch_run stop->instructions. */
    bool current_step_reserved; /* input_callback_step has already subtracted one. */
    void *context;
    /* From the existing family owner. Verify the actual mounted source, CPU,
     * image lineage, dispatch permission, scheduler allowance and (for GTE)
     * current device usability. No data reads or clock charges. */
    bool (*observe)(void *, MusashiBootMemory *, const FormatterCpu *,
                    uint32_t, struct bfm_formatter_witness *);
};

struct bfm_formatter_issue {
    MusashiBootMemory *memory;
    FormatterCpu *cpu;
    const struct bfm_formatter_scope *authority;
    struct bfm_formatter_scope scope;
    struct bfm_formatter_witness first, current;
    BfmFormatterControlSource before;
    struct bfm_formatter_family_snapshot family;
    MusashiCpuContext source;
    uint32_t word, cycles;
    bool prepared, clocked;
};

static inline int
bfm_formatter_mounted_word(void *context, uint32_t pc, BfmControlWord *out)
{
    struct bfm_formatter_issue *step = context;
    const struct bfm_formatter_scope *scope = step->authority;
    FormatterCpu *cpu = step->cpu;
    MusashiBootMemory *memory = step->memory;
    struct bfm_formatter_witness w = {0};
    uint32_t word;
    if (scope->context != step->scope.context || scope->observe != step->scope.observe ||
        scope->remaining != step->scope.remaining || scope->lane != step->scope.lane ||
        scope->current_step_reserved != step->scope.current_step_reserved ||
        scope->opening_retired != step->scope.opening_retired || pc != cpu->pc) return 0;
    unsigned available;
    if (scope->lane == BFM_OPENING_PITCH_SCALAR) {
        if (!scope->opening_retired || *scope->opening_retired >= 8192u) return 0;
        available = 8192u - *scope->opening_retired;
    } else {
        if (!scope->remaining) return 0;
        unsigned raw = *scope->remaining;
        if (scope->current_step_reserved && raw == UINT_MAX) return 0;
        available = raw + (unsigned)scope->current_step_reserved;
    }
    if (!available) return 0;
    switch (scope->lane) {
    case BFM_OPENING_PITCH_SCALAR: {
        const OpeningPitchRun *run = cpu->opening_pitch;
        if (!run || !run->image || run->image->identity != run->image ||
            run->image->memory != memory || !opening_pitch_word(memory, cpu, &word) ||
            word != step->word || !opening_pitch_issuer(memory, cpu, word)) return 0;
        if (step->prepared && (step->family.run != run ||
            !bfm_formatter_same_opening(&step->family.state.opening, run))) return 0;
        if (!step->prepared) { step->family.run = run; step->family.state.opening = *run; }
        w.issuer = run;
        w.provider = memory;
        w.mounted = (BfmControlWord){ .image = run->image, .value = word };
        break;
    }
    case BFM_RESIDENT_ROOM_GTE: {
        const Sc01RoomFrame *frame = &cpu->sc01_room;
        if (frame->memory != memory || !cpu->cpu_transfer || !sc01_room_geometry_owner(memory, cpu) ||
            (step->prepared && !bfm_formatter_same_room(&step->family.state.room, frame))) return 0;
        const uint8_t *p = cpu_ram_span(memory, cpu, pc, 4);
        if (!p) return 0;
        word = (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
               ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
        if (word != step->word) return 0;
        if (!step->prepared) step->family.state.room = *frame;
        /* The existing owner permits specific caller/form/device combinations.
         * A mounted word and CU2 alone cannot grant the device operation. */
        if (!scope->observe || !scope->observe(scope->context, memory, cpu, word, &w) ||
            w.issuer != frame || w.provider != cpu->cpu_transfer || !w.mounted.image ||
            w.mounted.value != word) return 0;
        break;
    }
    case BFM_FAMILY_SCALAR:
    case BFM_FAMILY_MERGE:
    case BFM_FAMILY_CONTROL:
        if (!scope->observe || !scope->observe(scope->context, memory, cpu, step->word, &w) ||
            !w.issuer || !w.provider || !w.mounted.image || w.mounted.value != step->word) return 0;
        break;
    default: return 0;
    }
    w.remaining = available;
    step->current = w;
    *out = w.mounted;
    return 1;
}

static inline bool bfm_formatter_capture(struct bfm_formatter_issue *s)
{
    const BfmControlAuthority reader = { .context = s, .read_executable = bfm_formatter_mounted_word };
    BfmFormatterControlSource current;
    if (!formatter_control_capture(s->memory, s->cpu, &reader, &current)) return false;
    if (s->prepared && (!formatter_control_same_source(&s->before, &current) ||
        s->first.issuer != s->current.issuer || s->first.provider != s->current.provider ||
        s->first.issuer_revision != s->current.issuer_revision ||
        s->first.remaining != s->current.remaining)) return false;
    if (!s->prepared) {
        s->before = current;
        s->first = s->current;
    }
    /* The same context object is filled by the real provider-facing builder on
     * either side of the clock. Require its logical fetch to remain identical. */
    return musashi_boot_cpu_context(s->cpu, MUSASHI_CPU_CONTEXT_SOURCE, &s->source) &&
           s->source.identity == s->cpu && s->source.gpr == s->cpu->r &&
           s->source.instruction_valid && s->source.provenance == MUSASHI_CPU_CONTEXT_SOURCE &&
           s->source.pc == current.site.pc && s->source.npc == current.site.next_pc &&
           s->source.branch_pc == current.site.branch_pc &&
           s->source.delay_slot == (int)current.site.delay_slot &&
           s->source.instruction == current.instruction.value;
}

static inline bool bfm_formatter_begin(struct bfm_formatter_issue *s,
    MusashiBootMemory *memory, FormatterCpu *cpu, const struct bfm_formatter_scope *scope,
    uint32_t word, uint32_t cost)
{
    if (!s || !memory || !cpu || !scope || !cost) return false;
    *s = (struct bfm_formatter_issue){ .memory = memory, .cpu = cpu, .authority = scope,
        .scope = *scope, .word = word, .cycles = cost };
    if (!bfm_formatter_capture(s)) return false;
    s->prepared = true;
    return true;
}

static inline bool bfm_formatter_sample(void *context, bool clocked, struct bfm_scalar_issue *out)
{
    struct bfm_formatter_issue *s = context;
    if (!s->prepared || s->clocked != clocked || !bfm_formatter_capture(s)) return false;
    const BfmFormatterControlSource *v = &s->before;
    *out = (struct bfm_scalar_issue){ .cpu = s->cpu, .image = v->instruction.image,
        .owner = s->first.issuer, .provider = s->first.provider, .revision = s->first.issuer_revision,
        .pc = v->site.pc, .next_pc = v->site.next_pc, .instruction = v->instruction.value,
        .delay_slot = v->site.delay_slot != 0, .clock_context = v->clock_value.userdata,
        .advance = v->clock_value.advance, .cycles = s->cycles };
    memcpy(out->regs, v->site.gpr, sizeof(out->regs));
    const unsigned target = (s->word >> 16) & 31u;
    if (v->merge_pending) {
        if (v->merge_next_pc != out->pc) return false;
        out->delayed_target = v->merge_reg;
        out->delayed_value = v->merge_value;
    }
    if (v->gte_load_pending) {
        if (v->gte_load_next_pc != out->pc) return false;
        if (!v->merge_pending || v->gte_load_reg == target) {
            if (v->merge_pending && v->merge_reg == target) return false;
            out->delayed_target = v->gte_load_reg;
            out->delayed_value = v->gte_load_value;
        }
    }
    return true;
}
