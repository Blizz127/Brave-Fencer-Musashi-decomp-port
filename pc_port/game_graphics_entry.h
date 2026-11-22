/* The ordinary CPU authenticates and executes the startup library exchange.
 * A direct return to this active game frame lets it issue the graphics call. */
static void game_exchange_retired(LaneRunFrame *frame, MusashiBootMemory *memory,
                                  const FormatterCpu *cpu, uint32_t issued)
{
    if (!frame || !frame->main_entry.follow_graphics || !frame->main_entry.next) return;
    GameMainEntry *entry = &frame->main_entry;
    const uint32_t last = GAME_EXCHANGE+STARTUP_EXCHANGE_SIZE-4u;
    const int returning = issued == last-4u;
    const uint32_t pc = issued == last ? POST_EXCHANGE : issued+4u;
    const uint32_t npc = returning ? POST_EXCHANGE : pc+4u;
    const int ok = entry->next == issued && game_main_owner(frame,memory,cpu) &&
        !lane_find(issued) && cpu->r[29] == 0x801fffc0u && cpu->r[30] == cpu->r[29] &&
        cpu->r[31] == POST_EXCHANGE && cpu->pc == pc && cpu->npc == npc &&
        cpu->delay_slot == (unsigned)returning && cpu->branch_pc == (returning ? issued : 0u);
    entry->next = ok ? pc : 0u;
}

/* The graphics wrapper may spend many calls inside the graphics library.
 * Resume its caller only after the guest CPU retires the wrapper's own JR and
 * delay slot on the original root and physical stack. */
static void game_graphics_return_retired(LaneRunFrame *frame, MusashiBootMemory *memory,
                                         const FormatterCpu *cpu, uint32_t issued)
{
    if (!frame || !frame->main_entry.return_stage) return;
    GameMainEntry *entry = &frame->main_entry;
    const int jr = issued == GRAPHICS_RETURN_JR;
    const uint8_t *live = musashi_boot_ram_span(memory,GRAPHICS_RETURN_JR,sizeof entry->graphics_return);
    uint32_t saved_ra;
    if (entry->return_stage != (jr ? 1u : 2u) || !game_main_owner(frame,memory,cpu) ||
        !entry->capture_return || lane_find(issued) ||
        !intrinsic_span(cpu,GRAPHICS_RETURN_JR,sizeof entry->graphics_return) ||
        !live || memcmp(live,entry->graphics_return,sizeof entry->graphics_return) ||
        cpu->pc != (jr ? GRAPHICS_RETURN_JR+4u : POST_GRAPHICS) ||
        cpu->npc != (jr ? POST_GRAPHICS : POST_GRAPHICS+4u) ||
        cpu->delay_slot != (unsigned)jr || cpu->branch_pc != (jr ? issued : 0u) ||
        cpu->r[31] != POST_GRAPHICS || cpu->r[29] != 0x801fffc0u || cpu->r[30] != cpu->r[29] ||
        !musashi_boot_read32(memory,cpu->r[29]-8u,&saved_ra) || saved_ra != POST_GRAPHICS) {
        entry->return_stage = 0;
        return;
    }
    entry->return_stage = jr ? 2u : 0u;
    if (!jr) entry->next = POST_GRAPHICS;
}

static int game_graphics_pc(uint32_t pc)
{
    return (uint32_t)(pc-POST_EXCHANGE) < 32u || (uint32_t)(pc-GRAPHICS_ENTRY) < 16u;
}

typedef struct GameGraphicsSpan {
    uint32_t start, call, target, parent;
    const uint8_t *code;
    size_t size;
} GameGraphicsSpan;

static GameGraphicsSpan game_graphics_span(const GameMainEntry *entry, uint32_t pc)
{
    if (pc >= GRAPHICS_ENTRY)
        return (GameGraphicsSpan){GRAPHICS_ENTRY,GRAPHICS_LIBRARY_CALL,GRAPHICS_LIBRARY,
                                  POST_GRAPHICS,entry->graphics,sizeof entry->graphics};
    if (pc >= POST_GRAPHICS)
        return (GameGraphicsSpan){POST_GRAPHICS,CD_START_CALL,CD_START_ENTRY,
                                  POST_GRAPHICS,entry->cd_start,sizeof entry->cd_start};
    return (GameGraphicsSpan){POST_EXCHANGE,GRAPHICS_CALL,GRAPHICS_ENTRY,
                              POST_EXCHANGE,entry->post_exchange,sizeof entry->post_exchange};
}

static int game_graphics_frame(LaneRunFrame *frame, MusashiBootMemory *memory,
                               const FormatterCpu *cpu, uint32_t pc)
{
    const GameGraphicsSpan span = game_graphics_span(&frame->main_entry,pc);
    const int delay = pc == span.call+4u;
    const uint32_t stack = span.start == GRAPHICS_ENTRY && pc != span.start ? 0x801fffa8u : 0x801fffc0u;
    return game_main_owner(frame,memory,cpu) && frame->main_entry.follow_graphics &&
        frame->main_entry.next == pc && cpu->pc == pc &&
        cpu->npc == (delay ? span.target : pc+4u) &&
        cpu->delay_slot == (unsigned)delay && cpu->branch_pc == (delay ? span.call : 0u) &&
        cpu->r[31] == (delay ? span.call+8u : span.parent) && cpu->r[29] == stack &&
        cpu->r[30] == 0x801fffc0u && (pc != POST_EXCHANGE+8u || cpu->r[1] == 0x80070000u);
}

static int game_graphics_issue(LaneRunFrame *frame, MusashiBootMemory *memory, FormatterCpu *cpu)
{
    GameMainEntry *entry = &frame->main_entry;
    const uint32_t pc = cpu->pc;
    const GameGraphicsSpan span = game_graphics_span(entry,pc);
    const uint8_t *live = musashi_boot_ram_span(memory,span.start,span.size);
    const int store = pc == POST_EXCHANGE+8u || pc == GRAPHICS_ENTRY+4u;
    const uint32_t address = pc == GRAPHICS_ENTRY+4u ? cpu->r[29]+16u : 0x80074778u;
    if (!game_graphics_frame(frame,memory,cpu,pc) || !live || !intrinsic_span(cpu,span.start,span.size) ||
        memcmp(live,span.code,span.size) || lane_find(pc) || (store && !intrinsic_span(cpu,address,4u))) {
        entry->next = 0;
        return 0;
    }
    if (!entry->charge.advance(entry->charge.userdata,store ? 3u : 2u) ||
        !game_graphics_frame(frame,memory,cpu,pc) || memcmp(live,span.code,span.size) || lane_find(pc)) {
        entry->next = 0;
        return -1;
    }
    if (store) {
        const uint32_t value = pc == GRAPHICS_ENTRY+4u ? cpu->r[31] : cpu->r[2];
        if (!musashi_boot_write32(memory,address,value)) {
            entry->next = 0;
            return -1;
        }
    } else if (pc == POST_EXCHANGE) cpu->r[2] = 992u;
    else if (pc == POST_EXCHANGE+4u) cpu->r[1] = 0x80070000u;
    else if (pc == GRAPHICS_ENTRY) cpu->r[29] -= 24u;
    else if (pc == GRAPHICS_LIBRARY_CALL+4u || pc == POST_GRAPHICS) cpu->r[4] = 0u;
    else if (pc == span.call) {
        cpu->r[31] = pc+8u;
        if (g_trace_state >= 0) musashi_trace_function_entry(span.target,pc,'j');
        if (g_entry_hook) g_entry_pending = span.target;
    }
    cpu->pc = cpu->npc;
    cpu->npc = pc == span.call ? span.target : cpu->pc+4u;
    cpu->delay_slot = pc == span.call;
    cpu->branch_pc = cpu->delay_slot ? span.call : 0u;
    if (cpu->pc == GRAPHICS_LIBRARY && entry->capture_return)
        entry->return_stage = 1u;
    entry->next = (cpu->pc == GRAPHICS_LIBRARY || cpu->pc == CD_START_ENTRY) ? 0u : cpu->pc;
    return 1;
}
