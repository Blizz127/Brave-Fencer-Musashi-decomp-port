/* Opening track's player-status query, from the matched game C in
 * src/main/8003836c.c. The sound tick calls it after the voice sweep.
 * Charge each original issue so events can change the status between reads. */
enum { MUSIC_STATUS_START = 0x8003836cu, MUSIC_STATUS_RETURN = 0x8002cf34u };

static int music_status_image(MusashiBootMemory *m, const FormatterCpu *cpu)
{
    const TitleMixerRun *run = cpu->title_mixer;
    uint32_t mounted_word;
    uint64_t fingerprint;
    return run && run->music && run->symbol == music_game_symbol(MUSIC_STATUS_START) &&
        title_mixer_word(m, cpu, &mounted_word) &&
        span_digest(m, MUSIC_STATUS_START, 14u, &fingerprint) &&
        fingerprint == UINT64_C(0xf9160474c0017a5a);
}

/* Execute only the already bound player-zero call. Reads go through the same
 * game-RAM accessor as the ordinary sound interrupt. Keep v1 and at as well
 * as v0 because an interrupted caller can observe the complete register file. */
static int music_status_native(MusashiBootMemory *m, FormatterCpu *cpu,
                               TitleMixerRun *run, MusashiGuestIrqStop *stop)
{
    unsigned issued = 0, clocks = 0;
    int active = 0;
    if (cpu->pc != MUSIC_STATUS_START || cpu->npc != cpu->pc + 4u ||
        cpu->r[4] != 0u || cpu->r[31] != MUSIC_STATUS_RETURN ||
        cpu->delay_slot || cpu->branch_pc || cpu->r[0] ||
        run->root_sp != cpu->r[29] || run->frame_sp != run->root_sp)
        return 0;
    do {
        const uint32_t pc = cpu->pc, next = cpu->npc, old_branch = cpu->branch_pc;
        const uint32_t offset = pc - MUSIC_STATUS_START;
        const unsigned delay = cpu->delay_slot;
        const int branch = offset == 28u || offset == 48u;
        const int read = offset == 20u || offset == 44u;
        const unsigned cost = read ? 3u : 2u;
        uint32_t after = next + 4u, regs[32], hi = cpu->hi, lo = cpu->lo;
        uint8_t byte;
        run->issued_pc = stop->pc = pc;
        if (offset >= 56u || (offset & 3u) || cpu->title_mixer != run ||
            stop->instructions >= 8192u - (unsigned)branch ||
            !music_status_image(m, cpu))
            return 0;
        uint32_t address = 0;
        if (read) {
            address = cpu->r[1] - (offset == 20u ? 0x612du : 0x612eu);
            if (!title_mixer_access(m, cpu, address, 1u, 0u, 0u))
                return 0;
        }
        memcpy(regs, cpu->r, sizeof regs);
        if (!run->clock->advance(run->clock->userdata, cost) ||
            cpu->pc != pc || cpu->npc != next || cpu->branch_pc != old_branch ||
            cpu->delay_slot != delay || cpu->hi != hi || cpu->lo != lo ||
            cpu->title_mixer != run || memcmp(regs, cpu->r, sizeof regs) ||
            (cpu->cd_irq_frame && cpu->cd_irq_frame->faulted) ||
            !music_status_image(m, cpu))
            return 0;
        switch (offset) {
        case 0u: cpu->r[2] = cpu->r[4] << 7; break;
        case 4u: cpu->r[2] -= cpu->r[4]; break;
        case 8u: cpu->r[3] = cpu->r[2] << 2; break;
        case 12u: case 36u: cpu->r[1] = 0x800c0000u; break;
        case 16u: case 40u: cpu->r[1] += cpu->r[3]; break;
        case 20u: case 44u:
            if (!cpu_read8(m, cpu, address, &byte)) return 0;
            cpu->r[2] = byte;
            break;
        case 24u: case 52u: break;
        case 28u:
            active = cpu->r[2] != 0u;
            if (!active) after = MUSIC_STATUS_START + 48u;
            break;
        case 32u: cpu->r[2] = 0u; break;
        case 48u: after = cpu->r[31]; break;
        default: return 0;
        }
        cpu->pc = next;
        cpu->npc = after;
        cpu->delay_slot = (uint8_t)branch;
        cpu->branch_pc = branch ? pc : 0u;
        ++stop->instructions;
        ++issued;
        clocks += cost;
    } while (cpu->pc != MUSIC_STATUS_RETURN);

    if (cpu->npc != MUSIC_STATUS_RETURN + 4u || cpu->branch_pc || cpu->delay_slot)
        return 0;
    static _Thread_local unsigned reported;
    const unsigned mask = active ? 2u : 1u;
    if (!(reported & mask)) {
        reported |= mask;
        fprintf(stderr, "native_boot: OPENING_PLAYER_STATUS active=%d value=%u native_issues=%u clocks=%u\n",
                active, (unsigned)cpu->r[2], issued, clocks);
    }
    return 1;
}
