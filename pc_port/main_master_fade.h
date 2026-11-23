/* Sound-tick master fade from src/main/8002cdd8.c. The registered callback
 * keeps its surrounding command, voice-status and channel services. */
enum {
    MASTER_FADE_START = 0x8002ce1cu, MASTER_FADE_END = 0x8002cec0u,
    MASTER_FADE_LENGTH = MASTER_FADE_END - MASTER_FADE_START,
    MASTER_FADE_LEVEL = 0x800a4ee0u, MASTER_FADE_STEP = 0x800a4ee2u,
    MASTER_FADE_TARGET = 0x800a4ee4u, MASTER_FADE_FLAGS = 0x800a4ee6u,
    MASTER_FADE_DIRTY = 0x800a4f16u
};

/* Bind only the fade instructions. The digest is from the public game C
 * compiled with GCC 2.7.2 -O2 -G0, with the public symbol addresses. */
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

static unsigned master_fade_cost(uint32_t pc)
{
    switch (pc) {
    case 0x8002ce20u: case 0x8002ce38u: case 0x8002ce40u: case 0x8002ce48u:
    case 0x8002ce60u: case 0x8002ce70u: case 0x8002ce84u: case 0x8002ce94u:
    case 0x8002ceb0u: case 0x8002ceb8u: return 3u;
    default: return 2u;
    }
}

static int master_fade_op(MusashiBootMemory *memory, FormatterCpu *cpu)
{
    uint32_t *r = cpu->r, pc = cpu->pc, next = cpu->npc, after = next + 4u;
    uint16_t half;
    uint8_t byte;
    int branch = 0;
    switch (pc) {
    case 0x8002ce1cu: r[6] = 0x800a0000u; break;
    case 0x8002ce20u:
        if (!cpu_read8(memory, cpu, MASTER_FADE_FLAGS, &byte)) return 0;
        r[6] = byte;
        break;
    case 0x8002ce24u: case 0x8002ce58u: case 0x8002ce64u: case 0x8002ce88u: break;
    case 0x8002ce28u: r[2] = r[6] & 1u; break;
    case 0x8002ce2cu:
        branch = 1;
        if (!r[2]) after = 0x8002cebcu;
        break;
    case 0x8002ce30u: r[2] = 1; break;
    case 0x8002ce34u: r[5] = 0x800a0000u; break;
    case 0x8002ce38u:
        if (!cpu_read16(memory, cpu, MASTER_FADE_TARGET, &half)) return 0;
        r[5] = half;
        break;
    case 0x8002ce3cu: r[3] = 0x800a0000u; break;
    case 0x8002ce40u:
        if (!cpu_read16(memory, cpu, MASTER_FADE_LEVEL, &half)) return 0;
        r[3] = half;
        break;
    case 0x8002ce44u: case 0x8002ce6cu: case 0x8002ce90u:
    case 0x8002ceacu: case 0x8002ceb4u: r[1] = 0x800a0000u; break;
    case 0x8002ce48u:
        if (!cpu_write8(memory, cpu, MASTER_FADE_DIRTY, (uint8_t)r[2])) return 0;
        break;
    case 0x8002ce4cu: r[4] = (uint16_t)r[5]; break;
    case 0x8002ce50u: r[2] = r[4] < r[3]; break;
    case 0x8002ce54u:
        branch = 1;
        if (!r[2]) after = 0x8002ce80u;
        break;
    case 0x8002ce5cu: case 0x8002ce80u: r[2] = 0x800a0000u; break;
    case 0x8002ce60u: case 0x8002ce84u:
        if (!cpu_read16(memory, cpu, MASTER_FADE_STEP, &half)) return 0;
        r[2] = half;
        break;
    case 0x8002ce68u: r[2] = r[3] - r[2]; break;
    case 0x8002ce8cu: r[2] = r[3] + r[2]; break;
    case 0x8002ce70u: case 0x8002ce94u:
        if (!cpu_write16(memory, cpu, MASTER_FADE_LEVEL, (uint16_t)r[2])) return 0;
        break;
    case 0x8002ce74u: case 0x8002ce98u: r[2] = (uint16_t)r[2]; break;
    case 0x8002ce78u: branch = 1; after = 0x8002cea0u; break;
    case 0x8002ce7cu: r[2] = r[4] < r[2]; break;
    case 0x8002ce9cu: r[2] = r[2] < r[4]; break;
    case 0x8002cea0u:
        branch = 1;
        if (r[2]) after = MASTER_FADE_END;
        break;
    case 0x8002cea4u: case 0x8002cebcu: r[4] = 0; break;
    case 0x8002cea8u: r[2] = r[6] & 0xfeu; break;
    case 0x8002ceb0u:
        if (!cpu_write16(memory, cpu, MASTER_FADE_LEVEL, (uint16_t)r[5])) return 0;
        break;
    case 0x8002ceb8u:
        if (!cpu_write8(memory, cpu, MASTER_FADE_FLAGS, (uint8_t)r[2])) return 0;
        break;
    default: return 0;
    }
    cpu->pc = next;
    cpu->npc = after;
    cpu->branch_pc = branch ? pc : 0;
    cpu->delay_slot = (uint8_t)branch;
    return 1;
}

static int master_fade_run(MusashiBootMemory *memory, FormatterCpu *cpu, const IrqGuestReturn *stack,
    unsigned depth, const MusashiGuestIrqImage *image, const MusashiExecutionClock *clock,
    MusashiGuestIrqStop *stop)
{
    uint8_t reference[MASTER_FADE_LENGTH];
    if (depth != 4u || stack[0].callee != IRQ_DISPATCH || stack[1].callee != IRQ_VBLANK ||
        stack[2].callee != IRQ_GAME_VBLANK || stack[3].callee != IRQ_SOUND_TICK ||
        stack[3].address != 0x80010984u || stack[3].sp != 0x8006cab4u ||
        cpu->pc != MASTER_FADE_START || cpu->npc != cpu->pc + 4u || cpu->delay_slot ||
        cpu->branch_pc || !clock || !clock->advance || !master_fade_image(image, reference)) return 0;
    do {
        uint32_t pc = cpu->pc, npc = cpu->npc, branch = cpu->branch_pc, regs[32];
        unsigned delay = cpu->delay_slot;
        stop->pc = pc;
        if (stop->instructions >= 8192 || cpu->clock != clock ||
            !master_fade_issue_bound(memory, cpu, image, reference)) return 0;
        memcpy(regs, cpu->r, sizeof regs);
        if (!clock->advance(clock->userdata, master_fade_cost(pc)) ||
            cpu->pc != pc || cpu->npc != npc || cpu->branch_pc != branch || cpu->delay_slot != delay ||
            cpu->clock != clock || memcmp(cpu->r, regs, sizeof regs) ||
            !master_fade_issue_bound(memory, cpu, image, reference) ||
            !master_fade_op(memory, cpu)) return 0;
        ++stop->instructions;
    } while (cpu->pc != MASTER_FADE_END);
    return cpu->npc == MASTER_FADE_END + 4u && !cpu->branch_pc && !cpu->delay_slot;
}
