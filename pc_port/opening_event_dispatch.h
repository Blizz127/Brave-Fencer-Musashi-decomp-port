/* An opening event can call only the handler selected by its mounted score.
 * The ordinary title transport executes the dispatcher, handler and return. */
enum {
    MUSIC_EVENT_START = 0x80038ffcu, MUSIC_EVENT_END = 0x8003916cu,
    MUSIC_EVENT_PARENT_RETURN = 0x80038bfcu, MUSIC_EVENT_TABLE = 0x80073400u
};
typedef struct OpeningEvent {
    uint32_t status, call, target, table_entry, table_dest, cursor;
    unsigned returned, issued, clocks;
    uint8_t mounted[MUSIC_EVENT_END - MUSIC_EVENT_START];
} OpeningEvent;
typedef struct {
    unsigned note_off, note_on, controller, program, meta;
} OpeningEventCounts;

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

static int music_event_issue(MusashiBootMemory *m, const FormatterCpu *cpu, uint32_t word)
{
    const TitleMixerRun *run = cpu->title_mixer;
    const OpeningEvent *event = run->event;
    MusicDataTrack track;
    uint32_t op = word >> 26;
    uint32_t source = (word >> 21) & 31u;
    const uint8_t *code = musashi_boot_ram_span(m, MUSIC_EVENT_START, sizeof event->mounted);
    if (!run->music || run->symbol != music_game_symbol(MUSIC_EVENT_START) ||
        run->frame_sp != 0x8006ca4cu ||
        cpu->r[29] != run->frame_sp - (cpu->pc == MUSIC_EVENT_START ? 0u : 24u) ||
        !code || memcmp(code, event->mounted, sizeof event->mounted) ||
        !music_data_equals(m, 0x800a4e8cu, 2u, 0x91u) ||
        !music_track_range(m, MUSIC_PLAYERS, &track))
        return 0;
    if ((op == 32u || op == 36u) && !event->returned) {
        uint32_t address = cpu->r[source] + (uint32_t)(int32_t)(int16_t)word;
        uint32_t status;
        if (address == event->cursor) {
            if (!music_data_read(m, address, 1u, &status)) return 0;
            if (!(status & 128u) && !music_data_read(m, MUSIC_PLAYERS + 0x1f7u, 1u, &status))
                return 0;
            if (status != event->status) return 0;
        } else if (address == MUSIC_PLAYERS + 0x1f7u &&
                   !music_data_equals(m, address, 1u, event->status)) {
            return 0;
        }
    }
    if (!op && (word & 63u) == 8u && source != 31u)
        return cpu->r[source] == event->table_dest && music_event_table(m, cpu, event);
    if (op == 3u)
        return cpu->pc == event->call && !event->returned &&
            (((cpu->pc + 4u) & 0xf0000000u) | ((word & 0x03ffffffu) << 2)) == event->target;
    if (!op && (word & 63u) == 8u && source == 31u)
        return event->returned && cpu->r[31] == MUSIC_EVENT_PARENT_RETURN;
    return 1;
}

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
