/* Opening sequencer note-on. A previously initialized voice can keep its
 * sample and envelope when its program, tone and bank still match. Only
 * volume and pitch then need to be refreshed. The common synthesis and
 * key-on paths follow both exits in the mounted game. */
enum {
    MUSIC_CACHE_COMPARE = 0x80039568u,
    MUSIC_CACHE_RELOAD = 0x800395acu,
    MUSIC_CACHE_READY = 0x80039604u
};

static int music_cache_image(MusashiBootMemory *m, const FormatterCpu *cpu)
{
    uint32_t word;
    uint64_t digest;
    return cpu->title_mixer && cpu->title_mixer->music &&
        cpu->title_mixer->symbol == music_game_symbol(0x80039308u) &&
        title_mixer_word(m, cpu, &word) &&
        span_digest(m, MUSIC_CACHE_COMPARE, 17u, &digest) &&
        digest == UINT64_C(0x88906846e5a04f09);
}

static int music_cache_owner(MusashiBootMemory *m, const FormatterCpu *cpu)
{
    const uint32_t player = cpu->r[14], voice = cpu->r[6], slot = cpu->r[15];
    const uint32_t program = cpu->r[20], tone = cpu->r[24];
    uint32_t channel, cursor, tones;
    MusicDataTrack track;
    if (!music_cache_image(m, cpu) || slot >= 16u ||
        voice != MUSIC_VOICES + MUSIC_VOICE_BYTES * slot || cpu->r[7] != voice + 0x10u ||
        player != MUSIC_PLAYERS || program >= 128u || tone >= 16u ||
        cpu->r[31] != 0x8003907cu || cpu->r[29] != 0x8006c9f4u ||
        cpu->title_mixer->frame_sp != cpu->r[29] + 0x40u ||
        !music_data_read(m, cpu->r[29], 2u, &channel) || channel >= 16u ||
        cpu->r[16] != player + 0x1au + channel * 0x1au ||
        !music_data_equals(m, cpu->r[16], 1u, program) ||
        !music_data_equals(m, player+0x1f7u, 1u, 0x90u + channel) ||
        !music_data_equals(m, player+0x1fau, 1u, 1u) ||
        !music_data_equals(m, 0x800a4e8cu, 2u, 0x91u) ||
        !music_track_range(m, player, &track) || !music_data_read(m, player, 4u, &cursor) ||
        cursor < track.start+2u || cursor > track.end ||
        cpu->r[21] > 127u || !cpu->r[30] || cpu->r[30] > 127u ||
        !music_data_equals(m, cursor-2u, 1u, cpu->r[21]) ||
        !music_data_equals(m, cursor-1u, 1u, cpu->r[30]) ||
        !music_data_equals(m, player+0x1dcu, 4u, MUSIC_INSTRUMENTS + 0x20u) ||
        !music_data_equals(m, player+0x1e0u, 4u, MUSIC_INSTRUMENTS + 0x820u) ||
        !music_data_read(m, MUSIC_INSTRUMENTS+0x20u+16u*program, 1u, &tones) ||
        tones > 16u || tone >= tones ||
        cpu->r[25] != MUSIC_INSTRUMENTS+0x820u + program*512u + tone*32u ||
        cpu->r[12] != cpu->r[25] + 1u ||
        !music_data_access(m, cpu->title_mixer->root_sp, cpu->r[29], cpu->r[25], 1u, 0, 0u))
        return 0;
    return music_data_equals(m, voice+0x10u, 4u, 1u << slot) &&
        music_data_equals(m, voice+0x50u, 4u, player) &&
        music_data_equals(m, voice+0x5cu, 1u, 1u);
}

/* A generic formatter write cannot gain this permission. Even the native
 * path must be in the taken match jump's delay slot and still own its slot. */
static int music_cache_store(MusashiBootMemory *m, const FormatterCpu *cpu, uint32_t address)
{
    return cpu->pc == MUSIC_CACHE_COMPARE + 64u && cpu->delay_slot &&
        cpu->branch_pc == MUSIC_CACHE_COMPARE + 60u && cpu->npc == MUSIC_CACHE_READY &&
        cpu->r[2] == 0x13u && address == cpu->r[6]+0x14u && music_cache_owner(m, cpu);
}

static int music_cache_native(MusashiBootMemory *m, FormatterCpu *cpu,
                              TitleMixerRun *run, MusashiGuestIrqStop *stop)
{
    unsigned issued = 0, clocks = 0;
    if (cpu->pc != MUSIC_CACHE_COMPARE || cpu->npc != cpu->pc+4u ||
        cpu->r[0] || cpu->delay_slot || cpu->branch_pc || cpu->merge_pending || cpu->gte_load_pending)
        return 0;
    do {
        const uint32_t pc = cpu->pc, next = cpu->npc, offset = pc - MUSIC_CACHE_COMPARE;
        const int branch = offset == 8u || offset == 24u || offset == 52u || offset == 60u;
        uint32_t address = 0, width = 0, after = next + 4u;
        const int store = offset == 64u;
        if (offset == 0u) { address = cpu->r[6]+8u; width = 2u; }
        if (offset == 16u) { address = cpu->r[6]+12u; width = 1u; }
        if (offset == 32u) { address = cpu->r[6]+0x50u; width = 4u; }
        if (offset == 40u) { address = cpu->r[2]+0x1ecu; width = 2u; }
        if (offset == 44u) { address = cpu->r[14]+0x1ecu; width = 2u; }
        if (store) { address = cpu->r[6]+0x14u; width = 4u; }
        run->issued_pc = stop->pc = pc;
        if (offset > 64u || (offset & 3u) || !music_cache_owner(m, cpu) ||
            stop->instructions >= 8192u - (unsigned)branch ||
            (width && !title_mixer_access(m, cpu, address, width, store, store ? cpu->r[2] : 0)))
            return 0;
        const uint32_t hi = cpu->hi, lo = cpu->lo, origin = cpu->branch_pc;
        const unsigned delay = cpu->delay_slot;
        uint32_t regs[32];
        memcpy(regs, cpu->r, sizeof regs);
        const unsigned cost = width ? 3u : 2u;
        if (!run->clock->advance(run->clock->userdata, cost) || cpu->pc != pc || cpu->npc != next ||
            cpu->branch_pc != origin || cpu->delay_slot != delay || cpu->hi != hi || cpu->lo != lo ||
            cpu->title_mixer != run || memcmp(regs, cpu->r, sizeof regs) ||
            (cpu->cd_irq_frame && cpu->cd_irq_frame->faulted) || !music_cache_owner(m, cpu))
            return 0;
        uint16_t half;
        uint8_t byte;
        switch (offset) {
        case 0u: case 40u: case 44u:
            if (!cpu_read16(m, cpu, address, &half)) return 0;
            cpu->r[offset == 40u ? 3 : 2] = (uint32_t)(int32_t)(int16_t)half;
            break;
        case 4u: case 12u: case 20u: case 28u: case 36u: case 48u: break;
        case 8u:  if (cpu->r[2] != cpu->r[20]) after = MUSIC_CACHE_RELOAD; break;
        case 16u:
            if (!cpu_read8(m, cpu, address, &byte)) return 0;
            cpu->r[2] = byte;
            break;
        case 24u: if (cpu->r[2] != cpu->r[24]) after = MUSIC_CACHE_RELOAD; break;
        case 32u: if (!cpu_read32(m, cpu, address, &cpu->r[2])) return 0; break;
        case 52u: if (cpu->r[3] != cpu->r[2]) after = MUSIC_CACHE_RELOAD; break;
        case 56u: cpu->r[2] = 0x13u; break;
        case 60u: after = MUSIC_CACHE_READY; break;
        case 64u:
            if (!cpu_write32(m, cpu, address, cpu->r[2])) return 0;
            break;
        default: return 0;
        }
        cpu->pc = next;
        cpu->npc = after;
        cpu->delay_slot = (uint8_t)branch;
        cpu->branch_pc = branch ? pc : 0u;
        ++stop->instructions;
        ++issued;
        clocks += cost;
    } while (cpu->pc != MUSIC_CACHE_RELOAD && cpu->pc != MUSIC_CACHE_READY);
    if (cpu->npc != cpu->pc + 4u || cpu->delay_slot || cpu->branch_pc) return 0;
    static _Thread_local unsigned reported;
    const unsigned bit = cpu->pc == MUSIC_CACHE_READY ? 2u : 1u;
    if (!(reported & bit)) {
        reported |= bit;
        fprintf(stderr, "native_boot: OPENING_VOICE_REUSE slot=%u note=%u velocity=%u cached=%u native_issues=%u clocks=%u\n",
                (unsigned)cpu->r[15], (unsigned)cpu->r[21], (unsigned)cpu->r[30],
                (unsigned)(bit == 2u), issued, clocks);
    }
    return 1;
}
