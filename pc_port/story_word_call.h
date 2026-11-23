/* The two adjacent, matched story routines access a single resident s32.
 * Capture their identities from the authenticated cold EXE in this root. */
static void story_word_capture(LaneRunFrame *root, MusashiBootMemory *ram, const FormatterCpu *cpu)
{
    const uint8_t *read = musashi_boot_ram_span(ram, STORY_READ, STORY_LEAF_SIZE);
    const uint8_t *write = musashi_boot_ram_span(ram, STORY_WRITE, STORY_LEAF_SIZE);
    if (!read || !write || !root->main_entry.memory || !game_main_owner(root, ram, cpu) ||
        !field_scene_bus(cpu) || !intrinsic_span(cpu, STORY_READ, 2u * STORY_LEAF_SIZE) ||
        cpu->clock->userdata != cpu->callback_device->userdata || !cpu->callback_device->charge_quiet)
        return;
    if (musashi_sha256_digest(read, STORY_LEAF_SIZE, root->story_word.read_digest) &&
        musashi_sha256_digest(write, STORY_LEAF_SIZE, root->story_word.write_digest)) {
        root->story_word.mounted = 1;
        root->mounted_call.pay = cpu->callback_device->charge_quiet;
    }
}

static int story_word_offer(LaneRunFrame *root, MusashiBootMemory *ram, FormatterCpu *cpu)
{
    const uint32_t entry = cpu->pc, origin = root->mounted_call.origin;
    const int writing = entry == STORY_WRITE;
    const uint8_t *word = musashi_boot_ram_span(ram, STORY_WORD, 4u);
    const uint8_t *code = musashi_boot_ram_span(ram, entry, STORY_LEAF_SIZE);
    uint8_t digest[32];
    if (!root->story_word.mounted || !word || !code ||
        origin < MUSASHI_NATIVE_LANE_GUEST_BASE ||
        origin > MUSASHI_NATIVE_LANE_GUEST_BASE + MUSASHI_RAM_SIZE - 12u ||
        !intrinsic_span(cpu, entry, STORY_LEAF_SIZE) || !intrinsic_span(cpu, STORY_WORD, 4u) ||
        cpu_ram_span(ram, cpu, STORY_WORD, 4u) != word ||
        !musashi_sha256_digest(code, STORY_LEAF_SIZE, digest) ||
        memcmp(digest, writing ? root->story_word.write_digest : root->story_word.read_digest, sizeof digest))
        return 0;
    for (uint32_t i = 0; i < STORY_LEAF_SIZE; i += 4u)
        if (field_scene_reserved(entry + i)) return 0;
    if (writing && scene_counter_overlap(STORY_WORD, 4u, origin, 12u)) return 0;
    if (root->scene.phase == SCENE_SERVICE) {
        const SceneDispatchFrame *s = &root->scene;
        const FormatterRange *service = s->service;
        if (!service || service->start != s->target || service->start >= service->end ||
            !musashi_boot_ram_span(ram, service->start, service->end - service->start) ||
            scene_counter_outer_code(origin, 12u) ||
            scene_counter_overlap(entry, STORY_LEAF_SIZE, service->start, service->end - service->start))
            return 0;
        if (writing && (scene_counter_overlap(STORY_WORD, 4u, s->top - 32u, 32u) ||
                        scene_counter_overlap(STORY_WORD, 4u, service->start, service->end - service->start)))
            return 0;
    }

    /* Four original issues take two source cycles apiece, plus one for RAM.
     * An intervening device event keeps the instruction-by-instruction path. */
    int paid = root->mounted_call.pay(cpu->callback_device->userdata, cpu, STORY_WORD_CYCLES);
    if (paid != 1) {
        if (!paid) ++root->story_word.quiet_declines;
        return paid ? -1 : 0;
    }
    if (writing) {
        if (!musashi_boot_write32(ram, STORY_WORD, cpu->r[4])) return -1;
        /* The store's split signed displacement leaves its base in $at. */
        cpu->r[1] = (STORY_WORD + 0x8000u) & 0xffff0000u;
        ++root->story_word.writes;
    } else {
        if (!musashi_boot_read32(ram, STORY_WORD, &cpu->r[2])) return -1;
        ++root->story_word.reads;
    }
    return formatter_return(cpu) ? 1 : -1;
}
