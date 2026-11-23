/* The status poll clears two button bytes backwards and then centers four
 * axes forwards. SDL polls, IRQ checkpoints and guest observers can see an
 * individual write or a loop's delay slot. Retire one issue per outer step. */
enum { PAD_DISCONNECT = 0x80018f88u, PAD_DISCONNECT_SIZE = 64u,
       PAD_DISCONNECT_CALL = 0x80018b04u, PAD_CURRENT = 42u };

static int controller_disconnect_issue(LaneRunFrame *nested, MusashiBootMemory *ram,
                                       FormatterCpu *cpu) {
    const LaneActive *caller = g_lane_active;
    LaneRunFrame *root = nested ? nested->prev : NULL;
    const uint32_t offset = cpu->pc - PAD_DISCONNECT;
    const uint32_t record = cpu->r[4];
    if (offset >= PAD_DISCONNECT_SIZE || (offset & 3u) || !root || nested->cpu != cpu ||
        nested != g_lane_run_frames || nested->mounted_call.step || !caller ||
        caller->cpu != cpu || caller->memory != ram ||
        caller->guest_pc != FRAME_PAD_POLL || caller->provider != g_installed_frame_poll ||
        !g_installed_frame_poll || !game_main_bound(root, ram, cpu) || !field_scene_bus(cpu) ||
        root->pad_latch.generation != g_lane_generation || !root->pad_latch.pay ||
        cpu->r[31] != PAD_DISCONNECT_CALL + 8u ||
        (record != PAD_FIRST && record != PAD_FIRST + PAD_RECORD_SIZE) ||
        cpu->irq_dma || cpu->irq_draw || cpu->movie_output || cpu->title_mixer ||
        cpu->main12_foreground_owner || cpu->main12_callback_frame || cpu->main12_card_service ||
        cpu->main12_row_pc || cpu->main12_clearing_file || cpu->main12_building_row ||
        cpu->main12_room_resuming || cpu->main12_payload_pc || cpu->main12_copy_pending ||
        cpu->room_reset_owner || cpu->room_scene.owner || cpu->room_latch.owner ||
        cpu->field_load.reg || cpu->scene_load.reg ||
        root->field_scene.phase != FIELD_IDLE || nested->field_scene.phase != FIELD_IDLE ||
        root->scene.phase != SCENE_IDLE || nested->scene.phase != SCENE_IDLE)
        return 0;

    const int delayed = offset == 20u || offset == 52u || offset == 60u;
    if (delayed ? (!cpu->delay_slot || cpu->branch_pc != cpu->pc - 4u)
                : (cpu->delay_slot || cpu->branch_pc || cpu->npc != cpu->pc + 4u))
        return 0;
    /* Both spans come from the player's resident image. Keep mutations, other
     * fetch providers and other port-service callers on their ordinary path. */
    const uint8_t *image = code_image();
    const uint8_t *body = musashi_boot_ram_span(ram, PAD_DISCONNECT, PAD_DISCONNECT_SIZE);
    const uint8_t *origin = musashi_boot_ram_span(ram, PAD_DISCONNECT_CALL, 8u);
    if (!image || g_code_image_state != 1 || !body || !origin || field_scene_reserved(cpu->pc) ||
        !intrinsic_span(cpu, PAD_DISCONNECT, PAD_DISCONNECT_SIZE) ||
        !intrinsic_span(cpu, PAD_DISCONNECT_CALL, 8u) ||
        !intrinsic_span(cpu, record, PAD_RECORD_SIZE) ||
        memcmp(body, image + CODE_IMAGE_TEXT + (PAD_DISCONNECT - CODE_IMAGE_BASE), PAD_DISCONNECT_SIZE) ||
        memcmp(origin, image + CODE_IMAGE_TEXT + (PAD_DISCONNECT_CALL - CODE_IMAGE_BASE), 8u))
        return 0;

    const int writes = offset == 8u || offset == 36u;
    const uint32_t cost = writes ? 3u : 2u;
    if (!cpu->clock->advance(cpu->clock->userdata, cost) ||
        (cpu->cd_irq_frame && cpu->cd_irq_frame->faulted)) return -1;

    /* Inputs are read after the normal clock callback, at the same point as
     * the guest observes them. A denied clock above retires nothing. */
    const uint32_t next = cpu->npc;
    uint32_t after = next + 4u;
    const int branch = offset == 16u || offset == 48u || offset == 56u;
    if (writes) {
        uint32_t value = offset == 8u ? 0u : cpu->r[5];
        if (!cpu_write8(ram, cpu, cpu->r[2] + PAD_CURRENT, value)) return -1;
        ++root->pad_latch.reset_writes;
    } else if (offset < 24u) {
        /* Descending button index and cursor. The final decrement in the
         * delay slot leaves the cursor one guest byte below the record. */
        switch (offset) {
        case 0:  cpu->r[3] = 1u; break;
        case 4:  cpu->r[2] = cpu->r[4] + 1u; break;
        case 12: --cpu->r[3]; break;
        case 16: if ((int32_t)cpu->r[3] >= 0) after = cpu->pc - 8u; break;
        case 20: --cpu->r[2]; break;
        }
    } else {
        /* The last loop comparison stays in v0 across the return. */
        switch (offset) {
        case 24: cpu->r[3] = 2u; break;
        case 28: cpu->r[5] = 128u; break;
        case 32: cpu->r[2] = cpu->r[4] + cpu->r[3]; break;
        case 40: ++cpu->r[3]; break;
        case 44: cpu->r[2] = (int32_t)cpu->r[3] < 6; break;
        case 48: if (cpu->r[2]) after = cpu->pc - 16u; break;
        case 56: after = cpu->r[31]; break;
        }
    }
    cpu->r[0] = 0;
    cpu->delay_slot = (uint8_t)branch;
    cpu->branch_pc = branch ? cpu->pc : 0;
    cpu->pc = next;
    cpu->npc = after;
    ++root->pad_latch.reset_issues;
    if (offset == 60u && next == PAD_DISCONNECT_CALL + 8u) {
        ++root->pad_latch.reset_returns;
        ++root->pad_latch.reset_port_returns[record != PAD_FIRST];
    }
    return 1;
}
