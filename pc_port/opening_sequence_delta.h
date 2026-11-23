/* Native counterpart of the game's variable-length event delay reader.
 * Its caller can observe the cursor store and the registers at sound-device
 * clock edges. Keep those boundaries while reading the mounted sequence. */
enum { MUSIC_DELTA_START = 0x80038fc4u, MUSIC_DELTA_RETURN = 0x80038c14u };

static int music_delta_image(MusashiBootMemory *memory, const FormatterCpu *cpu)
{
    const TitleMixerRun *run = cpu->title_mixer;
    uint32_t word;
    uint64_t digest;
    return run && run->music && run->symbol == music_game_symbol(MUSIC_DELTA_START) &&
        title_mixer_word(memory, cpu, &word) &&
        span_digest(memory, MUSIC_DELTA_START, 14u, &digest) &&
        digest == UINT64_C(0x87738257df5139e3);
}

static int music_delta_native(MusashiBootMemory *memory, FormatterCpu *cpu,
                              TitleMixerRun *run, MusashiGuestIrqStop *stop)
{
    const unsigned started = stop->instructions;
    unsigned bytes = 0;
    if (cpu->pc != MUSIC_DELTA_START || cpu->npc != MUSIC_DELTA_START + 4u ||
        cpu->r[4] != MUSIC_PLAYERS || cpu->r[31] != MUSIC_DELTA_RETURN ||
        !run || cpu->title_mixer != run || cpu->r[29] != run->frame_sp ||
        cpu->r[0] || cpu->delay_slot || cpu->branch_pc || cpu->merge_pending ||
        cpu->gte_load_pending)
        return 0;

    do {
        const uint32_t pc = cpu->pc, next = cpu->npc;
        const unsigned offset = pc - MUSIC_DELTA_START;
        const int control = offset == 32u || offset == 40u || offset == 48u;
        const int store = offset == 20u;
        const unsigned width = offset == 4u || store ? 4u : offset == 12u ? 1u : 0u;
        const uint32_t address = offset == 12u ? cpu->r[2] : cpu->r[4];
        const uint32_t value = store ? cpu->r[2] : 0u;
        const uint32_t hi = cpu->hi, lo = cpu->lo, branch = cpu->branch_pc;
        const unsigned delay = cpu->delay_slot;
        uint32_t after = next + 4u, regs[32], cursor;
        uint8_t byte;

        stop->pc = run->issued_pc = pc;
        if (offset >= 56u || (offset & 3u) || !music_delta_image(memory, cpu) ||
            stop->instructions >= 8192u - (unsigned)control ||
            (width && !title_mixer_access(memory, cpu, address, width, store, value)))
            return 0;

        memcpy(regs, cpu->r, sizeof regs);
        if (!run->clock->advance(run->clock->userdata, width ? 3u : 2u) ||
            cpu->pc != pc || cpu->npc != next || cpu->branch_pc != branch ||
            cpu->delay_slot != delay || cpu->hi != hi || cpu->lo != lo ||
            cpu->title_mixer != run || memcmp(regs, cpu->r, sizeof regs) ||
            (cpu->cd_irq_frame && cpu->cd_irq_frame->faulted) ||
            !music_delta_image(memory, cpu))
            return 0;

        switch (offset) {
        case 0u: cpu->r[5] = 0u; break;
        case 4u:
            if (!cpu_read32(memory, cpu, address, &cursor)) return 0;
            cpu->r[2] = cursor;
            break;
        case 8u: break;
        case 12u:
            if (!cpu_read8(memory, cpu, address, &byte)) return 0;
            cpu->r[3] = byte;
            break;
        case 16u: ++cpu->r[2]; break;
        case 20u:
            if (!cpu_write32(memory, cpu, address, value)) return 0;
            ++bytes;
            break;
        case 24u: cpu->r[2] = cpu->r[3] & 127u; break;
        case 28u: cpu->r[3] &= 128u; break;
        case 32u:
            if (!cpu->r[3]) after = MUSIC_DELTA_START + 48u;
            break;
        case 36u: cpu->r[5] += cpu->r[2]; break;
        case 40u: after = MUSIC_DELTA_START + 4u; break;
        case 44u: cpu->r[5] <<= 7; break;
        case 48u: after = cpu->r[31]; break;
        case 52u: cpu->r[2] = cpu->r[5]; break;
        default: return 0;
        }

        cpu->pc = next;
        cpu->npc = after;
        cpu->delay_slot = (uint8_t)control;
        cpu->branch_pc = control ? pc : 0u;
        ++stop->instructions;
    } while (cpu->pc != MUSIC_DELTA_RETURN);

    if (cpu->npc != MUSIC_DELTA_RETURN + 4u || cpu->delay_slot || cpu->branch_pc)
        return 0;
    static _Thread_local unsigned reported;
    const unsigned mask = bytes == 1u ? 1u : 2u;
    if (!(reported & mask)) {
        reported |= mask;
        fprintf(stderr, "native_boot: OPENING_SEQUENCE_DELTA bytes=%u result=%u native_issues=%u\n",
                bytes, (unsigned)cpu->r[2], stop->instructions - started);
    }
    return 1;
}
