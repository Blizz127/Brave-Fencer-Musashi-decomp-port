/* The File 1 room transition calls this scene constructor immediately after
 * returning from the 128-slot reset. Keep the six nested clears interruptible. */
enum {
    SCENE_CALL = ROOM_RESET_CALL + 8u, SCENE_RETURN = SCENE_CALL + 8u,
    SCENE_SETUP = 0x801378f0u, SCENE_CHILD = 0x8013a860u, SCENE_CLEAR = 0x80016714u,
    SCENE_WORK = 0x80127520u, SCENE_ROWS = 0x801269f0u, SCENE_ROW_BYTES = 76u,
    SCENE_VALUES = 0x8017e934u, SCENE_CHILD_ROWS = 0x8011da80u
};

static int room_scene_overlay_pc(uint32_t pc)
{
    return pc - SCENE_SETUP < 232u || pc - SCENE_CHILD < 80u;
}

static int room_scene_active(const FormatterCpu *cpu)
{
    return cpu->room_scene.owner && !cpu->irq_image && !cpu->cd_parent_image &&
           !cpu->registration_stack && !cpu->dma_registration &&
           !cpu->cd_irq_frame && !cpu->bios_frame;
}

static void room_scene_position(FormatterCpu *cpu)
{
    RoomSceneFrame *frame = &cpu->room_scene;
    frame->pc = cpu->pc;
    frame->npc = cpu->npc;
    frame->branch = cpu->branch_pc;
    memcpy(frame->regs, cpu->r, sizeof frame->regs);
}

static void room_scene_begin(FormatterCpu *cpu, MusashiMain12Owner *owner)
{
    RoomSceneFrame *frame = &cpu->room_scene;
    frame->owner = owner;
    frame->sp = cpu->r[29];
    frame->clears = 0;
    memcpy(frame->saved_outer, cpu->r + 16, sizeof frame->saved_outer);
    room_scene_position(cpu);
}

static void room_scene_clear_args(unsigned index, uint32_t *base, uint32_t *size, uint32_t *ret)
{
    if (index == 0u) {
        *base = SCENE_WORK; *size = 0xbfcu; *ret = 0x80137918u;
    } else if (index < 4u) {
        *base = SCENE_ROWS + SCENE_ROW_BYTES * (index - 1u);
        *size = SCENE_ROW_BYTES; *ret = 0x80137938u;
    } else {
        *base = SCENE_CHILD_ROWS + 16u * (index - 4u);
        *size = 16u; *ret = 0x8013a888u;
    }
}

/* The two guest frames occupy the 72 bytes below the outer SP. Verify the
 * saved value on each load, including both real returns, after any IRQ. */
static int room_scene_stack(MusashiBootMemory *ram, const FormatterCpu *cpu, uint32_t word)
{
    const RoomSceneFrame *f = &cpu->room_scene;
    unsigned reg = (word >> 16) & 31u;
    int child = cpu->pc - SCENE_CHILD < 80u;
    uint32_t offset = (uint16_t)word;
    uint32_t value, expected;
    if (cpu->r[29] != f->sp - (child ? 72u : 40u)) return 0;
    if (reg == 31u) expected = child ? 0x801379b8u : SCENE_RETURN;
    else if (reg >= 16u && reg < (child ? 18u : 20u))
        expected = child ? f->saved_child[reg - 16u] : f->saved_outer[reg - 16u];
    else return 0;
    if ((word >> 26) == 43u) return cpu->r[reg] == expected;
    return musashi_boot_read32(ram, cpu->r[29] + offset, &value) && value == expected;
}

static int room_scene_access(MusashiBootMemory *ram, const FormatterCpu *cpu, uint32_t word)
{
    const RoomSceneFrame *f = &cpu->room_scene;
    unsigned op = word >> 26;
    if (cpu->pc - SCENE_CLEAR < 164u) {
        uint32_t base, size, ret;
        if (f->clears >= 6u) return 0;
        room_scene_clear_args(f->clears, &base, &size, &ret);
        if (cpu->r[31] != ret || !intrinsic_span(cpu, base, size)) return 0;
        if (cpu->pc == SCENE_CLEAR) return cpu->r[4] == base && cpu->r[5] == size;
        if (op == 43u) return !(cpu->r[4] & 3u) && cpu->r[4] - base < size;
        if (cpu->pc == 0x800167b0u) return cpu->r[4] == base + size;
        return 1;
    }
    if (op == 35u || op == 43u) return room_scene_stack(ram, cpu, word);
    if (op == 37u || op == 41u) {
        /* Each table read and corresponding write uses the same row. */
        unsigned row = f->clears - 2u;
        uint32_t address = cpu->r[1] + (uint32_t)(int32_t)(int16_t)word;
        uint32_t first = op == 37u ? SCENE_VALUES + row * 8u :
                                     SCENE_ROWS + row * SCENE_ROW_BYTES + 56u;
        return row < 3u && !(address & 1u) && address - first < 8u &&
               intrinsic_span(cpu, address, 2u);
    }
    if (cpu->pc == 0x801379d0u) return f->clears == 6u && cpu->r[31] == SCENE_RETURN;
    if (cpu->pc == 0x8013a8a8u) return f->clears == 6u && cpu->r[31] == 0x801379b8u;
    return 1;
}

static int room_scene_word(MusashiBootMemory *ram, const FormatterCpu *cpu, uint32_t *word)
{
    const RoomSceneFrame *f = &cpu->room_scene;
    uint64_t digest;
    if (!room_scene_active(cpu) || cpu->main12_foreground_owner != f->owner ||
        cpu->r[0] || cpu->merge_pending || cpu->gte_load_pending ||
        cpu->pc != f->pc || cpu->npc != f->npc || cpu->branch_pc != f->branch ||
        cpu->delay_slot != (f->branch != 0u) || memcmp(f->regs, cpu->r, sizeof f->regs) ||
        (f->sp & 3u) || f->sp < MAIN12_BASE + MAIN12_BYTES + 72u ||
        f->sp > 0x80200000u - 48u || !intrinsic_span(cpu, f->sp - 72u, 120u) ||
        room_reset_images_at(ram, cpu, f->sp) != f->owner ||
        !span_digest(ram, SCENE_SETUP, 58u, &digest) || digest != UINT64_C(0xfd339e40dbf96f9d) ||
        !span_digest(ram, SCENE_CHILD, 20u, &digest) || digest != UINT64_C(0xf7aa82fbce89e827) ||
        !intrinsic_span(cpu, SCENE_CLEAR, 164u) || !f->owner->resident_matches ||
        !f->owner->resident_matches(f->owner, SCENE_CLEAR, 164u) ||
        !span_digest(ram, SCENE_CLEAR, 41u, &digest) || digest != UINT64_C(0x633c959e4192f3fd))
        return 0;
    if (cpu->pc != SCENE_CALL && cpu->pc != SCENE_CALL + 4u &&
        !room_scene_overlay_pc(cpu->pc) && cpu->pc - SCENE_CLEAR >= 164u) return 0;
    uint32_t mounted;
    if (!musashi_boot_read32(ram, cpu->pc, &mounted) || !room_scene_access(ram, cpu, mounted)) return 0;
    *word = mounted;
    return 1;
}

/* The guest CPU has already issued the instruction. Preserve the actual
 * child entry registers for stack validation and release the scene owner
 * only when its outer call returns to MAIN/0010. */
static void room_scene_retired(FormatterCpu *cpu, int clearing)
{
    RoomSceneFrame *frame = &cpu->room_scene;
    if (clearing && cpu->pc == cpu->r[31]) ++frame->clears;
    if (cpu->pc == SCENE_CHILD)
        memcpy(frame->saved_child, cpu->r + 16, sizeof frame->saved_child);
    if (cpu->pc == SCENE_RETURN)
        frame->owner = NULL;
    else
        room_scene_position(cpu);
}
