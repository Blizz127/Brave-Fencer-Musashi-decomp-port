/* The cold mount authenticates the full executable. Remember the identity of
 * this public 124-byte function without retaining its instructions. */
enum { COUNTER_ENTRY = 0x800118acu, COUNTER_SIZE = 124u, COUNTER_CYCLES = 70u,
       COUNTER_BASE = 0x800af630u, COUNTER_FIELDS = COUNTER_BASE + 0xa3b4u };

enum { STORY_READ = 0x80029504u, STORY_WRITE = 0x80029514u,
       STORY_WORD = 0x80078e80u, STORY_LEAF_SIZE = 16u, STORY_WORD_CYCLES = 9u };

enum { PAD_EDGE_MAP = 0x80014ed4u, PAD_HELD_MAP = 0x80015018u };

static int mounted_leaf_target(uint32_t pc)
{
    return pc == COUNTER_ENTRY || pc == STORY_READ || pc == STORY_WRITE ||
        pc == PAD_EDGE_MAP || pc == PAD_HELD_MAP;
}

static void scene_counter_capture(LaneRunFrame *root, MusashiBootMemory *ram,
                                  const FormatterCpu *cpu)
{
    const uint8_t *body = musashi_boot_ram_span(ram, COUNTER_ENTRY, COUNTER_SIZE);
    if (root->main_entry.memory == ram && game_main_owner(root, ram, cpu) &&
        field_scene_bus(cpu) && intrinsic_span(cpu, COUNTER_ENTRY, COUNTER_SIZE) &&
        body && cpu->clock->userdata == cpu->callback_device->userdata && cpu->callback_device->charge_quiet &&
        musashi_sha256_digest(body, COUNTER_SIZE, root->counter.digest)) {
        root->counter.mounted = 1;
        root->mounted_call.pay = cpu->callback_device->charge_quiet;
    }
}

/* The mounted dispatcher delegates ordinary work inside its selected service.
 * Its own entry, return and delay transitions remain entirely with the parent. */
static int mounted_leaf_service_bound(LaneRunFrame *root, MusashiBootMemory *ram,
                                       const FormatterCpu *cpu)
{
    const SceneDispatchFrame *s = &root->scene;
    if (s->phase != SCENE_SERVICE || !s->owner || !s->image || !s->service ||
        !game_main_owner(root, ram, cpu) || (s->top & 3u) || s->top < 32u || !musashi_boot_ram_span(ram, s->top - 32u, 32u) ||
        (s->cell & 3u) || s->cell < SCENE_TABLE ||
        (uint64_t)s->cell + 4u > (uint64_t)SCENE_TABLE + 4u * SCENE_SLOTS)
        return 0;
    MusashiMain12Owner *owner = main12_owner_for(cpu, ram);
    return owner && owner == s->owner && cpu->callback_device->main12_owner == owner &&
        s->image && owner->image == s->image && scene_root(root, ram, cpu) &&
        scene_selected(s, ram, cpu) && scene_saved(s, ram, 4u);
}

static int mounted_leaf_same_parent(LaneRunFrame *root, MusashiBootMemory *ram,
                                     const FormatterCpu *cpu)
{
    const SceneDispatchFrame *s = &root->scene;
    if (s->phase != root->mounted_call.parent_phase) return 0;
    if (s->phase == SCENE_IDLE) return 1;
    return root->mounted_call.parent_invocation == g_scene_calls &&
        root->mounted_call.parent_owner == s->owner && root->mounted_call.parent_image == s->image &&
        root->mounted_call.parent_service == s->service && root->mounted_call.parent_target == s->target &&
        root->mounted_call.parent_cell == s->cell && root->mounted_call.parent_top == s->top &&
        !memcmp(root->mounted_call.parent_saved, s->parent, sizeof s->parent) &&
        mounted_leaf_service_bound(root, ram, cpu);
}

static int scene_counter_overlap(uint32_t a, uint32_t size_a, uint32_t b, uint32_t size_b)
{
    return (uint64_t)a < (uint64_t)b + size_b && (uint64_t)b < (uint64_t)a + size_a;
}

static int scene_counter_outer_code(uint32_t address, uint32_t size)
{
    return scene_counter_overlap(address, size, SCENE_PARENT_CALL, 8u) ||
        scene_counter_overlap(address, size, SCENE_PARENT_RESUME, 4u) ||
        scene_counter_overlap(address, size, SCENE_ENTRY, SCENE_LENGTH) ||
        scene_counter_overlap(address, size, SCENE_RESUME, 4u);
}

static int scene_counter_writes_overlap(uint32_t spill, uint32_t address, uint32_t size)
{
    return scene_counter_overlap(spill, 4u, address, size) ||
        scene_counter_overlap(COUNTER_FIELDS, 12u, address, size);
}

/* scene_selected has already authenticated the service range pointer. The
 * nested return may land inside that service, but cannot perform its return
 * to the wrapper or disturb the parent frame, selected table or code. */
static int scene_counter_separate_from_parent(const SceneDispatchFrame *s, MusashiBootMemory *ram,
                                              uint32_t origin, uint32_t spill)
{
    if (s->phase == SCENE_IDLE) return 1;
    const FormatterRange *range = s->service;
    if (!range || range->start != s->target || range->start >= range->end ||
        !musashi_boot_ram_span(ram, range->start, range->end - range->start))
        return 0;
    uint32_t length = range->end - range->start;
    return !scene_counter_outer_code(origin, 12u) &&
        !scene_counter_outer_code(COUNTER_ENTRY, COUNTER_SIZE) &&
        !scene_counter_overlap(COUNTER_ENTRY, COUNTER_SIZE, range->start, length) &&
        !scene_counter_outer_code(spill, 4u) && !scene_counter_outer_code(COUNTER_FIELDS, 12u) &&
        !scene_counter_writes_overlap(spill, s->top - 32u, 32u) &&
        !scene_counter_writes_overlap(spill, SCENE_TABLE, 4u * SCENE_SLOTS) &&
        !scene_counter_writes_overlap(spill, SCENE_MODE, 2u) &&
        !scene_counter_writes_overlap(spill, range->start, length);
}

/* Only normally retired links in the outer CPU loop can supply this ticket.
 * A separate iteration must retire the original branch's delay slot. */
static void mounted_leaf_link(LaneRunFrame *root, MusashiBootMemory *ram, const FormatterCpu *cpu, uint32_t origin)
{
    if (root && root->mounted_call.pay && root->cpu == cpu && !root->prev && !g_lane_active &&
        cpu->pc == origin + 4u && mounted_leaf_target(cpu->npc) && cpu->delay_slot &&
        cpu->branch_pc == origin && cpu->r[31] == origin + 8u) {
        const SceneDispatchFrame *s = &root->scene;
        root->mounted_call.delay_step = root->mounted_call.entry_step = 0;
        if (s->phase != SCENE_IDLE && !mounted_leaf_service_bound(root, ram, cpu)) return;
        root->mounted_call.parent_phase = s->phase;
        if (s->phase == SCENE_SERVICE) {
            root->mounted_call.parent_owner = s->owner;
            root->mounted_call.parent_image = s->image;
            root->mounted_call.parent_service = s->service;
            root->mounted_call.parent_target = s->target;
            root->mounted_call.parent_cell = s->cell;
            root->mounted_call.parent_top = s->top;
            root->mounted_call.parent_invocation = g_scene_calls;
            memcpy(root->mounted_call.parent_saved, s->parent, sizeof s->parent);
        }
        root->mounted_call.origin = origin;
        root->mounted_call.target = cpu->npc;
        root->mounted_call.delay_step = root->mounted_call.step + 1u;
    }
}

static int mounted_leaf_delay(LaneRunFrame *root, MusashiBootMemory *ram, const FormatterCpu *cpu)
{
    return root && root->mounted_call.delay_step && root->mounted_call.delay_step == root->mounted_call.step &&
        root->cpu == cpu && !g_lane_active && cpu->delay_slot && cpu->branch_pc == root->mounted_call.origin &&
        cpu->pc == root->mounted_call.origin + 4u && cpu->npc == root->mounted_call.target &&
        cpu->r[31] == root->mounted_call.origin + 8u && mounted_leaf_same_parent(root, ram, cpu);
}

static int mounted_leaf_ready(LaneRunFrame *root, MusashiBootMemory *ram, const FormatterCpu *cpu)
{
    if (!root || !root->mounted_call.entry_step || root->mounted_call.entry_step != root->mounted_call.step)
        return 0;
    root->mounted_call.entry_step = 0;
    if (!game_main_owner(root, ram, cpu) || !field_scene_bus(cpu) || g_lane_active ||
        cpu->pc != root->mounted_call.target || cpu->npc != cpu->pc + 4u || cpu->delay_slot || cpu->branch_pc ||
        cpu->r[31] != root->mounted_call.origin + 8u || cpu->callback_device->charge_quiet != root->mounted_call.pay ||
        !root->mounted_call.pay || cpu->field_load.reg || cpu->scene_load.reg || cpu->title_mixer ||
        cpu->irq_dma || cpu->irq_draw || cpu->movie_output || cpu->main12_callback_frame ||
        cpu->main12_foreground_owner || cpu->main12_card_service || cpu->main12_row_pc ||
        cpu->main12_clearing_file || cpu->main12_building_row || cpu->main12_room_resuming ||
        cpu->main12_payload_pc || cpu->main12_copy_pending || cpu->room_reset_owner ||
        cpu->room_scene.owner || cpu->room_latch.owner || root->field_scene.phase != FIELD_IDLE ||
        !mounted_leaf_same_parent(root, ram, cpu))
        return 0;

    return 1;
}

static int scene_counter_offer(LaneRunFrame *root, MusashiBootMemory *ram, FormatterCpu *cpu)
{
    const uint32_t sp = cpu->r[29];
    if (!root->counter.mounted || (sp & 3u) || sp < 8u) return 0;
    const uint32_t spill = sp - 8u;
    const uint32_t origin = root->mounted_call.origin;
    /* A stack in cached main RAM must not alias the known game state,
     * resident EXE text or this call's link, delay and return instructions. */
    if (origin < MUSASHI_NATIVE_LANE_GUEST_BASE ||
        origin > MUSASHI_NATIVE_LANE_GUEST_BASE + MUSASHI_RAM_SIZE - 12u ||
        (spill < COUNTER_FIELDS + 12u && sp - 4u > COUNTER_BASE) ||
        (spill < CODE_IMAGE_END && sp - 4u > CODE_IMAGE_BASE) ||
        (spill < origin + 12u && sp - 4u > origin) ||
        !scene_counter_separate_from_parent(&root->scene, ram, origin, spill))
        return 0;
    uint8_t *stack = musashi_boot_ram_span(ram, spill, 8u);
    uint8_t *fields = musashi_boot_ram_span(ram, COUNTER_FIELDS, 12u);
    const uint8_t *body = musashi_boot_ram_span(ram, COUNTER_ENTRY, COUNTER_SIZE);
    uint8_t digest[32];
    if (!stack || !fields || !body || !intrinsic_span(cpu, COUNTER_ENTRY, COUNTER_SIZE) ||
        !intrinsic_span(cpu, COUNTER_FIELDS, 12u) || !intrinsic_span(cpu, spill, 4u) ||
        cpu_ram_span(ram, cpu, spill, 4u) != stack ||
        !musashi_sha256_digest(body, COUNTER_SIZE, digest) ||
        memcmp(digest, root->counter.digest, sizeof digest))
        return 0;
    for (uint32_t offset=0; offset<COUNTER_SIZE; offset+=4u) {
        if (field_scene_reserved(COUNTER_ENTRY + offset)) return 0;
    }

    int paid = root->mounted_call.pay(cpu->callback_device->userdata, cpu, COUNTER_CYCLES);
    if (paid != 1) {
        if (!paid) ++root->counter.quiet_declines;
        return paid ? -1 : 0;
    }
    /* After preflight these pointers name ordinary RAM owned for the whole
     * paid interval. The spill precedes the public game's five field stores. */
    if (!musashi_boot_write32(ram, spill, cpu->r[30])) return -1;
    uint32_t next = musashi_advance_scene_fields(fields);
    cpu->r[1] = COUNTER_BASE + 0x10000u;
    cpu->r[2] = COUNTER_BASE;
    cpu->r[3] = cpu->r[4] = next;
    ++root->counter.completed;
    return formatter_return(cpu) ? 1 : -1;
}
