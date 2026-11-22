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

static int game_graphics_pc(uint32_t pc)
{
    return (uint32_t)(pc-POST_EXCHANGE) < 20u || (uint32_t)(pc-GRAPHICS_ENTRY) < 16u;
}

static int game_graphics_frame(LaneRunFrame *frame, MusashiBootMemory *memory,
                               const FormatterCpu *cpu, uint32_t pc)
{
    const int in_wrapper = pc >= GRAPHICS_ENTRY;
    const uint32_t call = in_wrapper ? GRAPHICS_LIBRARY_CALL : GRAPHICS_CALL;
    const int delay = pc == call+4u;
    const uint32_t parent = in_wrapper ? GRAPHICS_CALL+8u : POST_EXCHANGE;
    const uint32_t stack = in_wrapper && pc != GRAPHICS_ENTRY ? 0x801fffa8u : 0x801fffc0u;
    return game_main_owner(frame,memory,cpu) && frame->main_entry.follow_graphics &&
        frame->main_entry.next == pc && cpu->pc == pc &&
        cpu->npc == (delay ? (in_wrapper ? GRAPHICS_LIBRARY : GRAPHICS_ENTRY) : pc+4u) &&
        cpu->delay_slot == (unsigned)delay && cpu->branch_pc == (delay ? call : 0u) &&
        cpu->r[31] == (delay ? call+8u : parent) && cpu->r[29] == stack &&
        cpu->r[30] == 0x801fffc0u && (pc != POST_EXCHANGE+8u || cpu->r[1] == 0x80070000u);
}

static int game_graphics_issue(LaneRunFrame *frame, MusashiBootMemory *memory, FormatterCpu *cpu)
{
    GameMainEntry *entry = &frame->main_entry;
    const uint32_t pc = cpu->pc;
    const int in_wrapper = pc >= GRAPHICS_ENTRY;
    const uint32_t start = in_wrapper ? GRAPHICS_ENTRY : POST_EXCHANGE;
    const size_t size = in_wrapper ? sizeof entry->graphics : sizeof entry->post_exchange;
    const uint8_t *snapshot = in_wrapper ? entry->graphics : entry->post_exchange;
    const uint8_t *live = musashi_boot_ram_span(memory,start,size);
    const int store = pc == POST_EXCHANGE+8u || pc == GRAPHICS_ENTRY+4u;
    const uint32_t address = pc == GRAPHICS_ENTRY+4u ? cpu->r[29]+16u : 0x80074778u;
    if (!game_graphics_frame(frame,memory,cpu,pc) || !live || !intrinsic_span(cpu,start,size) ||
        memcmp(live,snapshot,size) || lane_find(pc) || (store && !intrinsic_span(cpu,address,4u))) {
        entry->next = 0;
        return 0;
    }
    if (!entry->charge.advance(entry->charge.userdata,store ? 3u : 2u) ||
        !game_graphics_frame(frame,memory,cpu,pc) || memcmp(live,snapshot,size) || lane_find(pc)) {
        entry->next = 0;
        return -1;
    }
    const uint32_t call = in_wrapper ? GRAPHICS_LIBRARY_CALL : GRAPHICS_CALL;
    const uint32_t target = in_wrapper ? GRAPHICS_LIBRARY : GRAPHICS_ENTRY;
    if (store) {
        const uint32_t value = pc == GRAPHICS_ENTRY+4u ? cpu->r[31] : cpu->r[2];
        if (!musashi_boot_write32(memory,address,value)) {
            entry->next = 0;
            return -1;
        }
    } else if (pc == POST_EXCHANGE) cpu->r[2] = 992u;
    else if (pc == POST_EXCHANGE+4u) cpu->r[1] = 0x80070000u;
    else if (pc == GRAPHICS_ENTRY) cpu->r[29] -= 24u;
    else if (pc == GRAPHICS_LIBRARY_CALL+4u) cpu->r[4] = 0u;
    else if (pc == call) {
        cpu->r[31] = pc+8u;
        if (g_trace_state >= 0) musashi_trace_function_entry(target,pc,'j');
        if (g_entry_hook) g_entry_pending = target;
    }
    cpu->pc = cpu->npc;
    cpu->npc = pc == call ? target : cpu->pc+4u;
    cpu->delay_slot = pc == call;
    cpu->branch_pc = cpu->delay_slot ? call : 0u;
    entry->next = cpu->pc == GRAPHICS_LIBRARY ? 0u : cpu->pc;
    return 1;
}
