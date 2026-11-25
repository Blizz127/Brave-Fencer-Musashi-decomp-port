/* Included by the scalar CPU. Timing is shared by the machine's synchronous
 * callers without changing the layout or identity of existing device clocks. */
static _Thread_local MusashiMduTiming *mdu_machines;
static _Thread_local uint64_t mdu_generation;

int musashi_mdu_bind(MusashiMduTiming *timing, const MusashiBootMemory *memory,
    const MusashiExecutionClock *clock, const uint64_t *cycles) {
    if (!timing || !memory || !clock || !clock->advance || mdu_generation == UINT64_MAX) return 0;
    for (MusashiMduTiming *p = mdu_machines; p; p = p->previous)
        if (p == timing || p->memory == memory || p->clock == clock) return 0;
    *timing = (MusashiMduTiming){.memory = memory, .clock = clock,
        .issuer = *clock, .cycles = cycles, .generation = ++mdu_generation, .previous = mdu_machines};
    mdu_machines = timing;
    return 1;
}

void musashi_mdu_unbind(MusashiMduTiming *timing) {
    for (MusashiMduTiming **p = &mdu_machines; *p; p = &(*p)->previous) {
        if (*p == timing) {
            *p = timing->previous;
            *timing = (MusashiMduTiming){0};
            return;
        }
    }
}

static MusashiMduTiming *mdu_machine(const MusashiBootMemory *memory) {
    for (MusashiMduTiming *p = mdu_machines; p; p = p->previous)
        if (p->memory == memory) return p;
    return NULL;
}

static int mdu_binding(const MusashiMduTiming *timing, const FormatterCpu *cpu) {
    return cpu->clock == timing->clock && cpu->clock->advance == timing->issuer.advance &&
        cpu->clock->userdata == timing->issuer.userdata;
}

/* Read and producer operations interlock. Direct writes have no interlock
 * in the measured interpreter profile and do not cancel a pending result. */
static int mdu_interlocked(uint32_t word) {
    const unsigned fn = word & 63u;
    return (word >> 26) == 0 && (fn == 16u || fn == 18u || (fn >= 24u && fn <= 27u));
}

static uint32_t mdu_duration(uint32_t word, uint32_t lhs) {
    const unsigned fn = word & 63u;
    if (fn < 24u) return 0;
    if (fn >= 26u) return 38u;
    if (fn == 24u) lhs ^= 0u - (lhs >> 31);
    if (lhs <= 0x7ffu) return 8u;
    if (lhs <= 0xfffffu) return 11u;
    return 15u;
}

typedef struct FormatterMduIssue {
    uint32_t pc, npc, branch, hi, lo, regs[32];
    uint8_t delay;
    uint64_t generation;
    const MusashiCdIrqFrame *cd_frame;
    const MusashiCallbackDevice *callback;
    const MusashiGuestIrqImage *image;
    const struct OpeningPitchRun *pitch;
    const struct TitleMixerRun *mixer;
    const struct MasterFadeRun *fade;
} FormatterMduIssue;

static void mdu_capture(FormatterMduIssue *issue, const FormatterCpu *cpu, const MusashiMduTiming *timing) {
    *issue = (FormatterMduIssue){.pc = cpu->pc, .npc = cpu->npc, .branch = cpu->branch_pc,
        .delay = cpu->delay_slot, .hi = cpu->hi, .lo = cpu->lo, .image = cpu->irq_image,
        .pitch = cpu->opening_pitch, .mixer = cpu->title_mixer, .fade = cpu->master_fade,
        .cd_frame = cpu->cd_irq_frame, .callback = cpu->callback_device, .generation = timing->generation};
    memcpy(issue->regs, cpu->r, sizeof issue->regs);
}

static int mdu_same_issue(MusashiMduTiming *timing, MusashiBootMemory *memory,
    FormatterCpu *cpu, const FormatterMduIssue *issue, uint32_t instruction) {
    uint32_t live;
    return mdu_machine(memory) == timing && timing->generation == issue->generation && mdu_binding(timing, cpu) &&
        cpu->cd_irq_frame == issue->cd_frame && cpu->callback_device == issue->callback &&
        cpu->pc == issue->pc && cpu->npc == issue->npc && cpu->branch_pc == issue->branch &&
        cpu->delay_slot == issue->delay && cpu->hi == issue->hi && cpu->lo == issue->lo &&
        cpu->irq_image == issue->image && cpu->opening_pitch == issue->pitch &&
        cpu->title_mixer == issue->mixer && cpu->master_fade == issue->fade &&
        !memcmp(cpu->r, issue->regs, sizeof issue->regs) &&
        formatter_fetch(memory, cpu, &live) && live == instruction;
}
