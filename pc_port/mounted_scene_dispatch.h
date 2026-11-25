/* Observe the call selected by the mounted game's foreground dispatcher.
 * The shared guest CPU issues every instruction in the wrapper. */
enum {
    SCENE_PARENT_CALL = 0x800105e4u, SCENE_PARENT_RESUME = SCENE_PARENT_CALL + 8u,
    SCENE_ENTRY = 0x80010b40u, SCENE_LENGTH = 116u,
    SCENE_INDIRECT_CALL = SCENE_ENTRY + 76u, SCENE_RESUME = SCENE_INDIRECT_CALL + 8u,
    SCENE_JR = SCENE_ENTRY + 108u,
    SCENE_STATE = 0x800af630u, SCENE_MODE = SCENE_STATE + 0xa3aeu,
    SCENE_TABLE = 0x800629f4u, SCENE_SLOTS = 18u
};

enum SceneWitness {
    SCENE_NO_WITNESS, SCENE_PARENT_JAL, SCENE_PARENT_SLOT, SCENE_WRAPPER_WORD,
    SCENE_CHILD_ENTRY, SCENE_CHILD_JR, SCENE_CHILD_SLOT
};

/* An existing ordinary CPU registration identifies a guest routine. Both
 * the callee and its actual return site are looked up while the game runs. */
static const FormatterRange *scene_registration(uint32_t pc) {
    static const struct { const FormatterRange *ranges; size_t count; } resident[] = {
#define SCENE_RANGES(a) {a, sizeof a / sizeof *a}
        SCENE_RANGES(kFormatterRanges0), SCENE_RANGES(kFormatterRanges1),
        SCENE_RANGES(kFormatterRanges2), SCENE_RANGES(kFormatterRanges3)
#undef SCENE_RANGES
    };
    for (unsigned i = 0; i < sizeof resident / sizeof *resident; ++i) {
        const FormatterRange *found = formatter_range_at(resident[i].ranges, resident[i].count, pc);
        if (found) return found;
    }
    return NULL;
}

static void scene_remember(SceneDispatchFrame *s, const FormatterCpu *cpu) {
    memcpy(s->regs, cpu->r, sizeof s->regs);
    s->hi = cpu->hi;
    s->lo = cpu->lo;
}

static int scene_registers(const SceneDispatchFrame *s, const FormatterCpu *cpu) {
    return !memcmp(s->regs, cpu->r, sizeof s->regs) && s->hi == cpu->hi && s->lo == cpu->lo;
}

static int scene_root(const LaneRunFrame *f, MusashiBootMemory *m, const FormatterCpu *cpu) {
    const SceneDispatchFrame *s = &f->scene;
    return f == g_lane_run_frames && f->cpu == cpu && !f->prev && f->entry_stop &&
        g_fetch_memory == m && musashi_native_lane_usable(m) && g_code_image_state != 2 &&
        !cpu->r[0] && !cpu->merge_pending && !cpu->gte_load_pending && !cpu->title_mixer &&
        !cpu->main12_callback_frame && !cpu->room_reset_owner && !room_scene_active(cpu) &&
        !room_latch_active(cpu) && cpu->callback_device == s->callback &&
        cpu->callback_device->checkpoint == s->checkpoint && cpu->callback_device->userdata == s->worker &&
        cpu->callback_device->field_restore_quiet == s->quiet &&
        cpu->clock == s->clock && cpu->clock->advance == s->charge.advance &&
        cpu->clock->userdata == s->charge.userdata && cpu->callback_device->main12_owner == s->owner &&
        s->image->main12_owner == s->owner && s->owner->image == s->image &&
        s->owner->exe_text == s->exe_text && s->owner->resident_matches == s->matches &&
        s->owner->resident_word == s->word && main12_owner_for(cpu, m) == s->owner &&
        intrinsic_span(cpu, SCENE_PARENT_CALL, 8u) && intrinsic_span(cpu, SCENE_ENTRY, SCENE_LENGTH) &&
        intrinsic_span(cpu, SCENE_MODE, 2u) && intrinsic_span(cpu, s->top - 32u, 32u) &&
        s->owner->resident_matches(s->owner, SCENE_PARENT_CALL, 8u) &&
        s->owner->resident_matches(s->owner, SCENE_ENTRY, SCENE_LENGTH);
}

/* Independent, immutable input for the selected word; the game still reads
 * the actual guest RAM cell at its original load issue. */
static int scene_selection(const SceneDispatchFrame *s, MusashiBootMemory *m,
                           const FormatterCpu *cpu, uint32_t cell, uint32_t *target,
                           const FormatterRange **service) {
    uint32_t mounted, live;
    if ((cell & 3u) || cell - SCENE_TABLE >= SCENE_SLOTS * 4u ||
        !intrinsic_span(cpu, cell, 4u) || !s->owner->resident_word(s->owner, cell, &mounted) ||
        !musashi_boot_read32(m, cell, &live) || live != mounted || (mounted & 3u) ||
        (mounted - SCENE_ENTRY < SCENE_LENGTH) || mounted == SCENE_PARENT_CALL ||
        !intrinsic_span(cpu, mounted, 4u) || !s->owner->resident_matches(s->owner, mounted, 4u))
        return 0;
    const FormatterRange *found = scene_registration(mounted);
    if (!found || found->start != mounted) return 0;
    *target = mounted;
    *service = found;
    return 1;
}

static int scene_selected(const SceneDispatchFrame *s, MusashiBootMemory *m, const FormatterCpu *cpu) {
    uint32_t target;
    const FormatterRange *service;
    return scene_selection(s, m, cpu, s->cell, &target, &service) &&
        target == s->target && service == s->service;
}

static int scene_saved(const SceneDispatchFrame *s, MusashiBootMemory *m, unsigned count) {
    for (unsigned i = 0; i < count; ++i) {
        uint32_t value;
        if (!musashi_boot_read32(m, s->top - 4u * (i + 1u), &value) || value != s->parent[i])
            return 0;
    }
    return 1;
}

static int scene_flow(const FormatterCpu *cpu, uint32_t pc, uint32_t branch, uint32_t next) {
    return cpu->pc == pc && cpu->npc == next && cpu->branch_pc == branch &&
        cpu->delay_slot == (branch != 0u);
}

/* A whole-call C provider has an ABI return but no individual guest return
 * instructions. Authenticate its original entry, active frame and complete
 * registered resident span around the call and its one aggregate charge. */
static int scene_aggregate_owned(const LaneRunFrame *f, MusashiBootMemory *m, const FormatterCpu *cpu,
                                 const MusashiNativeLaneEntry *issued, uint64_t generation,
                                 unsigned long long invocation, int returned) {
    const SceneDispatchFrame *s = &f->scene;
    if (s->phase != SCENE_SERVICE_ENTRY || g_scene_calls != invocation || s->next != s->target ||
        issued->guest_pc != s->target || generation != g_lane_generation || !scene_root(f, m, cpu) ||
        !scene_selected(s, m, cpu) || !scene_saved(s, m, 4u)) return 0;
    const MusashiNativeLaneEntry *registered = lane_find(s->target);
    uint32_t pc = returned ? SCENE_RESUME : s->target;
    uint32_t size = s->service->end - s->service->start;
    return registered && registered->fn == issued->fn && registered->cost_cycles == issued->cost_cycles &&
        registered->argc == issued->argc && registered->returns == issued->returns &&
        intrinsic_span(cpu, s->target, size) && s->matches(s->owner, s->target, size) &&
        scene_flow(cpu, pc, 0u, pc + 4u) && cpu->r[31] == SCENE_RESUME &&
        cpu->r[29] == s->top - 32u && cpu->r[30] == cpu->r[29] &&
        cpu->r[16] == SCENE_STATE && cpu->r[17] == s->target &&
        !memcmp(cpu->r + 18, s->regs + 18, 6u * sizeof cpu->r[0]);
}

static int scene_body_owned(const LaneRunFrame *f, MusashiBootMemory *m, const FormatterCpu *cpu) {
    const SceneDispatchFrame *s = &f->scene;
    uint32_t pc = cpu->pc;
    if (s->phase != SCENE_WRAPPER || pc != s->next || pc - SCENE_ENTRY >= SCENE_LENGTH ||
        (pc & 3u) || lane_find(pc) || !scene_root(f, m, cpu) || !scene_registers(s, cpu))
        return 0;
    unsigned i = (pc - SCENE_ENTRY) / 4u;
    unsigned saved = i > 4u ? 4u : (i ? i - 1u : 0u);
    uint32_t sp = (i == 0u || i >= 27u) ? s->top : s->top - 32u;
    uint32_t branch = pc == SCENE_INDIRECT_CALL + 4u ? SCENE_INDIRECT_CALL : pc == SCENE_JR + 4u ? SCENE_JR : 0u;
    uint32_t next = branch == SCENE_INDIRECT_CALL ? s->target : branch == SCENE_JR ? SCENE_PARENT_RESUME : pc + 4u;
    return scene_flow(cpu, pc, branch, next) && cpu->r[29] == sp && scene_saved(s, m, saved) &&
        (i < 17u || i > 20u || (cpu->r[2] == s->cell && scene_selected(s, m, cpu))) &&
        (i < 19u || i > 20u || cpu->r[17] == s->target);
}

/* The wrapper owns four saved registers, the scene mode and the selected
 * entry in the mounted service table. Check the real instruction's effective
 * address before using its retirement as evidence of a service call. */
static int scene_access_owned(const LaneRunFrame *f, MusashiBootMemory *m,
                              const FormatterCpu *cpu, uint32_t word) {
    unsigned op = word >> 26;
    if (op < 32u) return 1;
    const SceneDispatchFrame *s = &f->scene;
    uint32_t addr = cpu->r[(word >> 21) & 31u] + (uint32_t)(int32_t)(int16_t)word;
    if (op == 33u || op == 37u) return addr == SCENE_MODE;
    if (op == 35u && addr == s->cell) return scene_selected(s, m, cpu);
    if ((op != 35u && op != 43u) || (addr & 3u) || addr < s->top - 16u || addr >= s->top)
        return 0;
    unsigned slot = (s->top - addr) / 4u - 1u;
    uint32_t value;
    if (op == 43u) return cpu->r[(word >> 16) & 31u] == s->parent[slot];
    return musashi_boot_read32(m, addr, &value) && value == s->parent[slot];
}

static unsigned scene_scalar_destination(uint32_t word) {
    unsigned op = word >> 26, fn = word & 63u;
    if (!op && (fn <= 7u || fn == 9u || fn == 16u || fn == 18u ||
                (fn >= 32u && fn <= 43u))) return (word >> 11) & 31u;
    if (op == 3u) return 31u;
    if ((op >= 8u && op <= 15u) || op == 32u || op == 33u || op == 35u ||
        op == 36u || op == 37u || (op == 16u && ((word >> 21) & 31u) == 0u))
        return (word >> 16) & 31u;
    return 0;
}

static int scene_quiet(const FormatterCpu *cpu) {
    return cpu->callback_device && cpu->callback_device->field_restore_quiet &&
        cpu->callback_device->field_restore_quiet(cpu->callback_device->userdata, cpu);
}

static void scene_queue_load(FormatterCpu *cpu, unsigned reg, uint32_t value) {
    if (reg && value != cpu->r[reg]) {
        cpu->scene_load.owner = cpu;
        cpu->scene_load.device = cpu->callback_device;
        cpu->scene_load.quiet = cpu->callback_device->field_restore_quiet;
        cpu->scene_load.checkpoint = cpu->callback_device->checkpoint;
        cpu->scene_load.worker = cpu->callback_device->userdata;
        cpu->scene_load.successor = cpu->npc;
        cpu->scene_load.reg = reg;
        cpu->scene_load.value = value;
    }
}

/* If custody changes, the ordinary decoder still consumes the old bank.
 * In a chain, the first load becomes visible after the second has sampled
 * its operands, including when both loads name the same register. */
static void scene_scalar_retired(FormatterCpu *cpu, uint32_t word, uint32_t prior,
                                 int delay_load, int observed_load) {
    unsigned rt = (word >> 16) & 31u;
    uint32_t loaded = cpu->r[rt];
    if (delay_load && rt) {
        cpu->r[rt] = prior;
        scene_writeback(cpu, 0u);
        if (loaded != cpu->r[rt]) ++g_scene_delayed_loads;
        else ++g_scene_equal_loads;
        scene_queue_load(cpu, rt, loaded);
    } else {
        scene_writeback(cpu, scene_scalar_destination(word));
        if (observed_load) ++g_scene_eager_loads;
    }
}

/* Totals belong to the single foreground CPU runner. Its root baseline
 * separates successive boots; nested guest calls cannot acquire this frame. */
static void scene_report(const LaneRunFrame *f, const char *point) {
    fprintf(stderr, "native_boot: MOUNTED_SCENE_DISPATCH point=%s guest_issues=%llu "
            "calls=%llu child_returns=%llu returns=%llu aggregate_returns=%llu "
            "delayed_loads=%llu equal_loads=%llu scalar_eager_loads=%llu\n", point,
            g_scene_scalar_issues - f->scene_baseline.scalar,
            g_scene_calls - f->scene_baseline.calls, g_scene_child_returns - f->scene_baseline.children,
            g_scene_returns - f->scene_baseline.returns, g_scene_aggregate_returns - f->scene_baseline.aggregate,
            g_scene_delayed_loads - f->scene_baseline.delayed,
            g_scene_equal_loads - f->scene_baseline.equal, g_scene_eager_loads - f->scene_baseline.eager);
}

static int scene_bind_parent(LaneRunFrame *f, MusashiBootMemory *m, FormatterCpu *cpu) {
    SceneDispatchFrame *s = &f->scene;
    if (f->prev || !f->entry_stop || !musashi_native_lane_usable(m) ||
        !cpu->callback_device || !cpu->callback_device->main12_owner ||
        !cpu->callback_device->main12_owner->image) return 0;
    MusashiMain12Owner *owner = main12_owner_for(cpu, m);
    if (!owner || !owner->resident_word || !owner->resident_matches ||
        !cpu->clock || !cpu->clock->advance || !cpu->callback_device->checkpoint ||
        !scene_flow(cpu, SCENE_PARENT_CALL, 0u, SCENE_PARENT_CALL + 4u) ||
        (cpu->r[29] & 3u) || cpu->r[29] < CODE_IMAGE_END + 32u ||
        !musashi_boot_ram_span(m, cpu->r[29] - 32u, 32u)) return 0;
    *s = (SceneDispatchFrame){.owner = owner, .callback = cpu->callback_device,
        .quiet = cpu->callback_device->field_restore_quiet,
        .worker = cpu->callback_device->userdata, .checkpoint = cpu->callback_device->checkpoint,
        .clock = cpu->clock, .charge = *cpu->clock, .top = cpu->r[29],
        .image = owner->image, .exe_text = owner->exe_text, .matches = owner->resident_matches,
        .word = owner->resident_word,
        .parent = {SCENE_PARENT_RESUME, cpu->r[30], cpu->r[17], cpu->r[16]}};
    if (!scene_root(f, m, cpu)) return 0;
    uint16_t mode;
    if (!cpu_read16(m, cpu, SCENE_MODE, &mode) || mode >= SCENE_SLOTS ||
        !scene_selection(s, m, cpu, SCENE_TABLE + 4u * mode, &s->target, &s->service))
        return 0;
    s->phase = SCENE_PARENT_ISSUE;
    scene_remember(s, cpu);
    return 1;
}

static enum SceneWitness scene_before_issue(LaneRunFrame *f, MusashiBootMemory *m,
                                            FormatterCpu *cpu, uint32_t word) {
    SceneDispatchFrame *s = &f->scene;
    uint32_t pc = cpu->pc;
    if (pc == SCENE_PARENT_CALL) {
        s->phase = SCENE_IDLE;
        if (word == MIPS_JAL(SCENE_ENTRY) && scene_bind_parent(f, m, cpu)) return SCENE_PARENT_JAL;
    } else if (s->phase == SCENE_PARENT_DELAY) {
        if (!word && scene_flow(cpu, SCENE_PARENT_CALL + 4u, SCENE_PARENT_CALL, SCENE_ENTRY) &&
            scene_registers(s, cpu)) return SCENE_PARENT_SLOT;
    } else if (s->phase == SCENE_WRAPPER) {
        if (scene_body_owned(f, m, cpu) && scene_access_owned(f, m, cpu, word))
            return SCENE_WRAPPER_WORD;
    } else if (s->phase == SCENE_SERVICE || s->phase == SCENE_SERVICE_ENTRY) {
        if (s->phase == SCENE_SERVICE_ENTRY) {
            if (!scene_flow(cpu, s->target, 0u, s->target + 4u) || !scene_registers(s, cpu)) {
                s->phase = SCENE_IDLE;
                return SCENE_NO_WITNESS;
            }
            if (word != MIPS_JR(31)) return SCENE_CHILD_ENTRY;
        }
        if (pc - SCENE_ENTRY >= SCENE_LENGTH) {
            if (cpu->r[31] != SCENE_RESUME || cpu->npc != pc + 4u || cpu->delay_slot ||
                cpu->branch_pc || word != MIPS_JR(31)) return SCENE_NO_WITNESS;
            if (scene_root(f, m, cpu) && scene_registration(pc) == s->service && scene_registration(pc + 4u) == s->service &&
                intrinsic_span(cpu, pc, 8u) && s->owner->resident_matches(s->owner, pc, 8u)) {
                s->next = pc;
                scene_remember(s, cpu);
                return SCENE_CHILD_JR;
            }
        }
    } else if (s->phase == SCENE_SERVICE_DELAY &&
               scene_flow(cpu, s->next, s->next - 4u, SCENE_RESUME) && scene_registers(s, cpu)) {
        return SCENE_CHILD_SLOT;
    }
    s->phase = SCENE_IDLE;
    return SCENE_NO_WITNESS;
}

/* The data bus and device clock may run other work between issue and
 * retirement. Recheck before treating an ordinary retirement as evidence. */
static int scene_witness_owned(const LaneRunFrame *f, MusashiBootMemory *m,
                                    const FormatterCpu *cpu, enum SceneWitness w) {
    const SceneDispatchFrame *s = &f->scene;
    if (!scene_root(f, m, cpu) || !scene_registers(s, cpu)) return 0;
    switch (w) {
    case SCENE_PARENT_JAL:
        return s->phase == SCENE_PARENT_ISSUE &&
            scene_flow(cpu, SCENE_PARENT_CALL, 0u, SCENE_PARENT_CALL + 4u);
    case SCENE_PARENT_SLOT:
        return s->phase == SCENE_PARENT_DELAY &&
            scene_flow(cpu, SCENE_PARENT_CALL + 4u, SCENE_PARENT_CALL, SCENE_ENTRY);
    case SCENE_WRAPPER_WORD: return scene_body_owned(f, m, cpu);
    case SCENE_CHILD_ENTRY:
        return s->phase == SCENE_SERVICE_ENTRY && scene_flow(cpu, s->target, 0u, s->target + 4u) &&
            scene_selected(s, m, cpu) && scene_saved(s, m, 4u);
    case SCENE_CHILD_JR: case SCENE_CHILD_SLOT: {
        const int delay = w == SCENE_CHILD_SLOT;
        uint32_t jr = delay ? s->next - 4u : s->next;
        return (delay ? s->phase == SCENE_SERVICE_DELAY :
                (s->phase == SCENE_SERVICE || s->phase == SCENE_SERVICE_ENTRY)) &&
            scene_flow(cpu, s->next, delay ? jr : 0u, delay ? SCENE_RESUME : jr + 4u) &&
            intrinsic_span(cpu, jr, 8u) && s->owner->resident_matches(s->owner, jr, 8u) &&
            scene_saved(s, m, 4u) && scene_selected(s, m, cpu);
    }
    default: return 0;
    }
}

/* Keep the child's call and return tied to the same mounted image and CPU
 * edge across the device charge, whichever implementation retires it. */
static enum SceneWitness scene_after_charge(LaneRunFrame *f, MusashiBootMemory *m,
                                             const FormatterCpu *cpu, enum SceneWitness witness) {
    if (witness && !scene_witness_owned(f, m, cpu, witness)) {
        f->scene.phase = SCENE_IDLE;
        return SCENE_NO_WITNESS;
    }
    return witness;
}

static void scene_retired(LaneRunFrame *f, MusashiBootMemory *m, FormatterCpu *cpu,
                                 enum SceneWitness w, uint32_t pc) {
    SceneDispatchFrame *s = &f->scene;
    if (w == SCENE_PARENT_JAL) {
        if (scene_flow(cpu, pc + 4u, pc, SCENE_ENTRY) && cpu->r[31] == SCENE_PARENT_RESUME) {
            s->phase = SCENE_PARENT_DELAY;
            scene_remember(s, cpu);
        } else s->phase = SCENE_IDLE;
        return;
    }
    if (w == SCENE_CHILD_ENTRY) {
        s->phase = SCENE_SERVICE;
        return;
    }
    if (w == SCENE_CHILD_JR) {
        if (scene_flow(cpu, pc + 4u, pc, SCENE_RESUME)) {
            s->next = cpu->pc;
            s->phase = SCENE_SERVICE_DELAY;
            scene_remember(s, cpu);
        } else s->phase = SCENE_IDLE;
        return;
    }
    int ok = 0;
    if (w == SCENE_PARENT_SLOT) {
        ok = scene_flow(cpu, SCENE_ENTRY, 0u, SCENE_ENTRY + 4u) &&
            scene_registers(s, cpu) && cpu->r[31] == SCENE_PARENT_RESUME;
    } else if (w == SCENE_CHILD_SLOT) {
        ok = scene_flow(cpu, SCENE_RESUME, 0u, SCENE_RESUME + 4u) &&
            cpu->r[31] == SCENE_RESUME && cpu->r[29] == s->top - 32u &&
            cpu->r[30] == cpu->r[29] && cpu->r[16] == SCENE_STATE && cpu->r[17] == s->target &&
            scene_saved(s, m, 4u);
        if (ok) ++g_scene_child_returns;
    } else if (w == SCENE_WRAPPER_WORD) {
        ++g_scene_scalar_issues;
        if (pc == SCENE_INDIRECT_CALL + 4u) {
            ok = scene_flow(cpu, s->target, 0u, s->target + 4u) && cpu->r[31] == SCENE_RESUME &&
                scene_selected(s, m, cpu) && scene_saved(s, m, 4u);
            s->phase = ok ? SCENE_SERVICE_ENTRY : SCENE_IDLE;
            if (ok) {
                ++g_scene_calls;
                s->next = cpu->pc;
                scene_remember(s, cpu);
            }
            return;
        }
        if (pc == SCENE_JR + 4u) {
            if (scene_flow(cpu, SCENE_PARENT_RESUME, 0u, SCENE_PARENT_RESUME + 4u) &&
                cpu->r[29] == s->top && cpu->r[31] == s->parent[0] && cpu->r[30] == s->parent[1] &&
                cpu->r[17] == s->parent[2] && cpu->r[16] == s->parent[3])
                ++g_scene_returns;
            s->phase = SCENE_IDLE;
            return;
        }
        ok = cpu->pc == pc + 4u;
    }
    s->phase = ok ? SCENE_WRAPPER : SCENE_IDLE;
    s->next = cpu->pc;
    if (ok) scene_remember(s, cpu);
}
