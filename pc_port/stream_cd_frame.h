/* Included only by the parent-backed CD owner. The snapshots detect changes
 * to authenticated code, the protected CD/caller context, or the dispatch
 * decision. A change hands back at the first unissued operation. It need not
 * be a scalar fault (it may affect an untaken path). */
typedef struct StreamCdOwner {
    const MusashiGuestIrqImage *image;
    FormatterCpu *cpu;
    MusashiBootMemory *memory;
    const uint8_t *code, *context, *pointers, *targets;
    uint8_t saved_context[8], saved_pointers[0x48], saved_targets[20];
    uint8_t verified_code[MUSASHI_STREAM_CODE_BYTES];
    uint32_t verified_targets[MUSASHI_STREAM_STATE_COUNT];
    MusashiStreamClockOwner verifier;
    void *machine;
    int entered;
    MusashiExecutionClock clock;
    MusashiEntryHook hook;
} StreamCdOwner;

static int stream_cd_current(void *userdata, uint32_t phase) {
    const StreamCdOwner *s = userdata;
    return s->cpu->cd_parent_image == s->image && s->image->identity == s->image &&
        s->image->owns_stream_clock == s->verifier && s->image->stream_machine == s->machine &&
        (!s->entered || !s->hook || g_entry_pending - MUSASHI_STREAM_ENTRY >= MUSASHI_STREAM_CODE_BYTES) &&
        s->verifier(s->machine, s->memory, s->cpu->clock, s->cpu->cd_register_device,
                    s->cpu->cd_irq_frame, s->entered ? NULL : s->hook) &&
        s->image->memory == s->memory && s->cpu->clock->advance == s->clock.advance &&
        s->cpu->clock->userdata == s->clock.userdata && g_entry_hook == s->hook &&
        !memcmp(s->context, s->saved_context, sizeof(s->saved_context)) &&
        !memcmp(s->pointers, s->saved_pointers, sizeof(s->saved_pointers)) &&
        !memcmp(s->targets, s->saved_targets, sizeof(s->saved_targets)) &&
        cd_irq_ram_word(s->memory, 0x800c7d24u, phase) &&
        !memcmp(s->code, s->verified_code, sizeof(s->verified_code));
}

static void stream_cd_entered(void *userdata) {
    StreamCdOwner *s = userdata;
    s->entered = 1;
    if (g_entry_hook && g_entry_pending == MUSASHI_STREAM_ENTRY) {
        g_entry_pending = 1u;
        g_entry_hook(g_entry_hook_user, MUSASHI_STREAM_ENTRY, s->cpu->r);
    }
}

static MusashiStreamCallResult stream_cd_try(MusashiBootMemory *memory, FormatterCpu *cpu,
    unsigned *budget, const CdIrqReturn *returns, unsigned depth, uint32_t context,
    uint32_t saved_sp, uint32_t *bundled, MusashiStreamWork *work) {
    const MusashiGuestIrqImage *image = cpu->cd_parent_image;
    uint32_t phase, cursor;
    if (!image || !image->stream_witness_ready || !image->owns_stream_clock || !image->stream_machine ||
        image->stream_code_offset > sizeof(image->cd_code) - MUSASHI_STREAM_CODE_BYTES ||
        cpu->r[0] || cpu->delay_slot || cpu->branch_pc || cpu->merge_pending || cpu->gte_load_pending ||
        cpu->npc != MUSASHI_STREAM_ENTRY + 4u || cpu->r[4] != 0x80079a70u ||
        depth != 2 || returns[0].target != 0x8001a338u || returns[0].pc != 0x80045308u ||
        returns[0].sp != saved_sp - 0x50u || returns[1].target != MUSASHI_STREAM_ENTRY ||
        returns[1].pc != 0x8001a914u || cpu->r[31] != returns[1].pc ||
        returns[1].sp != saved_sp - 0x88u || cpu->r[29] != returns[1].sp ||
        (g_entry_hook && g_entry_pending != 1u && g_entry_pending != MUSASHI_STREAM_ENTRY) ||
        !cd_irq_ram_word(memory, 0x800ae6f8u, 5) || !cd_irq_pointers(memory) ||
        !cd_irq_data_pointers(memory) || !musashi_boot_read32(memory, 0x800c7d24u, &phase) ||
        !musashi_boot_read32(memory, 0x800747b0u, &cursor) || (phase >= 2 && phase <= 4 && cursor >= 1024) ||
        !image->owns_stream_clock(image->stream_machine, memory, cpu->clock,
            cpu->cd_register_device, cpu->cd_irq_frame, g_entry_hook)) return MUSASHI_STREAM_DEFER;
    /* A CD worker entered while the foreground is in the constructor or
     * graphics setup keeps the ordinary instruction clock for this call. */
    if (g_lane_run_frames && (g_lane_run_frames->field_scene.phase != FIELD_IDLE ||
        (g_lane_run_frames->cpu && startup_constructor_or_graphics(g_lane_run_frames->cpu->pc))))
        return MUSASHI_STREAM_DEFER;
    for (unsigned i = 0; i < MUSASHI_STREAM_STATE_COUNT; ++i)
        if (!cd_irq_ram_word(memory, 0x80072a38u + i * 4u, image->stream_targets[i])) return MUSASHI_STREAM_DEFER;

    StreamCdOwner owner = {.image = image, .cpu = cpu, .memory = memory,
        .code = musashi_boot_ram_span(memory, MUSASHI_STREAM_ENTRY, MUSASHI_STREAM_CODE_BYTES),
        .context = musashi_boot_ram_span(memory, context, 8),
        .pointers = musashi_boot_ram_span(memory, 0x8006cf4cu, 0x48),
        .targets = musashi_boot_ram_span(memory, 0x80072a38u, 20),
        .verifier = image->owns_stream_clock, .machine = image->stream_machine,
        .clock = *cpu->clock, .hook = g_entry_hook};
    if (!owner.code || !owner.context || !owner.pointers || !owner.targets) return MUSASHI_STREAM_DEFER;
    memcpy(owner.saved_context, owner.context, sizeof(owner.saved_context));
    memcpy(owner.saved_pointers, owner.pointers, sizeof(owner.saved_pointers));
    memcpy(owner.saved_targets, owner.targets, sizeof(owner.saved_targets));
    MusashiStreamCpu state = {.pc = cpu->pc, .npc = cpu->npc};
    memcpy(state.r, cpu->r, sizeof(state.r));
    memcpy(owner.verified_code, image->cd_code + image->stream_code_offset, sizeof(owner.verified_code));
    memcpy(owner.verified_targets, image->stream_targets, sizeof(owner.verified_targets));
    const uint8_t *code = owner.verified_code;
    const MusashiStreamCall offer = {code, cpu->clock, cpu->cd_irq_frame, budget,
        &owner, stream_cd_current, stream_cd_entered, phase};
    MusashiStreamBus bus = musashi_stream_memory_bus(memory, owner.verified_targets);
    MusashiStreamCallResult result = musashi_stream_execute_work(&offer, &bus, &state, work);
    if (result == MUSASHI_STREAM_DEFER || result == MUSASHI_STREAM_WITNESS_FAILED) return result;
    memcpy(cpu->r, state.r, sizeof(cpu->r));
    cpu->pc = state.pc; cpu->npc = state.npc;
    cpu->delay_slot = (uint8_t)state.delay_slot; cpu->branch_pc = state.branch_pc;
    *bundled = musashi_stream_bundled_delay(code, &state) ? state.branch_pc : 0;
    return result;
}
