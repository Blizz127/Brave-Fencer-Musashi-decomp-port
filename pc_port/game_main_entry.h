/* Bind the native foreground consumers to the original US image at cold boot.
 * The game startup call itself uses the normal mounted CPU and device clock. */
enum {
    GAME_CRT_CALL = 0x80010094u, GAME_CTOR = 0x800100a0u, GAME_CTOR_END = 0x80010110u,
    GAME_MAIN_ENTRY = 0x80010178u, GAME_CD_CALL_END = 0x800101d4u
};

static int game_main_bound(LaneRunFrame *frame, MusashiBootMemory *memory, const FormatterCpu *cpu) {
    if (!frame || !memory || !cpu || !cpu->clock || !cpu->callback_device) return 0;
    const GameMainEntry *entry = &frame->main_entry;
    return !frame->prev && frame->entry_stop && frame->cpu == cpu && entry->memory == memory &&
        g_fetch_memory == memory && musashi_native_lane_usable(memory) &&
        cpu->clock == entry->clock && cpu->clock->userdata == entry->charge.userdata &&
        cpu->clock->advance == entry->charge.advance && cpu->callback_device == entry->callback &&
        cpu->callback_device->userdata == entry->worker && cpu->callback_device->checkpoint == entry->checkpoint &&
        !cpu->r[0] && !cpu->bios_frame && !cpu->cd_irq_frame && !cpu->registration_stack &&
        !cpu->dma_registration && !cpu->irq_image && !cpu->cd_parent_image &&
        !cpu->merge_pending && !cpu->gte_load_pending;
}

static int game_main_owner(LaneRunFrame *frame, MusashiBootMemory *memory, const FormatterCpu *cpu) {
    return frame == g_lane_run_frames && game_main_bound(frame, memory, cpu);
}

static void game_main_capture(LaneRunFrame *frame, MusashiBootMemory *memory, FormatterCpu *cpu) {
    static const uint8_t payload_sha256[32] = {
        0xbe, 0x19, 0x7b, 0xd5, 0x92, 0xbe, 0x13, 0x24, 0x00, 0x94, 0xbf, 0x49, 0xd0, 0x66, 0xe8, 0xf3,
        0xac, 0xd8, 0x24, 0x5f, 0xbe, 0xb9, 0xec, 0x81, 0x0e, 0x30, 0x0a, 0x37, 0xea, 0x0f, 0x44, 0xa4
    };
    const size_t size = CODE_IMAGE_END - CODE_IMAGE_BASE;
    const char *optional = getenv("MUSASHI_CODE_IMAGE");
    const char *mutant = getenv("MUSASHI_CODE_IMAGE_MUTANT");
    if (frame != g_lane_run_frames || frame->prev || !frame->entry_stop || frame->cpu != cpu ||
        cpu->pc != CODE_IMAGE_BASE || cpu->npc != cpu->pc + 4u || cpu->delay_slot || cpu->branch_pc ||
        cpu->r[31] || !musashi_native_lane_usable(memory) || !cpu->clock || !cpu->clock->advance ||
        !cpu->callback_device || !cpu->callback_device->checkpoint || (mutant && *mutant == '1') ||
        !intrinsic_span(cpu, GAME_CRT_CALL, 8u) || !intrinsic_span(cpu, GAME_MAIN_ENTRY, 32u)) return;
    for (unsigned offset = 0; offset < 32u; offset += 4u)
        if (lane_find(GAME_MAIN_ENTRY + offset)) return;
    const uint8_t *payload = musashi_boot_ram_span(memory, CODE_IMAGE_BASE, size);
    const uint8_t *cached = code_image();
    uint8_t digest[32];
    if (!payload || !musashi_sha256_digest(payload, size, digest) || memcmp(digest, payload_sha256, sizeof digest) ||
        (optional && *optional && !cached) || (cached && memcmp(cached + CODE_IMAGE_TEXT, payload, size))) return;
    frame->main_entry = (GameMainEntry) {
        .memory = memory, .clock = cpu->clock, .charge = *cpu->clock,
        .callback = cpu->callback_device, .checkpoint = cpu->callback_device->checkpoint,
        .worker = cpu->callback_device->userdata
    };
}

static int startup_constructor_or_graphics(uint32_t pc) {
    return pc - GAME_CTOR < GAME_CTOR_END - GAME_CTOR || pc - STARTUP_ENTRY < STARTUP_END - STARTUP_ENTRY;
}

static int startup_clocked_pc(uint32_t pc) {
    return pc - GAME_CRT_CALL < 8u || pc - GAME_MAIN_ENTRY < GAME_CD_CALL_END - GAME_MAIN_ENTRY ||
        startup_constructor_or_graphics(pc) || startup_exchange_pc(pc);
}
