/* MAIN/0010 calls the MAIN/0012 slot reset after latching the room.
 * Other scenes mount the same nine instructions at the same address. */
enum {
    ROOM_RESET_CALLER = 0x800d1d24u, ROOM_RESET_CALL = 0x800d1d74u,
    ROOM_RESET_ENTRY = 0x80129258u, ROOM_RESET_SLOTS = 0x8011db30u,
    ROOM_RESET_COUNT = 128, ROOM_RESET_STRIDE = 56
};

static MusashiMain12Owner *room_reset_images_at(MusashiBootMemory *ram, const FormatterCpu *cpu, uint32_t sp)
{
    uint64_t digest;
    MusashiMain12Owner *owner = main12_owner_for(cpu, ram);
    if (!owner || !g_overlay_0010_words || cpu->main12_callback_frame ||
        cpu->movie_output || cpu->title_mixer ||
        !intrinsic_span(cpu, ROOM_RESET_CALLER, 112u) ||
        !intrinsic_span(cpu, ROOM_RESET_SLOTS, ROOM_RESET_COUNT * ROOM_RESET_STRIDE))
        return NULL;
    /* Continue reaches MAIN/0010 from the foreground menu's second JAL.
     * Its frame stays live throughout this nested slot reset. Both original
     * 24-byte stack frames and their saved return addresses must agree. */
    if (cpu->main12_foreground_owner) {
        uint32_t top = cpu->main12_foreground_sp, menu_return, room_return;
        if (cpu->main12_foreground_owner != owner || !owner->restore.foreground_result_written ||
            cpu->main12_room_next || top < 48u || sp != top - 48u ||
            !musashi_boot_read32(ram, top - 32u, &room_return) || room_return != 0x8017bfe8u ||
            !musashi_boot_read32(ram, top - 8u, &menu_return) || menu_return != MAIN12_MENU_EXIT_RETURN)
            return NULL;
    }
    /* The complete caller and the 512-byte entry prefix independently identify
     * their two mounted members. Check the separately located callee as well. */
    if (!span_digest(ram, ROOM_RESET_CALLER, 28u, &digest) || digest != UINT64_C(0x36929f21ef883375) ||
        !span_digest(ram, MAIN12_BASE, 128u, &digest) || digest != UINT64_C(0x9a19614adcf31a2b) ||
        !span_digest(ram, ROOM_RESET_ENTRY, 9u, &digest) || digest != UINT64_C(0xb9cb72d7e1fd5c28))
        return NULL;
    return owner;
}

static MusashiMain12Owner *room_reset_images(MusashiBootMemory *ram, const FormatterCpu *cpu)
{
    return room_reset_images_at(ram, cpu, cpu->r[29]);
}

static int room_reset_active(const FormatterCpu *cpu)
{
    return cpu->room_reset_owner && cpu->pc - ROOM_RESET_ENTRY < 36u;
}

static int room_reset_valid(MusashiBootMemory *ram, const FormatterCpu *cpu)
{
    const unsigned offset = cpu->pc - ROOM_RESET_ENTRY;
    const unsigned slot = cpu->room_reset_slot;
    uint32_t branch = 0, npc = cpu->pc + 4u;
    if (!cpu->room_reset_owner || cpu->room_reset_step != cpu->pc ||
        cpu->r[31] != ROOM_RESET_CALL + 8u || cpu->r[0] || cpu->merge_pending ||
        cpu->gte_load_pending || slot > ROOM_RESET_COUNT || offset >= 36u || (offset & 3u) ||
        room_reset_images(ram, cpu) != cpu->room_reset_owner)
        return 0;
    if (offset < 12u) {
        if (slot || (offset == 4u && cpu->r[3] != 0x80120000u) ||
            (offset == 8u && cpu->r[3] != ROOM_RESET_SLOTS)) return 0;
    } else {
        if (cpu->r[3] != ROOM_RESET_SLOTS + slot * ROOM_RESET_STRIDE) return 0;
        if (offset <= 24u) {
            if (slot >= ROOM_RESET_COUNT ||
                cpu->r[2] != ROOM_RESET_COUNT - 1u - slot - (unsigned)(offset >= 20u))
                return 0;
        } else if (slot != ROOM_RESET_COUNT || cpu->r[2] != UINT32_MAX) return 0;
    }
    if (offset == 24u) {
        branch = cpu->pc - 4u;
        if (slot + 1u < ROOM_RESET_COUNT) npc = ROOM_RESET_ENTRY + 12u;
    } else if (offset == 32u) {
        branch = cpu->pc - 4u;
        npc = cpu->r[31];
    }
    return cpu->npc == npc && cpu->branch_pc == branch && cpu->delay_slot == (branch != 0);
}

/* Retire a single source operation per call to keep the normal scheduler,
 * interruption points and the halfword bus writes visible to the game. */
static int room_reset_issue(MusashiBootMemory *ram, FormatterCpu *cpu)
{
    const uint32_t pc = cpu->pc;
    const unsigned offset = pc - ROOM_RESET_ENTRY;
    if (!room_reset_valid(ram, cpu)) return 0;
    if (cpu->clock) {
        const MusashiExecutionClock *clock = cpu->clock;
        MusashiMain12Owner *owner = cpu->room_reset_owner;
        const unsigned slot = cpu->room_reset_slot;
        uint32_t registers[32];
        memcpy(registers, cpu->r, sizeof registers);
        if (!clock->advance || !clock->advance(clock->userdata, offset == 12u ? 3u : 2u) ||
            cpu->pc != pc || cpu->clock != clock || cpu->room_reset_owner != owner ||
            cpu->room_reset_slot != slot || memcmp(registers, cpu->r, sizeof registers) ||
            !room_reset_valid(ram, cpu)) return 0;
    }
    uint32_t next = cpu->npc, after = next + 4u;
    const int branch = offset == 20u || offset == 28u;
    switch (offset) {
    case 0: cpu->r[3] = 0x80120000u; break;
    case 4: cpu->r[3] -= 0x24d0u; break;
    case 8: cpu->r[2] = ROOM_RESET_COUNT - 1u; break;
    case 12: if (!cpu_write16(ram, cpu, cpu->r[3], 0)) return 0; break;
    case 16: --cpu->r[2]; break;
    case 20: if (!(cpu->r[2] & 0x80000000u)) after = ROOM_RESET_ENTRY + 12u; break;
    case 24: cpu->r[3] += ROOM_RESET_STRIDE; ++cpu->room_reset_slot; break;
    case 28: after = cpu->r[31]; break;
    case 32: break; /* Return delay slot. */
    }
    cpu->branch_pc = branch ? pc : 0u;
    cpu->delay_slot = (uint8_t)branch;
    cpu->pc = next;
    cpu->npc = after;
    cpu->room_reset_step = next;
    if (offset == 32u) {
        cpu->room_reset_owner = NULL;
        cpu->room_reset_step = 0;
        cpu->room_reset_slot = 0;
    }
    return 1;
}
