/* Channel 2's opening motion is installed by the bank-one sequencer. The
 * channel update already calls its pitch leaf on each tick. Retire the
 * caller, its delay, and the supplied leaf on the same CPU and clock. */
enum {
    OPENING_PITCH_CALL = 0x800313d0u,
    OPENING_PITCH_RESUME = OPENING_PITCH_CALL + 8u,
    OPENING_PITCH_ENTRY = 0x800314dcu,
    OPENING_PITCH_CHANNEL = 0x800a4a30u,
    OPENING_PITCH_MIXER = 0x800a4cb8u,
    OPENING_PITCH_SLOT = 0x800a46e8u,
    OPENING_PITCH_CALLER_SP = 0x8006ca3cu
};

typedef struct OpeningPitchRun {
    const MusashiGuestIrqImage *image;
    const MusashiExecutionClock *clock;
    MusashiExecutionClock binding;
    uint32_t pc, npc, branch, hi, lo, regs[32];
    int delay;
    uint8_t channel_state[0x54], mixer_state[0x48];
    uint8_t caller[8], leaf[sizeof(((MusashiGuestIrqImage *)0)->channel_pitch)];
} OpeningPitchRun;

static int opening_pitch_requested(const FormatterCpu *cpu)
{
    if (!cpu->irq_image || !cpu->irq_image->memory ||
        irq_channel_record(cpu) != OPENING_PITCH_CHANNEL) return 0;
    const uint8_t *flags = musashi_boot_ram_span(cpu->irq_image->memory, OPENING_PITCH_CHANNEL + 0x37u, 1u);
    return flags && *flags == 1u;
}

static int opening_pitch_owner(MusashiBootMemory *memory, const FormatterCpu *cpu)
{
    const uint32_t channel = OPENING_PITCH_CHANNEL, slot = OPENING_PITCH_SLOT;
    uint16_t voice, kind, bank;
    const uint8_t *voice_slot = musashi_boot_ram_span(memory, channel + 0x51u, 1u);
    const uint8_t *latch = musashi_boot_ram_span(memory, slot + 0x3au + 2u, 1u);
    /* The pitch leaf has its own eight-byte frame. Its parent link stays
     * in the channel updater's frame across both stack positions. */
    return opening_pitch_requested(cpu) &&
        (cpu->r[29] == OPENING_PITCH_CALLER_SP || cpu->r[29] == OPENING_PITCH_CALLER_SP - 8u) &&
        cd_irq_ram_word(memory, OPENING_PITCH_CALLER_SP + 52u, 0x8002d014u) &&
        !cpu->level_slot && voice_slot && *voice_slot == 2u && latch && *latch &&
        musashi_boot_read16(memory, channel + 10u, &voice) && voice == 18u &&
        cd_irq_ram_word(memory, channel + 0x14u, 0x800c9a34u) &&
        cd_irq_ram_word(memory, channel + 0x40u, 0x800342e8u) &&
        cd_irq_ram_word(memory, channel + 0x44u, 0u) &&
        musashi_boot_read16(memory, slot, &kind) && (kind & 63u) == 5u &&
        musashi_boot_read16(memory, slot + 52u, &bank) && bank == 1u &&
        cd_irq_ram_word(memory, slot + 16u, 0x800c9a50u) &&
        irq_opening_slot_prefix(cpu, slot, 0x800c9a50u) &&
        irq_opening_motion_record(memory) && irq_opening_track4(memory) &&
        cd_irq_ram_word(memory, 0x8006a97cu, 0x800a6180u) && irq_opening_instrument_row(memory, 9u);
}

static int opening_pitch_digest(const uint8_t *bytes, size_t size, const uint8_t expected[32])
{
    uint8_t digest[32];
    return bytes && musashi_sha256_digest(bytes, size, digest) && !memcmp(expected, digest, sizeof digest);
}

static int opening_pitch_word(MusashiBootMemory *memory, const FormatterCpu *cpu, uint32_t *word)
{
    const OpeningPitchRun *run = cpu->opening_pitch;
    const uint8_t *reference;
    if (!run || cpu->irq_image != run->image || run->image->identity != run->image ||
        run->image->memory != memory || (cpu->pc & 3u)) return 0;
    uint32_t offset = cpu->pc - OPENING_PITCH_ENTRY;
    if (offset < sizeof run->leaf) {
        reference = run->leaf + offset;
        if (memcmp(reference, run->image->channel_pitch + offset, 4)) return 0;
    } else {
        offset = cpu->pc - OPENING_PITCH_CALL;
        if (offset >= sizeof run->caller || !irq_live_word(run->image, memory, cpu->pc, word)) return 0;
        reference = run->caller + offset;
    }
    const uint8_t *live = musashi_boot_ram_span(memory, cpu->pc, 4);
    return live && !memcmp(live, reference, 4) && musashi_boot_read32(memory, cpu->pc, word);
}

/* Only the motion's owning channel and mixer receive writes. Its tuning,
 * instrument, voice mask and adjacent pitch coefficients are read-only. */
static int opening_pitch_access(MusashiBootMemory *memory, uint32_t word, const FormatterCpu *cpu)
{
    unsigned op = word >> 26;
    if (op < 32u) return 1;
    uint32_t address = cpu->r[(word >> 21) & 31u] + (uint32_t)(int32_t)(int16_t)word;
    const uint32_t ch = OPENING_PITCH_CHANNEL, mix = OPENING_PITCH_MIXER;
    if (op == 35u || op == 43u) {
        if (address == ch + 0x24u || address == mix || address == mix + 4u || address == mix + 0x40u) return 1;
        if (op == 43u) return 0;
        if (address == ch + 0x28u || address == ch + 0x2cu || address == 0x8006a97cu) return 1;
        return address == 0x80073188u && cd_irq_ram_word(memory, address, 1u << 18u);
    }
    if (op == 32u || op == 36u || op == 40u) {
        if (address == mix + 0x44u) return 1;
        return op != 40u && (address == ch + 0x37u || address == ch + 0x51u);
    }
    if (op != 33u && op != 37u && op != 41u) return 0;
    if (address == ch + 0x30u || address == mix + 0x14u || address == mix + 0x18u) return 1;
    if (op == 41u) return 0;
    if (address == ch + 0x0au || address == ch + 0x0cu || address == ch + 0x10u ||
        address == 0x800a61f0u) return 1;
    if ((address & 1u) || address - 0x8006ab30u >= 0xa8u) return 0;
    static const uint8_t curve_digest[32] = {
        0x93, 0xe3, 0xa0, 0x09, 0x04, 0x40, 0x55, 0xe4,
        0xb4, 0xa8, 0x55, 0xf9, 0x7e, 0xdc, 0x7e, 0x2b,
        0xc1, 0x42, 0x09, 0x72, 0xee, 0xab, 0x28, 0x36,
        0x35, 0x9d, 0x9c, 0x94, 0x51, 0xf2, 0x17, 0x65
    };
    return opening_pitch_digest(musashi_boot_ram_span(memory, 0x8006ab30u, 0xa8u), 0xa8u, curve_digest);
}

static int opening_pitch_issuer(MusashiBootMemory *memory, const FormatterCpu *cpu, uint32_t word)
{
    const OpeningPitchRun *run = cpu->opening_pitch;
    uint32_t mounted;
    const uint8_t *channel = musashi_boot_ram_span(memory, OPENING_PITCH_CHANNEL, sizeof run->channel_state);
    const uint8_t *mixer = musashi_boot_ram_span(memory, OPENING_PITCH_MIXER, sizeof run->mixer_state);
    return run && channel && mixer &&
        !memcmp(channel, run->channel_state, sizeof run->channel_state) &&
        !memcmp(mixer, run->mixer_state, sizeof run->mixer_state) && cpu->pc == run->pc && cpu->npc == run->npc &&
        cpu->branch_pc == run->branch && cpu->delay_slot == run->delay &&
        cpu->hi == run->hi && cpu->lo == run->lo && !memcmp(cpu->r, run->regs, sizeof run->regs) &&
        cpu->clock == run->clock && cpu->clock->advance == run->binding.advance &&
        cpu->clock->userdata == run->binding.userdata && opening_pitch_word(memory, cpu, &mounted) &&
        mounted == word && opening_pitch_owner(memory, cpu) && opening_pitch_access(memory, word, cpu);
}

static int opening_pitch_run(MusashiBootMemory *memory, FormatterCpu *cpu, const IrqGuestReturn *stack,
    unsigned depth, const MusashiGuestIrqImage *image, const MusashiExecutionClock *clock, MusashiGuestIrqStop *stop)
{
    static const uint8_t caller_digest[32] = {
        0x65, 0x19, 0x87, 0x54, 0x35, 0x25, 0x64, 0x61,
        0x4c, 0x35, 0xcb, 0x7c, 0x74, 0x30, 0xd1, 0x3a,
        0xdd, 0x22, 0x40, 0xfa, 0xb2, 0xc8, 0x91, 0x98,
        0xd1, 0xe4, 0xe8, 0xf6, 0x7c, 0x21, 0xf4, 0x95
    };
    static const uint8_t leaf_digest[32] = {
        0x6c, 0x29, 0xcd, 0x54, 0x12, 0x8c, 0x71, 0x4f,
        0x2e, 0x7f, 0x0e, 0x9c, 0x72, 0x2d, 0x12, 0x71,
        0xf9, 0x0b, 0x76, 0x63, 0x11, 0x3c, 0xdb, 0xa1,
        0xb0, 0x47, 0x4c, 0xc0, 0x02, 0x36, 0x1c, 0x81
    };
    if (depth != 6u || stack[0].callee != IRQ_DISPATCH || stack[1].callee != IRQ_VBLANK ||
        stack[2].callee != IRQ_GAME_VBLANK || stack[3].callee != IRQ_SOUND_TICK ||
        stack[4].callee != IRQ_CHANNEL_TICK || stack[5].callee != IRQ_CHANNEL_UPDATE ||
        stack[5].address != 0x8002d014u || stack[5].sp != 0x8006ca74u ||
        !image || image->identity != image || image->memory != memory || cpu->irq_image != image ||
        !clock || !clock->advance || cpu->clock != clock || cpu->opening_pitch ||
        cpu->pc != OPENING_PITCH_CALL || cpu->npc != cpu->pc + 4u || cpu->delay_slot ||
        cpu->branch_pc || cpu->merge_pending || cpu->gte_load_pending ||
        !irq_channel_update_frame(cpu, OPENING_PITCH_CHANNEL) || !opening_pitch_owner(memory, cpu)) return 0;

    const uint8_t *caller = musashi_boot_ram_span(memory, OPENING_PITCH_CALL, 8u);
    OpeningPitchRun run = {.image = image, .clock = clock, .binding = *clock};
    if (!opening_pitch_digest(caller, sizeof run.caller, caller_digest) ||
        !opening_pitch_digest(image->channel_pitch, sizeof run.leaf, leaf_digest)) return 0;
    memcpy(run.caller, caller, sizeof run.caller);
    memcpy(run.leaf, image->channel_pitch, sizeof run.leaf);
    cpu->opening_pitch = &run;
    const uint8_t *channel = musashi_boot_ram_span(memory, OPENING_PITCH_CHANNEL, sizeof run.channel_state);
    const uint8_t *mixer = musashi_boot_ram_span(memory, OPENING_PITCH_MIXER, sizeof run.mixer_state);
    int entered = 0, returning = 0, ok = 0;
    while (stop->instructions < 8192u) {
        uint32_t word;
        stop->pc = cpu->pc;
        if (!channel || !mixer || !opening_pitch_word(memory, cpu, &word)) break;
        memcpy(run.channel_state, channel, sizeof run.channel_state);
        memcpy(run.mixer_state, mixer, sizeof run.mixer_state);
        run.pc = cpu->pc;
        run.npc = cpu->npc;
        run.branch = cpu->branch_pc;
        run.delay = cpu->delay_slot;
        run.hi = cpu->hi;
        run.lo = cpu->lo;
        memcpy(run.regs, cpu->r, sizeof run.regs);
        if (!opening_pitch_issuer(memory, cpu, word) || !formatter_step_opening(memory, cpu, &stop->instructions)) break;
        ++stop->instructions;
        if (run.pc == OPENING_PITCH_CALL && (!cpu->delay_slot || cpu->branch_pc != run.pc ||
            cpu->npc != OPENING_PITCH_ENTRY || cpu->pc != run.pc + 4u)) break;
        if (run.pc == OPENING_PITCH_CALL + 4u) {
            if (entered || cpu->pc != OPENING_PITCH_ENTRY || cpu->npc != cpu->pc + 4u ||
                cpu->r[29] != OPENING_PITCH_CALLER_SP || cpu->r[4] != OPENING_PITCH_CHANNEL || cpu->r[31] != OPENING_PITCH_RESUME ||
                cpu->delay_slot || cpu->branch_pc) break;
            entered = 1;
        } else if (returning) {
            ok = cpu->pc == OPENING_PITCH_RESUME && cpu->npc == cpu->pc + 4u &&
                !cpu->delay_slot && !cpu->branch_pc && cpu->r[29] == OPENING_PITCH_CALLER_SP && cpu->r[31] == cpu->pc;
            break;
        } else if (entered) {
            returning = (word >> 26) == 0u && (word & 63u) == 8u && ((word >> 21) & 31u) == 31u &&
                cpu->delay_slot && cpu->branch_pc == run.pc && cpu->npc == OPENING_PITCH_RESUME;
            if (cpu->pc - OPENING_PITCH_ENTRY >= sizeof run.leaf ||
                (!returning && cpu->npc - OPENING_PITCH_ENTRY >= sizeof run.leaf)) break;
        }
    }
    cpu->opening_pitch = NULL;
    return ok;
}
