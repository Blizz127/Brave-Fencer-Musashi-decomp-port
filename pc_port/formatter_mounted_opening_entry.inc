/* SPDX-License-Identifier: MIT */
/* Include at the end of the formatter translation unit with the scoped step
 * definition and all family helpers visible. All ordinary callers keep the
 * original entry. The opening run lends its own success-only retirement count. */
#include "formatter_mounted_dispatch.inc"
struct bfm_opening_step {
    struct bfm_formatter_scope scope;
    struct bfm_formatter_step step;
};

static int formatter_opening_scalar_word(const FormatterCpu *cpu, uint32_t word)
{
    struct bfm_scalar_memory access;
    return bfm_decode_scalar_memory(word, cpu->r, &access);
}

static int formatter_opening_prepare_ticket(struct bfm_opening_step *opening,
    MusashiBootMemory *memory, FormatterCpu *cpu, uint32_t word, uint32_t cost)
{
    struct bfm_formatter_step *s = &opening->step;
    if (!bfm_formatter_begin(&s->source, memory, cpu, &opening->scope, word, cost)) return 0;
    s->ready = bfm_formatter_prepare_scalar(s).status == BFM_SCALAR_PREPARED;
    return s->ready;
}

static int formatter_opening_record_clock(struct bfm_opening_step *opening,
    MusashiBootMemory *memory, FormatterCpu *cpu, uint32_t word, uint32_t cost)
{
    return bfm_formatter_clock_succeeded(&opening->step, memory, cpu, word, cost) == BFM_SCALAR_PREPARED;
}

static int formatter_opening_commit_ticket(struct bfm_opening_step *opening)
{
    return bfm_formatter_take(&opening->step) == BFM_SCALAR_PREPARED &&
        bfm_formatter_opening_effect(&opening->step).status == BFM_SCALAR_COMPLETE;
}

static int formatter_step_opening(MusashiBootMemory *memory, FormatterCpu *cpu,
                                  const uint32_t *retired)
{
    struct bfm_opening_step opening = { .scope = { .lane = BFM_OPENING_PITCH_SCALAR,
        .opening_retired = retired } };
    return formatter_step_scoped(memory, cpu, &opening);
}

static int formatter_step(MusashiBootMemory *memory, FormatterCpu *cpu)
{
    return formatter_step_scoped(memory, cpu, NULL);
}
