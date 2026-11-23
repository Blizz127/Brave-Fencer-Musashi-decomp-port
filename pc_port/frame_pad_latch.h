/* The compiled frame poll calls the resident latch through a nested CPU run.
 * Keep its home area, entry checkpoint and return contract. The analog handler
 * still runs on the ordinary path after the button history has been latched. */
enum { PAD_POLL_SIZE = 120u, PAD_LATCH_SIZE = 400u, PAD_RECORD_SIZE = 76u,
       PAD_ANALOG = 0x80018f20u, PAD_AFTER_ANALOG = 0x80018e58u };

static int frame_pad_image(MusashiBootMemory *memory, const FormatterCpu *cpu,
                           uint32_t address, size_t size, uint8_t digest[32]) {
    const uint8_t *bytes = musashi_boot_ram_span(memory, address, size);
    return bytes && intrinsic_span(cpu, address, size) && musashi_sha256_digest(bytes, size, digest);
}

static void frame_pad_capture(LaneRunFrame *root, MusashiBootMemory *memory, FormatterCpu *cpu) {
    if (g_installed_frame_poll && game_main_owner(root, memory, cpu) && field_scene_bus(cpu) &&
        cpu->clock->userdata == cpu->callback_device->userdata &&
        frame_pad_image(memory, cpu, FRAME_PAD_POLL, PAD_POLL_SIZE, root->pad_latch.poll_digest) &&
        frame_pad_image(memory, cpu, PAD_LATCH, PAD_LATCH_SIZE, root->pad_latch.latch_digest)) {
        root->pad_latch.pay = cpu->callback_device->charge_quiet;
        root->pad_latch.generation = g_lane_generation;
    }
}

static int frame_pad_offer(LaneRunFrame *nested, MusashiBootMemory *memory, FormatterCpu *cpu) {
    LaneRunFrame *root = nested->prev;
    const LaneActive *caller = g_lane_active;
    uint8_t digest[32];
    if (!root || !root->pad_latch.pay || nested != g_lane_run_frames || nested->cpu != cpu ||
        nested->mounted_call.step || !game_main_bound(root, memory, cpu) || !field_scene_bus(cpu) ||
        root->pad_latch.generation != g_lane_generation || !caller || caller->provider != g_installed_frame_poll ||
        caller->cpu != cpu || caller->memory != memory || caller->guest_pc != FRAME_PAD_POLL ||
        cpu->callback_device->charge_quiet != root->pad_latch.pay ||
        cpu->npc != PAD_LATCH + 4u || cpu->pc != PAD_LATCH || cpu->delay_slot || cpu->branch_pc ||
        cpu->r[4] != PAD_FIRST || cpu->r[31] != UINT32_MAX || cpu->field_load.reg || cpu->scene_load.reg ||
        cpu->irq_dma || cpu->irq_draw || cpu->movie_output || cpu->title_mixer || cpu->main12_callback_frame ||
        cpu->main12_foreground_owner || cpu->main12_card_service || cpu->main12_row_pc ||
        cpu->main12_clearing_file || cpu->main12_building_row || cpu->main12_room_resuming ||
        cpu->main12_payload_pc || cpu->main12_copy_pending || cpu->room_reset_owner ||
        cpu->room_scene.owner || cpu->room_latch.owner || root->field_scene.phase != FIELD_IDLE ||
        root->scene.phase != SCENE_IDLE) return 0;

    const MusashiNativeLaneEntry *poll = lane_find(FRAME_PAD_POLL);
    if (!poll || poll->fn != caller->provider ||
        !frame_pad_image(memory, cpu, FRAME_PAD_POLL, PAD_POLL_SIZE, digest) ||
        memcmp(digest, root->pad_latch.poll_digest, sizeof digest) ||
        !frame_pad_image(memory, cpu, PAD_LATCH, PAD_LATCH_SIZE, digest) ||
        memcmp(digest, root->pad_latch.latch_digest, sizeof digest)) return 0;
    for (unsigned offset = 0; offset < PAD_LATCH_SIZE; offset += 4u)
        if (lane_find(PAD_LATCH + offset) || lane_ovl_first(PAD_LATCH + offset) != g_lane_ovl_count) return 0;

    const uint32_t sp = cpu->r[29];
    if ((sp & 7u) || sp < CODE_IMAGE_END + 24u ||
        scene_counter_overlap(sp - 24u, 24u, PAD_FIRST, PAD_RECORD_SIZE * 2u) ||
        !musashi_boot_ram_span(memory, sp - 24u, 24u) || !intrinsic_span(cpu, sp - 24u, 24u) ||
        !intrinsic_span(cpu, PAD_FIRST, PAD_RECORD_SIZE)) return 0;
    volatile uint8_t *record = musashi_boot_ram_span(memory, PAD_FIRST, PAD_RECORD_SIZE);
    if (!record) return 0;
    const unsigned type = record[0];
    if (type != 0x41 && type != 0x53 && type != 0x73) return 0;
    const int digital = type == 0x41;
    /* Aggregate costs end at the digital return or at the analog handler's
     * first checkpoint, before that handler can inspect or modify the record. */
    const unsigned clocks = digital ? 111u : type == 0x53 ? 226u : 230u;
    int paid = root->pad_latch.pay(cpu->callback_device->userdata, cpu, clocks);
    if (paid != 1) {
        if (!paid) ++root->pad_latch.quiet_declines;
        return paid ? -1 : 0;
    }

    /* A paid interval cannot be replayed if a replacement violated the
     * quiet-device contract. Stop before publishing the first guest write. */
    if (root->pad_latch.generation != g_lane_generation || caller->provider != g_installed_frame_poll ||
        !musashi_boot_write32(memory, sp - 8u, cpu->r[31])) return -1;
    const unsigned length = digital ? 2u : 8u;
    uint8_t changed = 0, pressed = 0, released = 0;
    for (unsigned i = 0; i < length; ++i) {
        uint8_t previous = record[0x32u + i];
        uint8_t current = record[0x2au + i];
        record[0x42u + i] = previous;
        if (i < 2u) {
            changed = current ^ previous;
            record[0x3au + i] = current & changed;
            pressed |= current & changed;
            released |= previous & changed;
        }
        record[0x32u + i] = current;
    }
    root->pad_latch.cycles += clocks;
    root->pad_latch.presses += pressed != 0;
    root->pad_latch.releases += released != 0;
    /* These are the guest's observable loop and argument registers at the
     * return or handler boundary. Preserve the other registers and HI/LO. */
    cpu->r[2] = PAD_FIRST + length;
    cpu->r[5] = record[0x2bu];
    cpu->r[6] = length;
    cpu->r[7] = PAD_FIRST;
    if (digital) {
        cpu->r[3] = changed;
        cpu->r[4] = record[0x3bu];
        ++root->pad_latch.digital;
        return formatter_return(cpu) ? 1 : -1;
    }
    cpu->r[3] = record[0x49u];
    cpu->r[29] = sp - 24u;
    cpu->r[31] = PAD_AFTER_ANALOG;
    cpu->pc = PAD_ANALOG;
    cpu->npc = PAD_ANALOG + 4u;
    if (g_trace_state >= 0) musashi_trace_function_entry(PAD_ANALOG, PAD_AFTER_ANALOG - 8u, 'j');
    if (g_entry_hook) g_entry_pending = PAD_ANALOG;
    if (type == 0x53) ++root->pad_latch.analog53;
    else ++root->pad_latch.analog73;
    return 1;
}

static void frame_pad_report(const LaneRunFrame *root) {
    if (root->pad_latch.reset_issues || root->pad_latch.reset_declined_issues)
        fprintf(stderr, "native_boot: PAD_DISCONNECT issues=%llu writes=%llu returns=%llu "
                "port1=%llu port2=%llu declined_issues=%llu\n",
                root->pad_latch.reset_issues, root->pad_latch.reset_writes, root->pad_latch.reset_returns,
                root->pad_latch.reset_port_returns[0], root->pad_latch.reset_port_returns[1],
                root->pad_latch.reset_declined_issues);
    if (root->pad_latch.cycles || root->pad_latch.quiet_declines)
        fprintf(stderr, "native_boot: PAD_LATCH digital=%llu analog53=%llu analog73=%llu "
                "presses=%llu releases=%llu cycles=%llu quiet_declines=%llu\n",
                root->pad_latch.digital, root->pad_latch.analog53, root->pad_latch.analog73,
                root->pad_latch.presses, root->pad_latch.releases, root->pad_latch.cycles,
                root->pad_latch.quiet_declines);
}
