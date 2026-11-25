/* The loaded File 1 room owns this invocation of MAIN member 10.
 * The formatter executes the mounted guest code. These checks bind its
 * memory accesses and its four ordinary child calls to the menu frame. */
enum {
    ROOM_LATCH_RETURN = MAIN12_ROOM_RETURN + 8u,
    ROOM_PENDING_MODE = 0x80078e50u, ROOM_CURRENT_MODE = 0x800b9a08u,
    ROOM_TRANSITION = 0x800b9a0au, ROOM_PREVIOUS = 0x800b9a0cu,
    ROOM_FLAG = 0x800b9a11u
};

static int room_latch_active(const FormatterCpu *cpu)
{
    return cpu->pc - ROOM_RESET_CALLER < 112u &&
        (cpu->main12_foreground_owner || cpu->room_latch.owner) &&
        !cpu->irq_image && !cpu->cd_parent_image && !cpu->registration_stack &&
        !cpu->dma_registration && !cpu->bios_frame && !cpu->cd_irq_frame;
}

static int room_latch_mount(MusashiBootMemory *ram, const FormatterCpu *cpu,
                            MusashiMain12Owner *owner, uint32_t top)
{
    uint32_t link;
    uint64_t digest;
    return owner && owner->installed && cpu->main12_foreground_owner == owner &&
        main12_owner_for(cpu, ram) == owner &&
        owner->restore.foreground_result_written && !cpu->main12_room_next &&
        !cpu->main12_callback_frame && !cpu->movie_output && !cpu->title_mixer &&
        !cpu->merge_pending && !cpu->gte_load_pending && !cpu->r[0] &&
        g_overlay_0010_words && cpu->main12_foreground_sp == top &&
        !(top & 3u) && top >= MAIN12_BASE + MAIN12_BYTES + 48u && top <= 0x80200000u &&
        intrinsic_span(cpu, top - 48u, 48u) &&
        musashi_boot_read32(ram, top - 8u, &link) && link == MAIN12_MENU_EXIT_RETURN &&
        intrinsic_span(cpu, ROOM_RESET_CALLER, 112u) &&
        span_digest(ram, ROOM_RESET_CALLER, 28u, &digest) && digest == UINT64_C(0x36929f21ef883375) &&
        span_digest(ram, MAIN12_BASE, 128u, &digest) && digest == UINT64_C(0x9a19614adcf31a2b);
}

static void room_latch_menu_retired(MusashiBootMemory *ram, FormatterCpu *cpu,
                                    uint32_t pc, uint32_t word, uint32_t branch, int delay)
{
    RoomLatchFrame *f = &cpu->room_latch;
    const uint32_t top = cpu->main12_foreground_sp;
    if (pc == MAIN12_ROOM_RETURN && !f->owner && !delay && !branch &&
        word == MIPS_JAL(ROOM_RESET_CALLER) && cpu->pc == pc + 4u &&
        cpu->npc == ROOM_RESET_CALLER && cpu->delay_slot && cpu->branch_pc == pc &&
        cpu->r[31] == ROOM_LATCH_RETURN && cpu->r[29] == top - 24u &&
        room_latch_mount(ram, cpu, cpu->main12_foreground_owner, top)) {
        f->owner = cpu->main12_foreground_owner;
        f->top = top;
        f->next = cpu->pc;
        memcpy(f->regs, cpu->r, sizeof f->regs);
    } else if (pc == MAIN12_ROOM_RETURN + 4u && f->next == pc &&
               delay && branch == MAIN12_ROOM_RETURN && cpu->pc == ROOM_RESET_CALLER &&
               cpu->npc == cpu->pc + 4u && !cpu->branch_pc && !cpu->delay_slot &&
               !word && !memcmp(cpu->r, f->regs, sizeof f->regs) &&
               room_latch_mount(ram, cpu, f->owner, f->top)) {
        f->next = cpu->pc;
    }
}

static uint32_t room_latch_call(uint32_t pc)
{
    switch (pc - ROOM_RESET_CALLER) {
    case 64u: return 0x8001c0c8u;
    case 72u: return 0x80015310u;
    case 80u: return ROOM_RESET_ENTRY;
    case 88u: return SCENE_SETUP;
    default: return 0;
    }
}

static int room_latch_valid(MusashiBootMemory *ram, const FormatterCpu *cpu)
{
    const RoomLatchFrame *f = &cpu->room_latch;
    const uint32_t offset = cpu->pc - ROOM_RESET_CALLER;
    uint32_t link;
    if (!room_latch_active(cpu) || (offset & 3u) || cpu->pc != f->next ||
        !room_latch_mount(ram, cpu, f->owner, f->top) ||
        cpu->r[29] != f->top - (offset == 0u || offset >= 104u ? 24u : 48u))
        return 0;
    if (offset > 24u && (!musashi_boot_read32(ram, f->top - 32u, &link) ||
                          link != ROOM_LATCH_RETURN)) return 0;
    if (f->returning || memcmp(cpu->r, f->regs, sizeof f->regs)) return 0;
    if (cpu->pc == ROOM_RESET_CALL || cpu->pc == ROOM_RESET_CALL + 4u) {
        if (room_reset_images(ram, cpu) != f->owner || cpu->room_reset_slot) return 0;
        if (cpu->pc == ROOM_RESET_CALL) {
            if (cpu->room_reset_owner || cpu->room_reset_step) return 0;
        } else if (cpu->room_reset_owner != f->owner || cpu->room_reset_step != cpu->pc) return 0;
    }
    if (cpu->pc == SCENE_CALL || cpu->pc == SCENE_CALL + 4u) {
        uint32_t word;
        if (cpu->room_scene.owner != f->owner || !room_scene_word(ram, cpu, &word)) return 0;
    }
    uint32_t target = room_latch_call(cpu->pc - 4u);
    if (offset == 108u) target = ROOM_LATCH_RETURN;
    return target ? cpu->delay_slot == 1u && cpu->branch_pc == cpu->pc - 4u && cpu->npc == target
                  : !cpu->delay_slot && !cpu->branch_pc && cpu->npc == cpu->pc + 4u;
}

static void room_latch_child_returned(FormatterCpu *cpu, uint32_t from, int native_entry)
{
    RoomLatchFrame *f = &cpu->room_latch;
    if (!f->returning || cpu->pc != f->next || cpu->r[31] != f->next ||
        cpu->delay_slot || cpu->branch_pc || cpu->npc != cpu->pc + 4u ||
        cpu->main12_foreground_owner != f->owner) return;
    const uint32_t child = room_latch_call(f->next - 8u);
    if (native_entry) {
        if (from != child || (child != 0x8001c0c8u && child != 0x80015310u)) return;
    } else if (!((child == 0x8001c0c8u && from - child < 132u) ||
                 (child == 0x80015310u && from - child < 60u) ||
                 (child == ROOM_RESET_ENTRY && from == child + 32u) ||
                 (child == SCENE_SETUP && from == child + 228u))) return;
    f->returning = 0;
    memcpy(f->regs, cpu->r, sizeof f->regs);
}

static int room_latch_fetch(MusashiBootMemory *ram, const FormatterCpu *cpu, uint32_t *word)
{
    if (!room_latch_valid(ram, cpu) || !musashi_boot_read32(ram, cpu->pc, word)) return 0;
    unsigned op = *word >> 26;
    if (op < 32u) return 1;

    uint32_t address = cpu->r[(*word >> 21) & 31u] + (int32_t)(int16_t)*word;
    uint32_t value = cpu->r[(*word >> 16) & 31u];
    if (op == 35u || op == 43u)
        return address == cpu->room_latch.top - 32u && intrinsic_span(cpu, address, 4u) &&
            (op == 35u || value == ROOM_LATCH_RETURN);
    if (op == 33u || op == 37u)
        return (address == ROOM_PENDING_MODE || address == ROOM_TRANSITION) && intrinsic_span(cpu, address, 2u);
    if (op == 41u)
        return (address == ROOM_TRANSITION || address == ROOM_CURRENT_MODE || address == ROOM_PREVIOUS) &&
            intrinsic_span(cpu, address, 2u);
    return op == 40u && address == ROOM_FLAG && intrinsic_span(cpu, address, 1u);
}

/* Only the actual call delay slot can yield this frame to its child.
 * Recording the retired state also makes a vetoed device clock retryable. */
static void room_latch_retired(FormatterCpu *cpu, uint32_t issued)
{
    RoomLatchFrame *frame = &cpu->room_latch;
    if (cpu->pc == ROOM_LATCH_RETURN) {
        *frame = (RoomLatchFrame){0};
        return;
    }
    const uint32_t child = room_latch_call(issued - 4u);
    if (child && cpu->pc == child) {
        frame->returning = 1;
        frame->next = issued + 4u;
    } else {
        frame->next = cpu->pc;
    }
    memcpy(frame->regs, cpu->r, sizeof frame->regs);
}
