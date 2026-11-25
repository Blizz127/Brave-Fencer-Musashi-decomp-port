/* The same overlay addresses can run different code between rooms. Bind this
 * guest call to the mounted pair of images and its original caller. */
enum {
    ROOM_RESET_CALLER = 0x800d1d24u,
    ROOM_RESET_CALL = 0x800d1d74u,
    ROOM_RESET_ENTRY = 0x80129258u,
    ROOM_RESET_BYTES = 36u,
    ROOM_SLOT_BASE = 0x8011db30u,
    ROOM_SLOT_COUNT = 128u,
    ROOM_SLOT_SIZE = 56u
};

static int room_latch_mount(MusashiBootMemory *, const FormatterCpu *,
                            MusashiMain12Owner *, uint32_t);

static MusashiMain12Owner *room_reset_images_at(MusashiBootMemory *ram,
                                                 const FormatterCpu *cpu,
                                                 uint32_t sp)
{
    MusashiMain12Owner *owner = main12_owner_for(cpu, ram);
    uint64_t digest;
    if (!owner || !owner->installed || !g_overlay_0010_words || cpu->main12_callback_frame ||
        cpu->movie_output || cpu->title_mixer || cpu->merge_pending || cpu->gte_load_pending ||
        cpu->r[0] || !intrinsic_span(cpu, ROOM_SLOT_BASE, ROOM_SLOT_COUNT * ROOM_SLOT_SIZE) ||
        !musashi_boot_ram_span(ram, ROOM_SLOT_BASE, ROOM_SLOT_COUNT * ROOM_SLOT_SIZE) ||
        !intrinsic_span(cpu, ROOM_RESET_ENTRY, ROOM_RESET_BYTES) ||
        !span_digest(ram, ROOM_RESET_ENTRY, ROOM_RESET_BYTES / 4u, &digest) ||
        digest != UINT64_C(0xb9cb72d7e1fd5c28))
        return NULL;

    if (cpu->main12_foreground_owner) {
        uint32_t top = cpu->main12_foreground_sp, saved;
        if (sp != top - 48u || !room_latch_mount(ram, cpu, owner, top) ||
            !musashi_boot_read32(ram, top - 32u, &saved) || saved != 0x8017bfe8u)
            return NULL;
    } else if (!intrinsic_span(cpu, ROOM_RESET_CALLER, 112u) ||
               !span_digest(ram, ROOM_RESET_CALLER, 28u, &digest) ||
               digest != UINT64_C(0x36929f21ef883375) ||
               !span_digest(ram, MAIN12_BASE, 128u, &digest) ||
               digest != UINT64_C(0x9a19614adcf31a2b)) {
        return NULL;
    }
    return owner;
}

static MusashiMain12Owner *room_reset_images(MusashiBootMemory *ram, const FormatterCpu *cpu)
{
    return room_reset_images_at(ram, cpu, cpu->r[29]);
}

static int room_reset_active(const FormatterCpu *cpu)
{
    return cpu->room_reset_owner && cpu->room_reset_step - ROOM_RESET_ENTRY < ROOM_RESET_BYTES &&
        !cpu->irq_image && !cpu->cd_parent_image && !cpu->cd_irq_frame &&
        !cpu->registration_stack && !cpu->dma_registration && !cpu->bios_frame;
}

/* Resume only the CPU state produced by the last successful guest issue.
 * Interrupts can advance devices without retiring the pending instruction. */
static void room_reset_position(FormatterCpu *cpu)
{
    cpu->room_reset_step = cpu->pc;
    cpu->room_reset_npc = cpu->npc;
    cpu->room_reset_branch = cpu->branch_pc;
    memcpy(cpu->room_reset_regs, cpu->r, sizeof cpu->room_reset_regs);
}

static int room_reset_word(MusashiBootMemory *ram, const FormatterCpu *cpu, uint32_t *word)
{
    if (!room_reset_active(cpu) || cpu->pc - ROOM_RESET_ENTRY >= ROOM_RESET_BYTES ||
        cpu->pc != cpu->room_reset_step || cpu->npc != cpu->room_reset_npc ||
        cpu->branch_pc != cpu->room_reset_branch || cpu->delay_slot != (cpu->branch_pc != 0) ||
        memcmp(cpu->room_reset_regs, cpu->r, sizeof cpu->room_reset_regs) ||
        cpu->r[31] != ROOM_RESET_CALL + 8u ||
        room_reset_images(ram, cpu) != cpu->room_reset_owner ||
        !musashi_boot_read32(ram, cpu->pc, word))
        return 0;
    /* The slot array owns only its 16-bit type fields. Decode the guest
     * effective address so the CPU cannot write the neighboring payload. */
    if (*word >> 26 >= 32u) {
        uint32_t address = cpu->r[*word >> 21 & 31u] + (uint32_t)(int32_t)(int16_t)*word;
        uint32_t offset = address - ROOM_SLOT_BASE;
        return *word >> 26 == 41u && (uint16_t)cpu->r[*word >> 16 & 31u] == 0u &&
            offset < ROOM_SLOT_COUNT * ROOM_SLOT_SIZE && offset % ROOM_SLOT_SIZE == 0u;
    }
    return 1;
}
