/* The loaded file returns through MAIN/0012's foreground dispatcher. Its
 * room table selector and state initializer are called in the same frame. */
enum {
    MAIN12_ROOM_SELECT = 0x80145934u, MAIN12_ROOM_SELECT_END = 0x80145a2cu,
    MAIN12_ROOM_INIT = 0x80145b24u, MAIN12_ROOM_INIT_END = 0x80145bf8u,
    MAIN12_ROOM_RETURN = 0x8017bfe0u, MAIN12_INIT_RETURN = 0x80145a1cu
};

static int main12_room_address(uint32_t pc)
{
    return (pc >= MAIN12_ROOM_SELECT && pc < MAIN12_ROOM_SELECT_END) ||
           (pc >= MAIN12_ROOM_INIT && pc < MAIN12_ROOM_INIT_END);
}

static int main12_room_word(MusashiBootMemory *ram, const FormatterCpu *cpu, uint32_t word)
{
    const uint32_t pc = cpu->pc, top = cpu->main12_foreground_sp;
    const int init = pc >= MAIN12_ROOM_INIT;
    const uint32_t start = init ? MAIN12_ROOM_INIT : MAIN12_ROOM_SELECT;
    const uint32_t returning = init ? MAIN12_INIT_RETURN : MAIN12_ROOM_RETURN;
    const uint32_t jump = init ? 0x80145bf0u : 0x80145a24u;
    const uint32_t caller_sp = top - (init ? 48u : 24u);
    const uint32_t frame = caller_sp - (init ? 32u : 24u);
    uint32_t saved;
    if (!cpu->main12_foreground_owner->restore.foreground_result_written ||
        pc != cpu->main12_room_next || !musashi_boot_read32(ram, top - 8u, &saved) ||
        saved != MAIN12_MENU_EXIT_RETURN ||
        cpu->r[29] != (pc == start || pc >= jump ? caller_sp : frame))
        return 0;
    if (init && (!musashi_boot_read32(ram, top - 32u, &saved) || saved != MAIN12_ROOM_RETURN))
        return 0;
    if (cpu->delay_slot) {
        uint32_t branch;
        if (cpu->branch_pc != pc - 4u || !musashi_boot_read32(ram, cpu->branch_pc, &branch))
            return 0;
        if (branch >> 26 == 3u && (cpu->r[31] != pc + 4u ||
            cpu->npc != ((pc & 0xf0000000u) | ((branch & 0x03ffffffu) << 2)))) return 0;
    }
    if (cpu->main12_room_resuming && (cpu->r[31] != pc || cpu->delay_slot || cpu->npc != pc + 4u))
        return 0;
    if (pc == start && (cpu->r[31] != returning || cpu->delay_slot || cpu->npc != pc + 4u))
        return 0;
    if (pc >= jump && (cpu->r[31] != returning ||
        (pc == jump + 4u && (!cpu->delay_slot || cpu->branch_pc != jump || cpu->npc != returning))))
        return 0;
    /* The in-game counter selects a complete 36-byte entry in the mounted
     * table. The caller has computed its actual argument before this JAL. */
    if (pc == 0x80145a0cu || pc == 0x80145a10u) {
        const uint32_t offset = cpu->r[4] - 0x801807bcu;
        if (offset > 7u * 36u || offset % 36u) return 0;
    }
    unsigned op = word >> 26;
    uint32_t address = cpu->r[(word >> 21) & 31u] + (uint32_t)(int32_t)(int16_t)word;
    uint32_t value = cpu->r[(word >> 16) & 31u];
    const uint32_t link_slot = caller_sp - 8u;
    if (pc == 0x80145bdcu) {
        /* S1 holds D_800AF630. The mounted SH uses AT one page higher
         * because the member's low half is a signed displacement. */
        return !cpu->delay_slot && cpu->npc == pc + 4u &&
               op == 41u && ((word >> 21) & 31u) == 1u && ((word >> 16) & 31u) == 2u &&
               cpu->r[17] == 0x800af630u && cpu->r[1] == cpu->r[17] + 0x10000u &&
               address == cpu->r[17] + 0xa3dau && value == 0x3000u &&
               musashi_boot_read32(ram, link_slot, &saved) && saved == MAIN12_INIT_RETURN;
    }
    if (op < 32u) return 1;
    if (pc == start + 4u) return op == 43u && address == link_slot && value == returning;
    if (pc == (init ? 0x80145be0u : 0x80145a1cu))
        return op == 35u && address == link_slot && musashi_boot_read32(ram, address, &saved) && saved == returning;
    if (!init) return 0;
    switch (pc) {
    case 0x80145b2cu: return op == 43u && address == frame + 20u && value == cpu->main12_room_s1;
    case 0x80145b34u: return op == 43u && address == frame + 16u && value == cpu->main12_room_s0;
    case 0x80145be4u: return op == 35u && address == frame + 20u &&
                             musashi_boot_read32(ram, address, &saved) && saved == cpu->main12_room_s1;
    case 0x80145be8u: return op == 35u && address == frame + 16u &&
                             musashi_boot_read32(ram, address, &saved) && saved == cpu->main12_room_s0;
    case 0x80145b9cu: return op == 36u && address == 0x80078ec0u;
    case 0x80145bb4u: {
        const uint8_t *setting = musashi_boot_ram_span(ram, address, 1u);
        return op == 40u && address == 0x80078ec0u && setting && (*setting & 0x7fu) && value == 1u;
    }
    default: return 0;
    }
}

static void main12_room_retired(FormatterCpu *cpu, uint32_t issued, uint32_t next)
{
    if (issued == MAIN12_ROOM_RETURN - 4u && next == MAIN12_ROOM_SELECT) {
        cpu->main12_room_next = next;
        cpu->main12_room_resuming = 0;
        return;
    }
    if (!cpu->main12_room_next || !main12_room_address(issued)) return;
    if (issued == 0x80145a28u && next == MAIN12_ROOM_RETURN) {
        cpu->main12_room_next = 0;
        cpu->main12_room_resuming = 0;
        return;
    }
    if (issued == MAIN12_INIT_RETURN - 4u && next == MAIN12_ROOM_INIT) {
        cpu->main12_room_s0 = cpu->r[16];
        cpu->main12_room_s1 = cpu->r[17];
    }
    /* A call outside these two mounted routines keeps its normal admission.
     * The next room instruction must be that call's own return address. */
    cpu->main12_room_resuming = !main12_room_address(next);
    cpu->main12_room_next = cpu->main12_room_resuming ? issued + 4u : next;
}
