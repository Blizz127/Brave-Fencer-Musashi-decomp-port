/* The primary sequence player consumes an explicit status or reuses its last
 * status. Its resident jump table selects a game handler; the handler keeps
 * its ordinary call frame, device access and clock. */
enum {
    MUSIC_EVENT_START = 0x80038ffcu, MUSIC_EVENT_END = 0x8003916cu,
    MUSIC_EVENT_PARENT_RETURN = 0x80038bfcu, MUSIC_EVENT_TABLE = 0x80073400u
};

typedef struct {
    uint32_t status, call, target, table_entry, table_dest, cursor;
    unsigned issued, clocks;
    uint8_t mounted[MUSIC_EVENT_END - MUSIC_EVENT_START];
} OpeningEvent;

/* Select only the status shapes exercised on this particular track. Other
 * events continue in the existing mounted-instruction dispatcher. */
static int music_event_select(MusashiBootMemory *m, const FormatterCpu *cpu, OpeningEvent *event)
{
    MusicDataTrack track;
    uint32_t cursor, first, status, word;
    uint64_t digest;
    if (!cpu->title_mixer || cpu->pc != MUSIC_EVENT_START ||
        cpu->npc != MUSIC_EVENT_START + 4u || cpu->delay_slot || cpu->branch_pc ||
        cpu->r[4] != MUSIC_PLAYERS || cpu->r[31] != MUSIC_EVENT_PARENT_RETURN ||
        cpu->r[29] != 0x8006ca4cu || cpu->title_mixer->frame_sp != cpu->r[29] ||
        cpu->r[0] || cpu->merge_pending || cpu->gte_load_pending ||
        !music_data_equals(m, 0x800a4e8cu, 2u, 0x91u) ||
        !music_track_range(m, MUSIC_PLAYERS, &track) ||
        !music_data_read(m, MUSIC_PLAYERS, 4u, &cursor) ||
        cursor < track.start || cursor >= track.end || !music_data_read(m, cursor, 1u, &first) ||
        !cpu->title_mixer->music || cpu->title_mixer->symbol != music_game_symbol(MUSIC_EVENT_START) ||
        !title_mixer_word(m, cpu, &word) ||
        !span_digest(m, MUSIC_EVENT_START, sizeof event->mounted / 4u, &digest) ||
        digest != UINT64_C(0x54fa1594356e834c)) return 0;
    const uint8_t *code = musashi_boot_ram_span(m, MUSIC_EVENT_START, sizeof event->mounted);
    if (!code) return 0;
    status = first;
    if (!(first & 128u) && !music_data_read(m, MUSIC_PLAYERS + 0x1f7u, 1u, &status)) return 0;

    uint32_t call, target;
    switch (status >> 4) {
    case 8: call = 0x80039064u; target = 0x80039b20u; break;
    case 9: call = 0x80039074u; target = 0x80039308u; break;
    case 11: call = 0x80039094u; target = 0x80039f50u; break;
    case 12: call = 0x800390a4u; target = 0x8003a098u; break;
    case 15:
        if (status != 255u) return 0;
        call = 0x80039154u; target = 0x8003a234u; break;
    default: return 0;
    }
    *event = (OpeningEvent){
        .status = status, .call = call, .target = target, .cursor = cursor,
        .table_entry = MUSIC_EVENT_TABLE + 4u * ((status >> 4) - 8u),
        .table_dest = status == 255u ? 0x800390d4u : call
    };
    memcpy(event->mounted, code, sizeof event->mounted);
    return 1;
}

static int music_event_table(MusashiBootMemory *m, const FormatterCpu *cpu, const OpeningEvent *e)
{
    const MusashiMain12Owner *owner = cpu->title_mixer->owner;
    return owner->resident_matches(owner, e->table_entry, 4u) &&
        music_data_equals(m, e->table_entry, 4u, e->table_dest);
}

/* These operations are clock checkpoints in the native player flow. The
 * mounted program authenticates each checkpoint; it supplies no operations
 * or timing. A refused clock leaves that issue unretired. */
typedef struct {
    MusashiBootMemory *memory;
    FormatterCpu *cpu;
    TitleMixerRun *run;
    MusashiGuestIrqStop *stop;
    OpeningEvent *event;
} OpeningEventFlow;

static int music_event_bound(const OpeningEventFlow *f)
{
    MusicDataTrack track;
    const uint8_t *code = musashi_boot_ram_span(f->memory, MUSIC_EVENT_START, sizeof f->event->mounted);
    return f->cpu->title_mixer == f->run && f->run->music &&
        f->run->symbol == music_game_symbol(MUSIC_EVENT_START) && f->run->frame_sp == 0x8006ca4cu &&
        f->cpu->r[29] == f->run->frame_sp - (f->cpu->pc == MUSIC_EVENT_START ? 0u : 24u) &&
        code && !memcmp(code, f->event->mounted, sizeof f->event->mounted) &&
        music_data_equals(f->memory, 0x800a4e8cu, 2u, 0x91u) &&
        music_track_range(f->memory, MUSIC_PLAYERS, &track);
}

static int music_event_charge(OpeningEventFlow *f, uint32_t pc, unsigned branch,
                             uint32_t address, unsigned width, unsigned store, uint32_t value)
{
    uint32_t word, live;
    FormatterCpu *cpu = f->cpu;
    TitleMixerRun *run = f->run;
    f->stop->pc = cpu->pc;
    if (cpu->pc != pc || f->stop->instructions >= 8192u - branch ||
        !music_event_bound(f) || !title_mixer_word(f->memory, cpu, &word)) return 0;
    title_mixer_remember_issue(run, cpu, word);
    if (width && !title_mixer_access(f->memory, cpu, address, width, store, value)) return 0;
    if (!run->clock->advance(run->clock->userdata, width ? 3u : 2u) ||
        (cpu->cd_irq_frame && cpu->cd_irq_frame->faulted) ||
        cpu->title_mixer != run || !title_mixer_unchanged(run, cpu, word) ||
        !music_event_bound(f) || !title_mixer_word(f->memory, cpu, &live) || word != live) return 0;
    return !width || title_mixer_access(f->memory, cpu, address, width, store, value);
}

static void music_event_retire(OpeningEventFlow *f, unsigned cost, unsigned branch, uint32_t destination)
{
    FormatterCpu *cpu = f->cpu;
    cpu->branch_pc = branch ? cpu->pc : 0u;
    cpu->pc = cpu->npc;
    cpu->npc = branch ? destination : cpu->pc + 4u;
    cpu->delay_slot = (uint8_t)branch;
    ++f->stop->instructions;
    ++f->event->issued;
    f->event->clocks += cost;
}

/* Register zero represents a genuine idle pipeline slot. */
static int music_event_reg(OpeningEventFlow *f, uint32_t pc, unsigned reg, uint32_t value)
{
    if (!music_event_charge(f, pc, 0u, 0u, 0u, 0u, 0u)) return 0;
    if (reg) f->cpu->r[reg] = value;
    music_event_retire(f, 2u, 0u, 0u);
    return 1;
}

static int music_event_load(OpeningEventFlow *f, uint32_t pc, unsigned reg, uint32_t address, unsigned width)
{
    uint32_t value;
    if (!music_event_charge(f, pc, 0u, address, width, 0u, 0u)) return 0;
    if (width == 1u) {
        uint8_t byte;
        if (!cpu_read8(f->memory, f->cpu, address, &byte)) return 0;
        value = byte;
    } else if (!cpu_read32(f->memory, f->cpu, address, &value)) return 0;
    if (pc == 0x80039004u && value != f->event->cursor) return 0;
    f->cpu->r[reg] = value;
    music_event_retire(f, 3u, 0u, 0u);
    return 1;
}

static int music_event_store(OpeningEventFlow *f, uint32_t pc, uint32_t address, unsigned width, uint32_t value)
{
    if (!music_event_charge(f, pc, 0u, address, width, 1u, value)) return 0;
    if (width == 1u) {
        if (!cpu_write8(f->memory, f->cpu, address, (uint8_t)value)) return 0;
    } else if (!cpu_write32(f->memory, f->cpu, address, value)) return 0;
    music_event_retire(f, 3u, 0u, 0u);
    return 1;
}

static int music_event_jump(OpeningEventFlow *f, uint32_t pc, uint32_t destination, int link)
{
    if (!music_event_charge(f, pc, 1u, 0u, 0u, 0u, 0u)) return 0;
    if (pc == 0x8003905cu && !music_event_table(f->memory, f->cpu, f->event)) return 0;
    if (link) f->cpu->r[31] = pc + 8u;
    music_event_retire(f, 2u, 1u, destination);
    return 1;
}

static int music_event_read_status(OpeningEventFlow *f)
{
    uint32_t *r = f->cpu->r;
    if (!music_event_reg(f, MUSIC_EVENT_START, 29, r[29] - 24u) ||
        !music_event_store(f, 0x80039000u, r[29] + 16u, 4u, r[31]) ||
        !music_event_load(f, 0x80039004u, 3, MUSIC_PLAYERS, 4u) ||
        !music_event_reg(f, 0x80039008u, 0, 0u) ||
        !music_event_load(f, 0x8003900cu, 5, r[3], 1u) ||
        !music_event_reg(f, 0x80039010u, 0, 0u) ||
        !music_event_reg(f, 0x80039014u, 2, r[5] & 128u)) return 0;
    const int explicit_status = r[2] != 0u;
    if (!music_event_jump(f, 0x80039018u, explicit_status ? 0x80039020u : 0x8003902cu, 0) ||
        !music_event_reg(f, 0x8003901cu, 2, r[3] + 1u)) return 0;
    if (explicit_status) {
        if (r[5] != f->event->status ||
            !music_event_store(f, 0x80039020u, MUSIC_PLAYERS, 4u, r[2]) ||
            !music_event_jump(f, 0x80039024u, 0x80039030u, 0) ||
            !music_event_store(f, 0x80039028u, MUSIC_PLAYERS + 0x1f7u, 1u, r[5])) return 0;
    } else if (!music_event_load(f, 0x8003902cu, 5, MUSIC_PLAYERS + 0x1f7u, 1u)) return 0;
    return music_event_reg(f, 0x80039030u, 0, 0u) && r[5] == f->event->status;
}

static int music_event_enter_handler(OpeningEventFlow *f)
{
    uint32_t *r = f->cpu->r;
    const OpeningEvent *e = f->event;
    if (!music_event_read_status(f) ||
        !music_event_reg(f, 0x80039034u, 2, r[5] >> 4) ||
        !music_event_reg(f, 0x80039038u, 3, r[2] - 8u) ||
        !music_event_reg(f, 0x8003903cu, 2, r[3] < 8u) || !r[2] ||
        !music_event_jump(f, 0x80039040u, 0x80039048u, 0) ||
        !music_event_reg(f, 0x80039044u, 0, 0u) ||
        !music_event_reg(f, 0x80039048u, 2, r[3] << 2) ||
        !music_event_reg(f, 0x8003904cu, 1, MUSIC_EVENT_TABLE & 0xffff0000u) ||
        !music_event_reg(f, 0x80039050u, 1, r[1] + r[2]) || r[1] + 0x3400u != e->table_entry ||
        !music_event_load(f, 0x80039054u, 2, e->table_entry, 4u) ||
        !music_event_reg(f, 0x80039058u, 0, 0u) || r[2] != e->table_dest ||
        !music_event_table(f->memory, f->cpu, e) ||
        !music_event_jump(f, 0x8003905cu, r[2], 0) ||
        !music_event_reg(f, 0x80039060u, 0, 0u)) return 0;
    const int meta = e->status == 255u;
    if (meta) {
        if (!music_event_reg(f, 0x800390d4u, 5, r[5] & 15u) ||
            !music_event_reg(f, 0x800390d8u, 2, 7u) ||
            !music_event_jump(f, 0x800390dcu, 0x800390e4u, 0) ||
            !music_event_reg(f, 0x800390e0u, 2, r[5] < 8u) ||
            !music_event_jump(f, 0x800390e4u, 0x800390fcu, 0) ||
            !music_event_reg(f, 0x800390e8u, 0, 0u) ||
            !music_event_reg(f, 0x800390fcu, 2, 15u) ||
            !music_event_jump(f, 0x80039100u, e->call, 0) ||
            !music_event_reg(f, 0x80039104u, 0, 0u)) return 0;
    }
    return music_callee(MUSIC_EVENT_START, e->call, e->target) &&
        music_event_jump(f, e->call, e->target, 1) &&
        music_event_reg(f, e->call + 4u, meta ? 0u : 5u, r[5] & 15u);
}

static int music_event_leave_handler(OpeningEventFlow *f)
{
    uint32_t *r = f->cpu->r;
    uint32_t ret = f->event->call + 8u;
    if (ret != 0x8003915cu && (!music_event_jump(f, ret, 0x8003915cu, 0) ||
        !music_event_reg(f, ret + 4u, 0, 0u))) return 0;
    return music_event_load(f, 0x8003915cu, 31, r[29] + 16u, 4u) &&
        music_event_reg(f, 0x80039160u, 2, 0u) && r[31] == MUSIC_EVENT_PARENT_RETURN &&
        music_event_jump(f, 0x80039164u, r[31], 0) &&
        music_event_reg(f, 0x80039168u, 29, r[29] + 24u);
}

static int music_event_segment(MusashiBootMemory *m, FormatterCpu *cpu, TitleMixerRun *run,
    MusashiGuestIrqStop *stop, OpeningEvent *e, int returning)
{
    const uint32_t begin = returning ? e->call + 8u : MUSIC_EVENT_START;
    const uint32_t end = returning ? MUSIC_EVENT_PARENT_RETURN : e->target;
    if (!run || cpu->title_mixer != run || run->frame_sp != 0x8006ca4cu ||
        cpu->pc != begin || cpu->npc != begin + 4u || cpu->delay_slot || cpu->branch_pc ||
        cpu->r[29] != run->frame_sp - (returning ? 24u : 0u) ||
        cpu->r[31] != (returning ? begin : MUSIC_EVENT_PARENT_RETURN) ||
        cpu->r[0] || cpu->merge_pending || cpu->gte_load_pending) return 0;
    OpeningEventFlow f = {m, cpu, run, stop, e};
    return (returning ? music_event_leave_handler(&f) : music_event_enter_handler(&f)) &&
        cpu->pc == end && cpu->npc == end + 4u && !cpu->delay_slot && !cpu->branch_pc &&
        cpu->r[29] == run->frame_sp - (returning ? 0u : 24u);
}

typedef struct {
    unsigned note_off, note_on, controller, program, meta;
} OpeningEventCounts;

static void music_event_observed(const OpeningEvent *event, OpeningEventCounts *counts)
{
    static _Thread_local unsigned seen;
    unsigned family = event->status >> 4;
    switch (family) {
    case 8: ++counts->note_off; break;
    case 9: ++counts->note_on; break;
    case 11: ++counts->controller; break;
    case 12: ++counts->program; break;
    case 15: ++counts->meta; break;
    }
    if (!(seen & (1u << family))) {
        seen |= 1u << family;
        fprintf(stderr, "native_boot: OPENING_EVENT_RETURN status=%02x call=%08x child=%08x issues=%u clocks=%u\n",
                (unsigned)event->status, (unsigned)event->call, (unsigned)event->target, event->issued, event->clocks);
    }
}
