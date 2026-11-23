/* The two resident mappers read the edge and held words of the same input
 * record. A nonzero word is their complete result for every controller kind. */
static const struct {
    uint32_t entry, size, offset;
} controller_mappers[] = {
    { PAD_EDGE_MAP, 324u, 0x3au },
    { PAD_HELD_MAP, 256u, 0x32u }
};

static void controller_mapper_capture(LaneRunFrame *root, MusashiBootMemory *memory,
                                      FormatterCpu *cpu) {
    if (!game_main_owner(root, memory, cpu) || !field_scene_bus(cpu) ||
        cpu->clock->userdata != cpu->callback_device->userdata || !cpu->callback_device->charge_quiet)
        return;
    for (unsigned i = 0; i < 2u; ++i) {
        if (frame_pad_image(memory, cpu, controller_mappers[i].entry, controller_mappers[i].size,
                            root->pad_mapper.digest[i])) {
            root->pad_mapper.mounted[i] = 1;
            root->mounted_call.pay = cpu->callback_device->charge_quiet;
        }
    }
}

static void controller_mapper_report(const LaneRunFrame *root) {
    if (root->pad_mapper.cycles || root->pad_mapper.quiet_declines || root->pad_mapper.analog_guest)
        fprintf(stderr, "native_boot: PAD_MAP edge_mask=%llu held_mask=%llu edge_empty=%llu held_empty=%llu "
                "disconnected=%llu analog_guest=%llu cycles=%llu quiet_declines=%llu\n",
                root->pad_mapper.mask[0], root->pad_mapper.mask[1],
                root->pad_mapper.empty[0], root->pad_mapper.empty[1], root->pad_mapper.disconnected,
                root->pad_mapper.analog_guest,
                root->pad_mapper.cycles, root->pad_mapper.quiet_declines);
}

static int controller_mapper_offer(LaneRunFrame *root, MusashiBootMemory *ram, FormatterCpu *cpu) {
    const unsigned held = cpu->pc == PAD_HELD_MAP;
    const uint32_t entry = cpu->pc, size = controller_mappers[held].size;
    const uint32_t origin = root->mounted_call.origin, sp = cpu->r[29];
    if (!root->pad_mapper.mounted[held] || cpu->r[4] > 1u || (sp & 3u) || sp < CODE_IMAGE_END + 8u ||
        origin < MUSASHI_NATIVE_LANE_GUEST_BASE || origin > MUSASHI_NATIVE_LANE_GUEST_BASE + MUSASHI_RAM_SIZE - 12u)
        return 0;
    uint8_t digest[32];
    if (!frame_pad_image(ram, cpu, entry, size, digest) ||
        memcmp(digest, root->pad_mapper.digest[held], sizeof digest)) return 0;
    for (unsigned offset = 0; offset < size; offset += 4u)
        if (field_scene_reserved(entry + offset)) return 0;

    const uint32_t record = PAD_FIRST + cpu->r[4] * PAD_RECORD_SIZE;
    const uint32_t address = record + controller_mappers[held].offset;
    const uint32_t spill = sp - 8u;
    const uint8_t *word = musashi_boot_ram_span(ram, address, 2u);
    const uint8_t *kind = musashi_boot_ram_span(ram, record, 1u);
    uint8_t *saved = musashi_boot_ram_span(ram, spill, 4u);
    if (!word || !kind || !saved || !intrinsic_span(cpu, record, PAD_RECORD_SIZE) || !intrinsic_span(cpu, spill, 4u) ||
        cpu_ram_span(ram, cpu, address, 2u) != word || cpu_ram_span(ram, cpu, record, 1u) != kind ||
        cpu_ram_span(ram, cpu, spill, 4u) != saved ||
        scene_counter_overlap(spill, 4u, PAD_FIRST, 2u * PAD_RECORD_SIZE) ||
        scene_counter_overlap(spill, 4u, origin, 12u)) return 0;
    if (root->scene.phase == SCENE_SERVICE) {
        const SceneDispatchFrame *scene = &root->scene;
        const FormatterRange *service = scene->service;
        if (!service || service->start != scene->target || service->start >= service->end ||
            !musashi_boot_ram_span(ram, service->start, service->end - service->start) ||
            scene_counter_outer_code(origin, 12u) || scene_counter_outer_code(spill, 4u) ||
            scene_counter_overlap(entry, size, service->start, service->end - service->start) ||
            scene_counter_overlap(spill, 4u, service->start, service->end - service->start) ||
            scene_counter_overlap(spill, 4u, scene->top - 32u, 32u) ||
            scene_counter_overlap(spill, 4u, SCENE_TABLE, 4u * SCENE_SLOTS) ||
            scene_counter_overlap(spill, 4u, SCENE_MODE, 2u)) return 0;
    }

    const uint32_t mask = word[0] | ((uint32_t)word[1] << 8);
    const uint32_t pad_kind = mask ? 0u : *kind;
    if (!mask && pad_kind != 0u && pad_kind != 0x41u) {
        if (pad_kind == 0x53u || pad_kind == 0x73u) ++root->pad_mapper.analog_guest;
        return 0;
    }
    /* Both resident entries have these same two measured return paths. The
     * analog mapping path still runs its own routines and timing ordinarily. */
    const unsigned clocks = mask ? 41u : 54u;
    const int paid = root->mounted_call.pay(cpu->callback_device->userdata, cpu, clocks);
    if (paid != 1) {
        if (!paid) ++root->pad_mapper.quiet_declines;
        return paid ? -1 : 0;
    }
    if (!musashi_boot_write32(ram, spill, cpu->r[31])) return -1;
    cpu->r[2] = mask;
    cpu->r[3] = mask ? record : pad_kind;
    if (mask) ++root->pad_mapper.mask[held];
    else {
        ++root->pad_mapper.empty[held];
        if (!pad_kind) ++root->pad_mapper.disconnected;
    }
    root->pad_mapper.cycles += clocks;
    return formatter_return(cpu) ? 1 : -1;
}
