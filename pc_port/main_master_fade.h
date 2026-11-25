/* The VBlank sound tick owns this range while its installed IRQ stack is
 * live. The scalar formatter issues each operation from the mounted image. */
enum {
    MASTER_FADE_START = 0x8002ce1cu, MASTER_FADE_END = 0x8002cec0u,
    MASTER_FADE_LENGTH = MASTER_FADE_END - MASTER_FADE_START,
    MASTER_FADE_LEVEL = 0x800a4ee0u, MASTER_FADE_STEP = 0x800a4ee2u,
    MASTER_FADE_TARGET = 0x800a4ee4u, MASTER_FADE_FLAGS = 0x800a4ee6u,
    MASTER_FADE_DIRTY = 0x800a4f16u
};

static int master_fade_image(const MusashiGuestIrqImage *image, uint8_t reference[MASTER_FADE_LENGTH])
{
    uint64_t hash = UINT64_C(0xcbf29ce484222325);
    memcpy(reference, image->sound_tick + (MASTER_FADE_START - 0x8002cdd8u), MASTER_FADE_LENGTH);
    for (unsigned i = 0; i < MASTER_FADE_LENGTH; ++i) {
        hash ^= reference[i];
        hash *= UINT64_C(0x100000001b3);
    }
    return hash == UINT64_C(0xaf00dbe4c3f7755d);
}

static int master_fade_issue_bound(MusashiBootMemory *memory, const FormatterCpu *cpu,
    const MusashiGuestIrqImage *image, const uint8_t reference[MASTER_FADE_LENGTH])
{
    uint32_t word, saved_return;
    unsigned offset = cpu->pc - MASTER_FADE_START;
    if (offset >= MASTER_FADE_LENGTH || (offset & 3u) || cpu->irq_image != image ||
        cpu->r[0] || cpu->r[29] != 0x8006ca94u || cpu->r[31] != 0x80010984u ||
        cpu->merge_pending || cpu->gte_load_pending ||
        !musashi_boot_read32(memory, cpu->r[29] + 24u, &saved_return) || saved_return != cpu->r[31] ||
        !irq_live_word(image, memory, cpu->pc, &word)) return 0;
    const uint8_t *live = musashi_boot_ram_span(memory, cpu->pc, 4);
    return live && !memcmp(live, reference + offset, 4);
}

typedef struct MasterFadeRun {
    const MusashiGuestIrqImage *image;
    const MusashiExecutionClock *clock;
    MusashiExecutionClock charge;
    uint8_t reference[MASTER_FADE_LENGTH];
    uint32_t pc, npc, branch, delay, hi, lo, regs[32];
} MasterFadeRun;

static int master_fade_issuer(MusashiBootMemory *memory, const FormatterCpu *cpu, uint32_t word)
{
    const MasterFadeRun *run = cpu->master_fade;
    if (!run || cpu->pc != run->pc || cpu->npc != run->npc ||
        cpu->branch_pc != run->branch || cpu->delay_slot != run->delay ||
        cpu->hi != run->hi || cpu->lo != run->lo || memcmp(cpu->r, run->regs, sizeof run->regs) ||
        cpu->clock != run->clock || cpu->clock->advance != run->charge.advance ||
        cpu->clock->userdata != run->charge.userdata ||
        !master_fade_issue_bound(memory, cpu, run->image, run->reference)) return 0;

    unsigned op = word >> 26;
    if (op < 32u) return 1;
    uint32_t address = cpu->r[(word >> 21) & 31u] + (uint32_t)(int32_t)(int16_t)word;
    if (address == MASTER_FADE_LEVEL) return op == 33u || op == 37u || op == 41u;
    if (address == MASTER_FADE_STEP || address == MASTER_FADE_TARGET)
        return op == 33u || op == 37u;
    if (address == MASTER_FADE_FLAGS) return op == 32u || op == 36u || op == 40u;
    return address == MASTER_FADE_DIRTY && op == 40u;
}

static int master_fade_run(MusashiBootMemory *memory, FormatterCpu *cpu, const IrqGuestReturn *stack,
    unsigned depth, const MusashiGuestIrqImage *image, const MusashiExecutionClock *clock,
    MusashiGuestIrqStop *stop)
{
    if (depth != 4u || stack[0].callee != IRQ_DISPATCH || stack[1].callee != IRQ_VBLANK ||
        stack[2].callee != IRQ_GAME_VBLANK || stack[3].callee != IRQ_SOUND_TICK ||
        stack[3].address != 0x80010984u || stack[3].sp != 0x8006cab4u ||
        cpu->master_fade || cpu->clock != clock || !clock || !clock->advance ||
        cpu->pc != MASTER_FADE_START || cpu->npc != cpu->pc + 4u ||
        cpu->delay_slot || cpu->branch_pc) return 0;

    MasterFadeRun run = {.image = image, .clock = clock, .charge = *clock};
    if (!master_fade_image(image, run.reference)) return 0;
    cpu->master_fade = &run;
    int ok = 0;
    while (cpu->pc - MASTER_FADE_START < MASTER_FADE_LENGTH) {
        uint32_t word;
        stop->pc = cpu->pc;
        run.pc = cpu->pc;
        run.npc = cpu->npc;
        run.delay = cpu->delay_slot;
        run.branch = cpu->branch_pc;
        run.hi = cpu->hi;
        run.lo = cpu->lo;
        memcpy(run.regs, cpu->r, sizeof run.regs);
        if (stop->instructions >= 8192u || !irq_live_word(image, memory, cpu->pc, &word) ||
            !master_fade_issuer(memory, cpu, word) || !formatter_step(memory, cpu)) goto done;
        ++stop->instructions;
    }
    ok = cpu->pc == MASTER_FADE_END && cpu->npc == cpu->pc + 4u &&
        !cpu->delay_slot && !cpu->branch_pc;
done:
    cpu->master_fade = NULL;
    return ok;
}
