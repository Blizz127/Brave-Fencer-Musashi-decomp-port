/* MAIN/0010's room-mode latch, compiled from the published owner C.
 * Its native steps run in the main foreground CPU. Each child keeps its own
 * entry, clock and return path, including the MAIN/0012 slot and scene frames. */
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

/* Only the mounted menu's actual JAL can authorize its delay slot. The
 * continuation must survive both retirements before entering MAIN/0010. */
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

/* Child execution owns its volatile registers. Resume only when its own
 * checked return has retired. A copied PC, SP and RA alone cannot resume. */
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
    return room_latch_valid(ram, cpu) && musashi_boot_read32(ram, cpu->pc, word);
}

static int room_latch_issue(MusashiBootMemory *ram, FormatterCpu *cpu, uint32_t *flow)
{
    RoomLatchFrame *f = &cpu->room_latch;
    const uint32_t pc = cpu->pc, offset = pc - ROOM_RESET_CALLER;
    uint32_t next = cpu->npc, after = next + 4u, target = room_latch_call(pc);
    uint16_t half;
    const int scene_call = pc == SCENE_CALL || pc == SCENE_CALL + 4u;
    if (!room_latch_valid(ram, cpu)) return 0;
    if (cpu->clock) {
        const MusashiExecutionClock *clock = cpu->clock;
        MusashiMain12Owner *owner = f->owner;
        const uint32_t hi = cpu->hi, lo = cpu->lo;
        uint32_t regs[32];
        const int memory_op = offset == 8u || offset == 16u || offset == 24u ||
            offset == 32u || offset == 40u || offset == 52u || offset == 60u || offset == 96u;
        memcpy(regs, cpu->r, sizeof regs);
        if (!clock->advance || !clock->advance(clock->userdata, memory_op ? 3u : 2u) ||
            cpu->clock != clock || cpu->pc != pc || cpu->npc != next || f->owner != owner ||
            cpu->hi != hi || cpu->lo != lo || memcmp(regs, cpu->r, sizeof regs) ||
            !room_latch_valid(ram, cpu)) return 0;
    }
    switch (offset) {
    case 0:  cpu->r[29] -= 24u; break;
    case 4:  cpu->r[2] = 0x80080000u; break;
    case 8:
        if (!intrinsic_span(cpu, ROOM_PENDING_MODE, 2u) || !cpu_read16(ram, cpu, ROOM_PENDING_MODE, &half)) return 0;
        cpu->r[2] = half; break;
    case 12: cpu->r[4] = 0x800c0000u; break;
    case 16:
        if (!intrinsic_span(cpu, ROOM_TRANSITION, 2u) || !cpu_read16(ram, cpu, ROOM_TRANSITION, &half)) return 0;
        cpu->r[4] = half; break;
    case 20: cpu->r[3] = 1u; break;
    case 24: if (!cpu_write32(ram, cpu, cpu->r[29] + 16u, cpu->r[31])) return 0; break;
    case 28: case 36: case 48: case 56: cpu->r[1] = 0x800c0000u; break;
    case 32: if (!intrinsic_span(cpu, ROOM_FLAG, 1u) || !cpu_write8(ram, cpu, ROOM_FLAG, (uint8_t)cpu->r[3])) return 0; break;
    case 40: if (!intrinsic_span(cpu, ROOM_CURRENT_MODE, 2u) || !cpu_write16(ram, cpu, ROOM_CURRENT_MODE, (uint16_t)cpu->r[2])) return 0; break;
    case 44: cpu->r[2] &= 0xf000u; break;
    case 52: if (!intrinsic_span(cpu, ROOM_PREVIOUS, 2u) || !cpu_write16(ram, cpu, ROOM_PREVIOUS, (uint16_t)cpu->r[4])) return 0; break;
    case 60: if (!intrinsic_span(cpu, ROOM_TRANSITION, 2u) || !cpu_write16(ram, cpu, ROOM_TRANSITION, (uint16_t)cpu->r[2])) return 0; break;
    case 64: case 72: case 80: case 88:
        cpu->r[31] = pc + 8u;
        after = target;
        break;
    case 68: case 76: case 84: case 92: case 108: break;
    case 96:
        if (!cpu_read32(ram, cpu, cpu->r[29] + 16u, &cpu->r[31])) return 0;
        break;
    case 100: cpu->r[29] += 24u; break;
    case 104: target = after = cpu->r[31]; break;
    default: return 0;
    }
    cpu->branch_pc = target ? pc : 0u;
    cpu->delay_slot = (uint8_t)(target != 0);
    cpu->pc = next;
    cpu->npc = after;
    *flow = target;
    if (pc == ROOM_RESET_CALL || pc == ROOM_RESET_CALL + 4u) {
        cpu->room_reset_owner = f->owner;
        cpu->room_reset_step = next;
        cpu->room_reset_slot = 0;
    }
    if (scene_call) room_scene_position(cpu);
    if (offset == 108u) {
        memset(f, 0, sizeof *f);
    } else {
        f->returning = room_latch_call(pc - 4u) != 0;
        f->next = f->returning ? pc + 4u : next;
        memcpy(f->regs, cpu->r, sizeof f->regs);
    }
    return 1;
}
