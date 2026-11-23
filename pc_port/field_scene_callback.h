/* Resident field callback. Its service address is overlaid: only the regular
 * CPU or the currently installed lane provider may execute the callee. */
enum {
    FIELD_SCENE_CALLER = 0x80010b8cu, FIELD_SCENE_ENTRY = 0x8001125cu,
    FIELD_SCENE_SERVICE_CALL = FIELD_SCENE_ENTRY + 40u,
    FIELD_SCENE_CONTINUE = FIELD_SCENE_SERVICE_CALL + 8u,
    FIELD_SCENE_RETURN = FIELD_SCENE_ENTRY + 68u,
    FIELD_SCENE_SERVICE = 0x800cedfcu, FIELD_SCENE_STATE = 0x800af630u,
    FIELD_SCENE_FLAG = FIELD_SCENE_STATE + 0xa434u
};

static int field_scene_reserved(uint32_t pc) {
    return lane_find(pc) || lane_ovl_first(pc) != g_lane_ovl_count;
}

static int field_scene_bus(const FormatterCpu *cpu) {
    if (!cpu->bios_image) return 1;
#ifdef MUSASHI_LINKED_BIOS_EXCEPTION
    return cpu->bios_image->intersects == musashi_bios_exception_intersects;
#else
    return 0;
#endif
}

static void field_scene_capture(LaneRunFrame *frame, MusashiBootMemory *memory, FormatterCpu *cpu) {
    FieldScene *field = &frame->field_scene;
    if (cpu->pc != CODE_IMAGE_BASE || !game_main_owner(frame, memory, cpu) || !field_scene_bus(cpu))
        return;
    /* game_main_capture authenticated this cold root's entire mounted image. */
    const uint8_t *caller = musashi_boot_ram_span(memory, FIELD_SCENE_CALLER, sizeof field->caller);
    const uint8_t *body = musashi_boot_ram_span(memory, FIELD_SCENE_ENTRY, sizeof field->body);
    if (!caller || !body) return;
    memcpy(field->caller, caller, sizeof field->caller);
    memcpy(field->body, body, sizeof field->body);
    field->mounted = 1;
}

static int field_scene_owner(LaneRunFrame *frame, MusashiBootMemory *memory, FormatterCpu *cpu) {
    FieldScene *field = &frame->field_scene;
    if (!field->mounted || !game_main_owner(frame, memory, cpu) || !field_scene_bus(cpu) ||
        cpu->title_mixer || cpu->movie_output || cpu->main12_callback_frame ||
        cpu->main12_foreground_owner || main12_owns_fetch(cpu) || main12_service_pc(cpu) ||
        main12_payload_copy_pc(cpu) || main12_copy_pc(cpu) || main12_menu_exit_pc(cpu) ||
        room_latch_active(cpu) || room_scene_active(cpu) || room_reset_active(cpu) ||
        !intrinsic_span(cpu, FIELD_SCENE_CALLER, sizeof field->caller) ||
        !intrinsic_span(cpu, FIELD_SCENE_ENTRY, sizeof field->body) ||
        !intrinsic_span(cpu, field->stack, 32u) || !intrinsic_span(cpu, FIELD_SCENE_FLAG, 1u))
        return 0;
    const uint8_t *caller = musashi_boot_ram_span(memory, FIELD_SCENE_CALLER, sizeof field->caller);
    const uint8_t *body = musashi_boot_ram_span(memory, FIELD_SCENE_ENTRY, sizeof field->body);
    return caller && body && !memcmp(caller, field->caller, sizeof field->caller) &&
        !memcmp(body, field->body, sizeof field->body);
}

static void field_scene_follow(FieldScene *field, const FormatterCpu *cpu, FieldScenePhase phase) {
    field->phase = phase;
    field->pc = cpu->pc;
    field->npc = cpu->npc;
    field->delay = cpu->delay_slot;
    field->branch = cpu->branch_pc;
}

static int field_scene_service_frame(const FieldScene *field, const FormatterCpu *cpu) {
    return cpu->r[29] == field->stack && cpu->r[30] == field->stack &&
        cpu->r[16] == FIELD_SCENE_STATE && cpu->r[17] == FIELD_SCENE_ENTRY &&
        cpu->r[31] == FIELD_SCENE_CONTINUE;
}

/* Admit the real indirect call by its live destination. No scene table or
 * selector is needed. A direct entry, copied frame or jump to a continuation
 * has no preceding retirement. */
static void field_scene_before(LaneRunFrame *frame, MusashiBootMemory *memory, FormatterCpu *cpu) {
    FieldScene *field = &frame->field_scene;
    if (field->phase != FIELD_IDLE && (frame->cpu != cpu || !field_scene_expected(field, cpu)))
        field->phase = FIELD_IDLE;
    if (field->phase == FIELD_SERVICE_ENTRY &&
        (!field_scene_service_frame(field, cpu) || !field_scene_owner(frame, memory, cpu)))
        field->phase = FIELD_IDLE;
    /* A previously delegated restore must pass admission again on retry. */
    if (field->phase == FIELD_RESTORING) field->phase = FIELD_BODY;
    if (field->phase != FIELD_IDLE || cpu->pc != FIELD_SCENE_CALLER ||
        cpu->npc != cpu->pc + 4u || cpu->delay_slot || cpu->branch_pc ||
        cpu->r[17] != FIELD_SCENE_ENTRY || (cpu->r[29] & 3u)) return;
    field->stack = cpu->r[29] - 32u;
    if (!musashi_boot_ram_span(memory, field->stack, 32u) || !field_scene_owner(frame, memory, cpu))
        return;
    for (unsigned i = 0; i < sizeof field->body; i += 4u)
        if (field_scene_reserved(FIELD_SCENE_ENTRY + i)) return;
    if (!field_scene_reserved(FIELD_SCENE_CALLER) && !field_scene_reserved(FIELD_SCENE_CALLER + 4u)) {
        field->restores = 0;
        field_scene_follow(field, cpu, FIELD_CALLING);
    }
}

static int field_scene_service_return(LaneRunFrame *frame, MusashiBootMemory *memory,
                                      FormatterCpu *cpu) {
    return cpu->pc == FIELD_SCENE_CONTINUE && cpu->npc == cpu->pc + 4u &&
        !cpu->delay_slot && !cpu->branch_pc && field_scene_service_frame(&frame->field_scene, cpu) &&
        field_scene_owner(frame, memory, cpu);
}

static int field_scene_restoring(uint32_t pc) {
    return pc >= FIELD_SCENE_CONTINUE + 4u && pc < FIELD_SCENE_RETURN;
}

/* Count a restore delegated to a scheduler without the IRQ-load contract
 * only after the actual ordinary instruction retires. */
static void field_scene_retired(LaneRunFrame *frame, MusashiBootMemory *memory, FormatterCpu *cpu,
                                uint32_t issued, uint32_t instruction) {
    FieldScene *field = &frame->field_scene;
    FieldScenePhase phase = field->phase;
    if (frame->cpu != cpu || field->pc != issued) return;
    if (phase == FIELD_SERVING || phase == FIELD_SERVICE_ENTRY) {
        if (phase == FIELD_SERVICE_ENTRY && !field_scene_owner(frame, memory, cpu)) {
            field->phase = FIELD_IDLE;
            return;
        }
        if (instruction == MIPS_JR(31) && cpu->r[31] == FIELD_SCENE_CONTINUE &&
            cpu->pc == issued + 4u && cpu->npc == FIELD_SCENE_CONTINUE &&
            cpu->delay_slot && cpu->branch_pc == issued)
            field_scene_follow(field, cpu, FIELD_SERVICE_DELAY);
        else if (cpu->pc == FIELD_SCENE_CONTINUE) field->phase = FIELD_IDLE;
        else field_scene_follow(field, cpu, FIELD_SERVING);
        return;
    }
    if (phase == FIELD_SERVICE_DELAY) {
        if (field_scene_service_return(frame, memory, cpu)) {
            ++field->counters.guest_returns;
            field_scene_follow(field, cpu, FIELD_BODY);
        } else field->phase = FIELD_IDLE;
        return;
    }
    if (phase == FIELD_CALLING) {
        int call = issued == FIELD_SCENE_CALLER;
        int valid = field_scene_owner(frame, memory, cpu) &&
            !field_scene_reserved(issued) && cpu->r[31] == FIELD_SCENE_CALLER + 8u &&
            cpu->r[17] == FIELD_SCENE_ENTRY && cpu->r[29] == field->stack + 32u &&
            cpu->pc == (call ? issued + 4u : FIELD_SCENE_ENTRY) &&
            cpu->npc == (call ? FIELD_SCENE_ENTRY : FIELD_SCENE_ENTRY + 4u) &&
            cpu->delay_slot == (unsigned)call && cpu->branch_pc == (call ? issued : 0u);
        if (valid) field_scene_follow(field, cpu, call ? FIELD_CALLING : FIELD_BODY);
        else field->phase = FIELD_IDLE;
        return;
    }
    if (phase == FIELD_RESTORING) {
        ++field->counters.delegated_issues;
        field->counters.delegated_clocks += issued == FIELD_SCENE_RETURN - 4u ? 2u : 3u;
        if (cpu->pc == issued + 4u && cpu->npc == issued + 8u &&
            !cpu->delay_slot && !cpu->branch_pc && field_scene_owner(frame, memory, cpu))
            field_scene_follow(field, cpu, FIELD_BODY);
        else field->phase = FIELD_IDLE;
    }
}

static void field_scene_lane_retired(LaneRunFrame *frame, MusashiBootMemory *memory,
                                     FormatterCpu *cpu, uint32_t issued) {
    FieldScene *field = &frame->field_scene;
    if (field->pc != issued || frame->cpu != cpu) return;
    if (field->phase == FIELD_SERVICE_ENTRY && issued == FIELD_SCENE_SERVICE &&
        field_scene_service_return(frame, memory, cpu)) {
        ++field->counters.native_returns;
        field_scene_follow(field, cpu, FIELD_BODY);
    } else if (field->phase == FIELD_SERVING && cpu->pc != FIELD_SCENE_CONTINUE)
        field_scene_follow(field, cpu, FIELD_SERVING);
    else field->phase = FIELD_IDLE;
}

/* Existing scheduler services can complete a guest call outside formatter_step.
 * Follow their actual outgoing edge within the opaque callee only. The return
 * to the resident continuation still requires its JR/delay or exact bound lane. */
static void field_scene_scheduler_retired(FieldScene *field, const FormatterCpu *cpu, uint32_t issued) {
    if (issued && field->phase == FIELD_SERVING && field->pc == issued &&
        !field_scene_expected(field, cpu)) {
        if (cpu->pc == FIELD_SCENE_CONTINUE) field->phase = FIELD_IDLE;
        else field_scene_follow(field, cpu, FIELD_SERVING);
    }
}

static int field_scene_frame(const FieldScene *field, const FormatterCpu *cpu) {
    uint32_t pc = cpu->pc;
    if (pc - FIELD_SCENE_ENTRY >= sizeof field->body || (pc & 3u) ||
        !field_scene_expected(field, cpu) || cpu->r[17] != FIELD_SCENE_ENTRY) return 0;
    uint32_t sp = pc == FIELD_SCENE_ENTRY || pc >= FIELD_SCENE_RETURN
                ? field->stack + 32u : field->stack;
    if (cpu->r[29] != sp) return 0;
    if (pc > FIELD_SCENE_ENTRY + 16u && pc <= FIELD_SCENE_CONTINUE + 8u &&
        cpu->r[30] != field->stack) return 0;
    if (pc > FIELD_SCENE_ENTRY + 24u && pc <= FIELD_SCENE_CONTINUE + 12u &&
        cpu->r[16] != FIELD_SCENE_STATE) return 0;
    if (pc == FIELD_SCENE_ENTRY + 36u && cpu->r[1] - 0x5bccu != FIELD_SCENE_FLAG) return 0;
    if (pc <= FIELD_SCENE_SERVICE_CALL || pc >= FIELD_SCENE_RETURN)
        return cpu->r[31] == FIELD_SCENE_CALLER + 8u;
    if (pc <= FIELD_SCENE_CONTINUE + 4u) return cpu->r[31] == FIELD_SCENE_CONTINUE;
    return 1;
}

/* IRQ workers have independent CPUs. A checkpoint that moves this CPU into
 * another host service hands it the complete register bank before that
 * service runs. A parked CPU has not issued a successor. */
static void field_scene_changed_owner(LaneRunFrame *frame, FormatterCpu *cpu) {
    if (!field_scene_pending(cpu) || cpu->pc == UINT32_MAX) return;
    FieldSceneLoad *load = &cpu->field_load;
    if (cpu->pc == load->successor && cpu->callback_device == load->device &&
        cpu->callback_device->field_restore_irq == load->irq &&
        cpu->callback_device->field_restore_quiet == load->quiet &&
        cpu->callback_device->checkpoint == load->checkpoint &&
        cpu->callback_device->userdata == load->worker &&
        cpu->r[load->reg] == load->old_value &&
        !cpu->registration_stack && !cpu->dma_registration && !cpu->bios_frame &&
        !cpu->cd_irq_frame && !cpu->irq_image && !cpu->cd_parent_image &&
        !cpu->title_mixer && !cpu->movie_output && !cpu->main12_row_pc &&
        !cpu->main12_clearing_file && !main12_owns_fetch(cpu) &&
        !room_latch_active(cpu) && !room_scene_active(cpu) && !room_reset_active(cpu)) return;
    field_scene_writeback(cpu, 0);
    if (frame && frame->cpu == cpu) frame->field_scene.phase = FIELD_IDLE;
}

int musashi_boot_field_load_snapshot(const void *continuation, MusashiFieldLoadSnapshot *out) {
    const FormatterCpu *cpu = continuation;
    if (!cpu || !out || !field_scene_pending(cpu)) return 0;
    const FieldSceneLoad *load = &cpu->field_load;
    *out = (MusashiFieldLoadSnapshot){
        .origin = load->successor - 4u, .successor = load->successor, .reg = load->reg,
        .old_value = load->old_value, .value = load->value, .visible = cpu->r[load->reg]
    };
    return 1;
}

/* The IRQ owner may take the ordinary bank if both possible visible values
 * agree. A differing pending load required proof that this checkpoint could
 * not deliver. A contradictory request cannot supply a field IRQ context. */
int musashi_boot_field_irq_context(const void *continuation, MusashiCpuContext *out,
                                   uint32_t gpr[32]) {
    FormatterCpu *cpu = (FormatterCpu *)continuation;
    MusashiCpuContext context;
    if (!gpr || !out || !musashi_boot_cpu_context(cpu, MUSASHI_CPU_CONTEXT_SOURCE, &context)) return 0;
    if (field_scene_pending(cpu)) {
        FieldSceneLoad *load = &cpu->field_load;
        if (cpu->pc != load->successor || cpu->callback_device != load->device ||
            cpu->callback_device->field_restore_irq != load->irq ||
            cpu->callback_device->field_restore_quiet != load->quiet || load->value != load->old_value ||
            cpu->callback_device->checkpoint != load->checkpoint ||
            cpu->callback_device->userdata != load->worker || cpu->r[load->reg] != load->old_value)
            return 0;
        memcpy(gpr, cpu->r, sizeof cpu->r);
        context.gpr = gpr;
        load->irq_instruction = context.instruction;
        load->irq_prepared = 1;
    }
    *out = context;
    return 1;
}

int musashi_boot_accept_field_irq(const void *continuation, const MusashiCpuExceptionToken *token) {
    FormatterCpu *cpu = (FormatterCpu *)continuation;
    if (!cpu) return 0;
    if (!field_scene_pending(cpu)) return 1;
    FieldSceneLoad *load = &cpu->field_load;
    MusashiCpuStatusSnapshot snapshot;
    if (!load->irq_prepared || !token || token->identity != cpu || !token->sequence ||
        !token->owner || cpu->pc != load->successor || cpu->callback_device != load->device ||
        cpu->callback_device->field_restore_irq != load->irq ||
        cpu->callback_device->field_restore_quiet != load->quiet || load->value != load->old_value ||
        cpu->callback_device->checkpoint != load->checkpoint ||
        cpu->callback_device->userdata != load->worker || cpu->r[load->reg] != load->old_value ||
        !load->irq(load->worker, token, &snapshot) ||
        !snapshot.active || snapshot.faulted || snapshot.kernel_entered ||
        snapshot.identity != cpu || snapshot.sequence != token->sequence ||
        snapshot.kind != MUSASHI_CPU_EXCEPTION_IRQ || snapshot.provenance != MUSASHI_CPU_CONTEXT_SOURCE ||
        !snapshot.instruction_valid || !snapshot.cause_epc_valid || snapshot.pc != cpu->pc ||
        snapshot.npc != cpu->npc || snapshot.instruction != load->irq_instruction ||
        snapshot.delay_slot != cpu->delay_slot || snapshot.branch_pc != cpu->branch_pc ||
        snapshot.epc != (cpu->delay_slot ? cpu->branch_pc : cpu->pc) ||
        snapshot.cause != (0x400u | (cpu->delay_slot ? 0x80000000u : 0u))) return 0;
    for (unsigned i = 0; i < 32u; ++i)
        if (snapshot.gpr[i] != cpu->r[i]) return 0;
    field_scene_writeback(cpu, 0);
    if (g_lane_run_frames && g_lane_run_frames->cpu == cpu)
        ++g_lane_run_frames->field_scene.counters.irq_writebacks;
    return 1;
}

/* A paid or unowned successor still reads the old GPR when the ordinary
 * scalar decoder executes it. Its own explicit write takes precedence even
 * when that write reproduces the old value. Only these three field loads
 * can reach this helper. No other scalar instruction creates a delay here. */
static void field_scene_scalar_successor(FormatterCpu *cpu, uint32_t word) {
    if (!field_scene_pending(cpu)) return;
    unsigned op = word >> 26, fn = word & 63u, written = 0;
    if (!op) {
        if (fn <= 7u || fn == 9u || fn == 16u || fn == 18u || (fn >= 32u && fn <= 43u))
            written = (word >> 11) & 31u;
    } else if (op == 3u) written = 31u;
    else if ((op >= 8u && op <= 15u) || (op >= 32u && op <= 37u && op != 34u) ||
             (op == 16u && ((word >> 21) & 31u) == 0u))
        written = (word >> 16) & 31u;
    field_scene_writeback(cpu, written);
}

static int field_scene_restore_available(const FormatterCpu *cpu) {
    if (!cpu->callback_device->field_restore_irq) return 0;
    for (uint32_t pc = cpu->pc; pc < FIELD_SCENE_RETURN; pc += 4u)
        if (field_scene_reserved(pc)) return 0;
    return 1;
}

static int field_scene_restore(MusashiBootMemory *memory, FormatterCpu *cpu, uint32_t pc) {
    if (pc == FIELD_SCENE_RETURN - 4u) {
        cpu->r[29] += 32u;
        field_scene_writeback(cpu, 0);
        return 1;
    }
    unsigned offset = pc - (FIELD_SCENE_CONTINUE + 4u);
    unsigned reg = offset == 0u ? 31u : offset == 4u ? 30u : 16u;
    uint32_t value;
    /* Admission already authenticated the entire ordinary RAM stack frame.
     * A declined load goes through the scalar bus using its existing charge. */
    if (!musashi_boot_read32(memory, cpu->r[29] + 24u - offset, &value)) return -1;
    if (value != cpu->r[reg] && (!cpu->callback_device->field_restore_quiet ||
        !cpu->callback_device->field_restore_quiet(cpu->callback_device->userdata, cpu))) return 0;
    field_scene_writeback(cpu, 0);
    cpu->field_load = (FieldSceneLoad){
        .owner = cpu, .device = cpu->callback_device, .irq = cpu->callback_device->field_restore_irq,
        .quiet = cpu->callback_device->field_restore_quiet,
        .worker = cpu->callback_device->userdata, .checkpoint = cpu->callback_device->checkpoint,
        .successor = pc + 4u, .value = value, .old_value = cpu->r[reg], .reg = reg
    };
    return 1;
}

static int field_scene_owned_issue(LaneRunFrame *frame, MusashiBootMemory *memory, FormatterCpu *cpu) {
    return frame->field_scene.phase == FIELD_BODY && field_scene_frame(&frame->field_scene, cpu) &&
        field_scene_owner(frame, memory, cpu) && !field_scene_reserved(cpu->pc);
}

static int field_scene_issue(LaneRunFrame *frame, MusashiBootMemory *memory, FormatterCpu *cpu,
                             uint32_t instruction, int *paid) {
    FieldScene *field = &frame->field_scene;
    uint32_t pc = cpu->pc;
    if (!field_scene_owned_issue(frame, memory, cpu)) {
        field->phase = FIELD_IDLE;
        return 0;
    }
    const int restoring = field_scene_restoring(pc);
    if (restoring && !field_scene_restore_available(cpu)) {
        field->phase = FIELD_RESTORING;
        return 0;
    }
    unsigned offset = pc - FIELD_SCENE_ENTRY;
    int save = offset >= 4u && offset <= 12u;
    unsigned cost = save || offset == 36u || (restoring && pc < FIELD_SCENE_RETURN - 4u) ? 3u : 2u;
    enum SceneWitness witness = SCENE_NO_WITNESS;
    if (frame->scene.phase != SCENE_IDLE)
        witness = scene_before_issue(frame, memory, cpu, instruction);
    uint32_t registers[32];
    memcpy(registers, cpu->r, sizeof registers);
    if (!cpu->clock->advance(cpu->clock->userdata, cost) ||
        (cpu->cd_irq_frame && cpu->cd_irq_frame->faulted)) {
        if (!restoring) field->phase = FIELD_IDLE;
        return -1;
    }
    *paid = 1;
    witness = scene_after_charge(frame, memory, cpu, witness);
    if (cpu->pc != pc || !field_scene_owned_issue(frame, memory, cpu) ||
        memcmp(registers, cpu->r, sizeof registers) || (restoring && !field_scene_restore_available(cpu))) {
        field->phase = FIELD_IDLE;
        if (witness) frame->scene.phase = SCENE_IDLE;
        return 0; /* The ordinary CPU retires its original fetched word once. */
    }
    if (restoring) {
        int restored = field_scene_restore(memory, cpu, pc);
        if (restored < 0) return -1;
        if (!restored) {
            field->phase = FIELD_RESTORING;
            return 0;
        }
        ++field->restores;
        ++field->counters.restore_issues;
        field->counters.restore_clocks += cost;
    } else if (save) {
        const unsigned reg = offset == 4u ? 31u : offset == 8u ? 30u : 16u;
        if (!cpu_write32(memory, cpu, field->stack + 28u - offset, cpu->r[reg])) return -1;
    } else switch (offset) {
        case 0u: cpu->r[29] -= 32u; break;
        case 16u: cpu->r[30] = cpu->r[29]; break;
        case 20u: cpu->r[16] = (FIELD_SCENE_STATE + 0x8000u) & 0xffff0000u; break;
        case 24u: cpu->r[16] -= 0x9d0u; break;
        case 28u: cpu->r[1] = 0x10000u; break;
        case 32u: cpu->r[1] += cpu->r[16]; break;
        case 36u:
            if (!cpu_write8(memory, cpu, FIELD_SCENE_FLAG, 0u)) return -1;
            ++field->counters.flag_clears;
            break;
        case 40u:
            cpu->r[31] = FIELD_SCENE_CONTINUE;
            if (g_trace_state >= 0) musashi_trace_function_entry(FIELD_SCENE_SERVICE, pc, 'j');
            if (g_entry_hook) g_entry_pending = FIELD_SCENE_SERVICE;
            break;
        case 48u: cpu->r[29] = cpu->r[30]; break;
    }
    ++field->counters.issues;
    field->counters.clocks += cost;
    cpu->pc = cpu->npc;
    cpu->npc = pc == FIELD_SCENE_SERVICE_CALL ? FIELD_SCENE_SERVICE :
               pc == FIELD_SCENE_RETURN ? cpu->r[31] : cpu->pc + 4u;
    cpu->delay_slot = pc == FIELD_SCENE_SERVICE_CALL || pc == FIELD_SCENE_RETURN;
    cpu->branch_pc = cpu->delay_slot ? pc : 0;
    if (pc == FIELD_SCENE_SERVICE_CALL + 4u) {
        ++field->counters.calls;
        field_scene_follow(field, cpu, FIELD_SERVICE_ENTRY);
    } else if (pc == FIELD_SCENE_RETURN + 4u) {
        ++field->counters.completed;
        if (field->restores == 4u) ++field->counters.complete_19;
        field->phase = FIELD_IDLE;
    } else field_scene_follow(field, cpu, FIELD_BODY);
    if (witness) scene_retired(frame, memory, cpu, witness, pc);
    return 1;
}

static void field_scene_report(const FieldScene *field) {
    const FieldSceneCounters *count = &field->counters;
    if (count->issues)
        fprintf(stderr, "native_boot: SCENE_FIELD_CALLBACK issues=%llu clocks=%llu delegated_issues=%llu delegated_clocks=%llu flag_clears=%llu calls=%llu guest_returns=%llu native_returns=%llu completed=%llu restore_issues=%llu restore_clocks=%llu complete_19=%llu irq_writebacks=%llu provider_handoffs=%llu\n",
                (unsigned long long)count->issues, (unsigned long long)count->clocks,
                (unsigned long long)count->delegated_issues, (unsigned long long)count->delegated_clocks,
                (unsigned long long)count->flag_clears, (unsigned long long)count->calls,
                (unsigned long long)count->guest_returns, (unsigned long long)count->native_returns,
                (unsigned long long)count->completed, (unsigned long long)count->restore_issues,
                (unsigned long long)count->restore_clocks, (unsigned long long)count->complete_19,
                (unsigned long long)count->irq_writebacks, (unsigned long long)count->provider_handoffs);
}
