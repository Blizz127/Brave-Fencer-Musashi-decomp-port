/* Command 1 resets transient flags on the room's terminated list of packed
 * addresses, then enables two scene-state flags. Only the observed SC01 room
 * constructor supplies this context. Other calls keep their original path. */
enum {
    ROOM_FLAGS_ENTRY = 0x801552f4u, ROOM_FLAGS_RESUME = 0x80145df0u,
    ROOM_FLAGS_LIST = 0x800de2a4u, ROOM_FLAGS_CAPACITY = 32u,
    ROOM_FLAGS_CONTEXT = 0x80126b58u, ROOM_FLAGS_STATE = ROOM_FLAGS_CONTEXT + 0x44u
};

static int room_flags_owned(LaneRunFrame *root, MusashiBootMemory *ram, FormatterCpu *cpu)
{
    const Sc01RoomFrame *room = &cpu->sc01_room;
    uint64_t issuer, leaf;
    uint32_t resume;
    if (!game_main_owner(root, ram, cpu) || !field_scene_bus(cpu) || g_lane_active ||
        room_reset_active(cpu) || room_scene_active(cpu) || room_latch_active(cpu) ||
        !root->mounted_call.pay || root->mounted_call.pay != cpu->callback_device->charge_quiet ||
        cpu->clock->userdata != cpu->callback_device->userdata || cpu->scene_load.reg || cpu->field_load.reg ||
        room->memory != ram || !sc01_room_expected(cpu) || !sc01_room_position(room, cpu) ||
        cpu->delay_slot || cpu->branch_pc || cpu->pc != ROOM_FLAGS_ENTRY || cpu->npc != cpu->pc + 4u ||
        cpu->r[31] != ROOM_FLAGS_RESUME || cpu->r[4] != ROOM_FLAGS_CONTEXT || cpu->r[16] != cpu->r[4] ||
        room->caller_sp < 56u || cpu->r[29] != room->caller_sp - 56u ||
        !sc01_room_caller(ram, cpu, room->caller_sp) || !sc01_room_image(ram, cpu) ||
        !intrinsic_span(cpu, cpu->r[29] + 28u, 4u) ||
        !musashi_boot_read32(ram, cpu->r[29] + 28u, &resume) || resume != 0x80128498u) return 0;
    if (root->field_scene.phase != FIELD_IDLE &&
        (root->field_scene.phase != FIELD_SERVING || !field_scene_expected(&root->field_scene, cpu) ||
         !field_scene_owner(root, ram, cpu))) return 0;
    if (root->scene.phase != SCENE_IDLE &&
        (root->scene.phase != SCENE_SERVICE || !mounted_leaf_service_bound(root, ram, cpu))) return 0;
    for (unsigned offset = 0; offset < 80u; offset += 4u)
        if (field_scene_reserved(ROOM_FLAGS_ENTRY + offset)) return 0;
    return intrinsic_span(cpu, 0x80145cecu, 508u) && intrinsic_span(cpu, ROOM_FLAGS_ENTRY, 80u) &&
           span_digest(ram, 0x80145cecu, 127u, &issuer) && issuer == UINT64_C(0xa4f61ad9fb19051a) &&
           span_digest(ram, ROOM_FLAGS_ENTRY, 20u, &leaf) && leaf == UINT64_C(0xcb53ae523b45a62d) &&
           intrinsic_span(cpu, ROOM_FLAGS_LIST, ROOM_FLAGS_CAPACITY * 4u) &&
           musashi_boot_ram_span(ram, ROOM_FLAGS_LIST, ROOM_FLAGS_CAPACITY * 4u) &&
           cpu_ram_span(ram, cpu, ROOM_FLAGS_LIST, 4u) == musashi_boot_ram_span(ram, ROOM_FLAGS_LIST, 4u) &&
           intrinsic_span(cpu, ROOM_FLAGS_STATE, 4u) &&
           cpu_ram_span(ram, cpu, ROOM_FLAGS_STATE, 4u) == musashi_boot_ram_span(ram, ROOM_FLAGS_STATE, 4u);
}

static int room_flags_offer(LaneRunFrame *root, MusashiBootMemory *ram, FormatterCpu *cpu)
{
    if (!room_flags_owned(root, ram, cpu)) return 0;
    uint32_t entries[ROOM_FLAGS_CAPACITY], flags;
    unsigned count = 0;
    for (; count < ROOM_FLAGS_CAPACITY; ++count) {
        if (!musashi_boot_read32(ram, ROOM_FLAGS_LIST + count * 4u, &entries[count])) return 0;
        if (!entries[count]) break;
    }
    if (count == ROOM_FLAGS_CAPACITY || !musashi_boot_read32(ram, ROOM_FLAGS_STATE, &flags)) return 0;

    /* The original work varies with the number of nonempty records. A real
     * intervening device event leaves the call on the individual-issue path. */
    const uint32_t cycles = count ? 31u + 14u * count : 25u;
    const uint64_t generation = g_lane_generation;
    const unsigned long long invocation = g_scene_calls;
    GameMainEntry main;
    int (*pay)(void *, const void *, uint32_t) = root->mounted_call.pay;
    Sc01RoomFrame room;
    SceneDispatchFrame scene;
    FieldScene field;
    memcpy(&main, &root->main_entry, sizeof main);
    memcpy(&room, &cpu->sc01_room, sizeof room);
    memcpy(&scene, &root->scene, sizeof scene);
    memcpy(&field, &root->field_scene, sizeof field);
    uint32_t before[32], hi = cpu->hi, lo = cpu->lo;
    memcpy(before, cpu->r, sizeof before);
    int paid = pay(cpu->callback_device->userdata, cpu, cycles);
    if (paid != 1) {
        if (!paid) ++root->room_flags.declines;
        return paid ? -1 : 0;
    }
    uint32_t current;
    if (generation != g_lane_generation || invocation != g_scene_calls || root->mounted_call.pay != pay ||
        memcmp(&main, &root->main_entry, sizeof main) ||
        memcmp(&room, &cpu->sc01_room, sizeof room) || memcmp(&scene, &root->scene, sizeof scene) ||
        memcmp(&field, &root->field_scene, sizeof field) || !room_flags_owned(root, ram, cpu) ||
        memcmp(before, cpu->r, sizeof before) || cpu->hi != hi || cpu->lo != lo ||
        !musashi_boot_read32(ram, ROOM_FLAGS_STATE, &current) || current != flags) return -1;
    for (unsigned i = 0; i <= count; ++i)
        if (!musashi_boot_read32(ram, ROOM_FLAGS_LIST + i * 4u, &current) || current != entries[i]) return -1;

    const uint32_t retained_bits = UINT32_C(0x80ffffff);
    for (unsigned i = 0; i < count; ++i)
        if (!musashi_boot_write32(ram, ROOM_FLAGS_LIST + i * 4u, entries[i] & retained_bits)) return -1;
    const uint32_t enabled = UINT32_C(0x30000000);
    flags |= enabled;
    if (!musashi_boot_write32(ram, ROOM_FLAGS_STATE, flags)) return -1;
    cpu->r[2] = flags;
    cpu->r[3] = enabled;
    if (count) cpu->r[5] = retained_bits;
    if (!formatter_return(cpu)) return -1;
    sc01_room_remember(&cpu->sc01_room, cpu);
    if (root->field_scene.phase == FIELD_SERVING)
        field_scene_lane_retired(root, ram, cpu, ROOM_FLAGS_ENTRY);
    ++root->room_flags.calls;
    root->room_flags.records += count;
    root->room_flags.cycles += cycles;
    return 1;
}
