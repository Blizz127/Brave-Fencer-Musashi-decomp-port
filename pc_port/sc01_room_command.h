/* SC01 members 5 and 6 have the same decoded image. The saved-room scene
 * selects its initialization and readiness commands through its live table. Follow that invocation across
 * its real calls and returns without selecting a whole overlay by address. */
enum {
    SC01_ROOM_CALL = 0x801282b4u, SC01_ROOM_COMMAND = 0x80128420u,
    SC01_ROOM_ENTRY = 0x80128158u, SC01_ROOM_TARGET = 0x8018264cu
};

typedef struct {
    uint32_t start, end;
    uint64_t digest;
} Sc01RoomCode;

static const Sc01RoomCode *sc01_room_code(uint32_t pc)
{
    static const Sc01RoomCode code[] = {
        { SC01_ROOM_COMMAND, 0x801284b8u, UINT64_C(0x2d02979f09bdb3a2) },
        { 0x80129220u, 0x80129248u, UINT64_C(0x57c76e8739f31e90) },
        { 0x80129248u, 0x80129258u, UINT64_C(0x28d2c2faa49a62be) },
        { 0x80129258u, 0x8012927cu, UINT64_C(0xb9cb72d7e1fd5c28) },
        { SCENE_SETUP, 0x801379d8u, UINT64_C(0xc2316711d9c58dfd) },
        { SCENE_CHILD, 0x8013a8b0u, UINT64_C(0xf7aa82fbce89e827) },
        { 0x80128c14u, 0x80128c98u, UINT64_C(0x4d7afab4eefb1f24) },
        { 0x8013c98cu, 0x8013c9c4u, UINT64_C(0x72efc34a017b847d) },
        { 0x80129c40u, 0x80129cf8u, UINT64_C(0xbc5ca01e22b4970c) },
        /* Scene program routines entered while initializing the saved room. */
        { 0x80129398u, 0x80129428u, UINT64_C(0x9987aebbbea85bd3) },
        { 0x8012944cu, 0x8012956cu, UINT64_C(0x71de85ccbdef664e) },
        { 0x801299c8u, 0x80129c40u, UINT64_C(0x4a81ae66209fa257) },
        { 0x8012a018u, 0x8012a048u, UINT64_C(0xbc150c0e797a4c73) },
        { 0x8012a048u, 0x8012a094u, UINT64_C(0xdac62e84e2b653b7) },
        { 0x8012a094u, 0x8012a0e0u, UINT64_C(0x0ef4d977cfacffd2) },
        { 0x8012f14cu, 0x8012f1a4u, UINT64_C(0x128d65fe65f8bf49) },
        { 0x801336e8u, 0x8013373cu, UINT64_C(0x35e98d4968d720d9) },
        { 0x80136bc4u, 0x80136c1cu, UINT64_C(0x5cd01ac0e30a0cc3) },
        { 0x8013b568u, 0x8013b598u, UINT64_C(0xb37960ea593c5341) },
        { 0x8013b83cu, 0x8013bc7cu, UINT64_C(0x9353953d373dbc3d) },
        { 0x8013c08cu, 0x8013c0f8u, UINT64_C(0xf219923ca71732b0) },
        { 0x8013c0f8u, 0x8013c360u, UINT64_C(0x4f388680e0f5de85) },
        { 0x8013d3d4u, 0x8013d53cu, UINT64_C(0x811f30904a708b8f) },
        { 0x8013d53cu, 0x8013d8fcu, UINT64_C(0xcaab2fe754316cbc) },
        { 0x80144b9cu, 0x801457a4u, UINT64_C(0x95642a09063f90e8) },
        { 0x801457a4u, 0x801458e0u, UINT64_C(0x000a7f17bb576752) },
        { 0x80145cecu, 0x80145ee8u, UINT64_C(0xa4f61ad9fb19051a) },
        { 0x80145ee8u, 0x80146014u, UINT64_C(0x8d914750e3ddfd56) },
        { 0x801468c8u, 0x80146924u, UINT64_C(0xdae192acb4e85493) },
        { 0x80147084u, 0x8014708cu, UINT64_C(0xe4d9bde2d06cdc3f) },
        { 0x80147098u, 0x801470a0u, UINT64_C(0x6aeb3dda90bab01a) },
        { 0x801470acu, 0x801470b4u, UINT64_C(0x84d4a9eb26a30590) },
        { 0x80149210u, 0x80149228u, UINT64_C(0xdd6dda789decad4d) },
        { 0x8014bb0cu, 0x8014bb24u, UINT64_C(0x4c52481718e9820d) },
        { 0x8014bd98u, 0x8014bdc8u, UINT64_C(0x941b51220ab98dc1) },
        { 0x8014be78u, 0x8014be9cu, UINT64_C(0x11a54949740cfe24) },
        { 0x8014be9cu, 0x8014bec0u, UINT64_C(0x411794daf0f6309d) },
        { 0x8014c6d0u, 0x8014c6e0u, UINT64_C(0x13a19f9a562a9e3e) },
        { 0x8014c968u, 0x8014c978u, UINT64_C(0xd64f0a30ae1c8526) },
        { 0x80153c8cu, 0x80153c9cu, UINT64_C(0x1584fed42b65ed32) },
        { 0x801552f4u, 0x80155344u, UINT64_C(0xcb53ae523b45a62d) },
        { 0x801627c0u, 0x801627e8u, UINT64_C(0x12353172190e76b9) },
        { 0x80162af4u, 0x80162b1cu, UINT64_C(0x1fdd96ddf65ac3a7) },
        { 0x8016533cu, 0x80165374u, UINT64_C(0x640a333454e2b750) },
        { 0x80165938u, 0x80165958u, UINT64_C(0xe5aac0c9be502629) },
        { 0x801659dcu, 0x80165a18u, UINT64_C(0xd05a5408a066e09c) },
        { 0x80165a18u, 0x80165a20u, UINT64_C(0x589026cfbec22f44) },
        { 0x80165c78u, 0x80165ca0u, UINT64_C(0x449d1b979509e3aa) },
        { 0x8017d220u, 0x8017d26cu, UINT64_C(0x31daf33aed9d4980) },
        { 0x8017d6e4u, 0x8017d7a4u, UINT64_C(0xbe34159d08b3ec7f) },
        { 0x8017d7a4u, 0x8017d7e0u, UINT64_C(0x9d81ee8ded4380ae) },
        { 0x8017d7e0u, 0x8017d808u, UINT64_C(0x33ef4c91a3cf178e) },
        { 0x8017d808u, 0x8017d9a0u, UINT64_C(0xd4ee503a2f339bcf) }
    };
    for (size_t i = 0; i < sizeof code / sizeof code[0]; ++i)
        if (pc >= code[i].start && pc < code[i].end) return &code[i];
    return NULL;
}

enum { SC01_REQUEST_CHECKING = 1, SC01_REQUEST_ADVANCING };
enum {
    SC01_REQUEST = 0x801284b8u, SC01_REQUEST_SELECTOR = 0x8017da84u,
    SC01_REQUEST_CALLBACK_RETURN = 0x800d0614u, SC01_REQUEST_ADVANCE_RETURN = 0x801284e0u
};

typedef struct {
    Sc01RoomCode code;
    uint32_t return_pc;
} Sc01RequestRoutine;

/* The two MAIN/0010 helpers call the installed room selector. These four
 * code extents belong to the live command-2 invocation only. Ordinary
 * resident services retain their existing device and CPU owners. */
static const Sc01RequestRoutine *sc01_request_routine(uint32_t pc)
{
    static const Sc01RequestRoutine routines[] = {
        { { SC01_REQUEST, 0x801284f0u, UINT64_C(0xf897c6c651fa071b) }, SC01_ROOM_CALL + 8u },
        { { 0x800d0588u, 0x800d05e8u, UINT64_C(0x1489043feeb50494) }, 0x801284c8u },
        { { 0x800d05e8u, 0x800d0630u, UINT64_C(0x86ae0ed24111ceec) }, 0x800d05a0u },
        { { SC01_REQUEST_SELECTOR, 0x8017da9cu, UINT64_C(0x67d67207e3386f21) }, SC01_REQUEST_CALLBACK_RETURN }
    };
    for (size_t i = 0; i < sizeof routines / sizeof routines[0]; ++i)
        if (pc >= routines[i].code.start && pc < routines[i].code.end) return &routines[i];
    return NULL;
}

static const Sc01RoomCode *sc01_frame_code(const Sc01RoomFrame *f, uint32_t pc)
{
    if (!f->request_phase) return sc01_room_code(pc);
    const Sc01RequestRoutine *routine = sc01_request_routine(pc);
    return routine ? &routine->code : NULL;
}

static int sc01_request_image(MusashiBootMemory *ram, const FormatterCpu *cpu, unsigned phase)
{
    uint32_t selector;
    uint16_t mode, room, ordinal;
    uint64_t main_prefix;
    return intrinsic_span(cpu, 0x800b99f0u, 8u) && musashi_boot_read16(ram, 0x800b99f0u, &mode) &&
           mode == 7u && musashi_boot_read16(ram, 0x800b99f6u, &ordinal) &&
           (ordinal == 2u || (phase == SC01_REQUEST_ADVANCING && ordinal == 3u)) &&
           intrinsic_span(cpu, 0x800b9a08u, 2u) && musashi_boot_read16(ram, 0x800b9a08u, &room) &&
           room == 0x3001u && intrinsic_span(cpu, 0x800d375cu, 4u) &&
           musashi_boot_read32(ram, 0x800d375cu, &selector) && selector == SC01_REQUEST_SELECTOR &&
           intrinsic_span(cpu, 0x800cedf8u, 128u) && span_digest(ram, 0x800cedf8u, 32u, &main_prefix) &&
           main_prefix == UINT64_C(0x1903d9f888849312);
}

static int sc01_request_access(const FormatterCpu *cpu, uint32_t word)
{
    unsigned op = word >> 26u;
    uint32_t base = cpu->r[(word >> 21u) & 31u];
    if (!op && (word & 63u) == 9u)
        return cpu->pc + 8u == SC01_REQUEST_CALLBACK_RETURN && base == SC01_REQUEST_SELECTOR;
    if (!op && (word & 63u) == 8u) {
        const Sc01RequestRoutine *routine = sc01_request_routine(cpu->pc);
        return routine && base == routine->return_pc;
    }
    if (op < 32u) return 1;
    uint32_t address = base + (uint32_t)(int32_t)(int16_t)word;
    if (op == 35u || op == 43u) {
        uint32_t top = cpu->sc01_room.caller_sp;
        if (top >= 64u && !(address & 3u) && address - (top - 64u) <= 60u)
            return intrinsic_span(cpu, address, 4u);
    }
    return ((op == 33u || op == 37u) && address == 0x800b9a08u && intrinsic_span(cpu, address, 2u)) ||
           (op == 35u && address == 0x800d375cu && intrinsic_span(cpu, address, 4u)) ||
           (op == 43u && address == 0x8012704cu && intrinsic_span(cpu, address, 4u));
}

static int sc01_room_foreground(const FormatterCpu *cpu)
{
    return !cpu->irq_image && !cpu->cd_parent_image && !cpu->bios_frame &&
           !cpu->cd_irq_frame && !cpu->dma_registration && !cpu->registration_stack &&
           !cpu->movie_output && !cpu->title_mixer && !cpu->main12_callback_frame &&
           !cpu->main12_foreground_owner;
}

static int sc01_room_image(MusashiBootMemory *memory, const FormatterCpu *cpu)
{
    uint64_t prefix;
    uint32_t target;
    uint32_t slot = SC01_ROOM_TARGET + (cpu->sc01_room.request_phase ? 4u : 0u);
    uint32_t expected = cpu->sc01_room.request_phase ? SC01_REQUEST : SC01_ROOM_COMMAND;
    return intrinsic_span(cpu, SC01_ROOM_ENTRY, SC01_ROOM_COMMAND - SC01_ROOM_ENTRY) &&
           intrinsic_span(cpu, slot, sizeof target) &&
           span_digest(memory, SC01_ROOM_ENTRY,
                       (SC01_ROOM_COMMAND - SC01_ROOM_ENTRY) / 4u, &prefix) &&
           prefix == UINT64_C(0x4f4e44c8e1a9471a) &&
           musashi_boot_read32(memory, slot, &target) && target == expected;
}

static int sc01_room_expected(const FormatterCpu *cpu)
{
    const Sc01RoomFrame *f = &cpu->sc01_room;
    return f->next && !f->callee_return && cpu->pc == f->next && sc01_room_foreground(cpu);
}

static int sc01_room_position(const Sc01RoomFrame *f, const FormatterCpu *cpu)
{
    return f->next == cpu->pc && f->npc == cpu->npc && f->branch == cpu->branch_pc &&
           !!f->branch == !!cpu->delay_slot && f->sp == cpu->r[29] && f->ra == cpu->r[31];
}

static void sc01_room_remember(Sc01RoomFrame *f, const FormatterCpu *cpu)
{
    f->next = cpu->pc;
    f->npc = cpu->npc;
    f->branch = cpu->branch_pc;
    f->sp = cpu->r[29];
    f->ra = cpu->r[31];
}

/* The owned room initializer moves unaligned data between a scene table
 * and two work areas. Each partial access uses the ordinary CPU's aligned
 * read or read/modify/write and its normal delayed-load state. */
static int sc01_room_merge_kind(uint32_t opcode)
{
    switch (opcode) {
    case 34: return MERGE_LWL;
    case 38: return MERGE_LWR;
    case 42: return MERGE_SWL;
    case 46: return MERGE_SWR;
    default: return MERGE_NONE;
    }
}

static int sc01_room_merge_access(MusashiBootMemory *memory, const FormatterCpu *cpu,
                                  uint32_t address, int kind)
{
    const uint32_t word = address & ~3u;
    const int write = kind == MERGE_SWL || kind == MERGE_SWR;
    const int in_span = word - 0x801cded8u < 8u ||
        (write ? word - 0x801cdf78u < 8u : word - 0x80182ce8u < 12u);
    return in_span && intrinsic_span(cpu, word, 4u) &&
           musashi_boot_ram_span(memory, word, 4u) != NULL;
}

static int sc01_room_merge_pending(const FormatterCpu *cpu)
{
    const Sc01RoomMerge *pending = &cpu->sc01_room.merge;
    if (!pending->kind) return !cpu->merge_pending;
    return pending->reg < 32u && (pending->kind == MERGE_LWL || pending->kind == MERGE_LWR) &&
        cpu->merge_pending == 1u && cpu->merge_next_pc == cpu->pc &&
        cpu->merge_kind == pending->kind && cpu->merge_reg == pending->reg &&
        cpu->merge_value == pending->value && cpu->r[pending->reg] == pending->prior;
}

static void sc01_room_merge_retired(FormatterCpu *cpu)
{
    Sc01RoomMerge *pending = &cpu->sc01_room.merge;
    pending->kind = cpu->merge_pending ? cpu->merge_kind : MERGE_NONE;
    if (cpu->merge_pending) {
        pending->reg = cpu->merge_reg;
        pending->value = cpu->merge_value;
        pending->prior = cpu->r[cpu->merge_reg];
    }
}

static void sc01_room_end(Sc01RoomFrame *f)
{
    f->next = f->callee_return = f->return_jump = 0;
    f->request_phase = 0;
    f->merge.kind = MERGE_NONE;
}

/* Existing formatter services return atomically. This witness follows an
 * actual return from the expected service, including nested calls within it. */
static void sc01_room_service_return(FormatterCpu *cpu)
{
    Sc01RoomFrame *f = &cpu->sc01_room;
    if (!sc01_room_foreground(cpu)) return;
    if (!sc01_room_position(f, cpu) || cpu->r[29] > f->callee_sp) {
        sc01_room_end(f);
        return;
    }
    const uint32_t to = cpu->r[31];
    if (to == f->callee_return && cpu->r[29] == f->callee_sp)
        f->callee_return = 0;
    else if (sc01_frame_code(f, to)) {
        sc01_room_end(f);
        return;
    }
    f->next = to;
    f->npc = to + 4u;
    f->branch = f->return_jump = 0;
}

/* Original guest code can call further functions before its JR and delay
 * instruction. Keep observing the same CPU's actual issue sequence without
 * granting its callee access to any additional guest code. */
static void sc01_room_callee_retired(FormatterCpu *cpu, uint32_t issued, uint32_t word,
                                     uint32_t old_branch)
{
    Sc01RoomFrame *f = &cpu->sc01_room;
    if (cpu->pc == f->callee_return && f->return_jump && old_branch == f->return_jump &&
        cpu->r[31] == f->callee_return && cpu->r[29] == f->callee_sp &&
        !cpu->delay_slot && !cpu->branch_pc && cpu->npc == cpu->pc + 4u)
        f->callee_return = 0;
    else if (sc01_frame_code(f, cpu->pc) || cpu->r[29] > f->callee_sp) {
        sc01_room_end(f);
        return;
    }
    f->return_jump = word == MIPS_JR(31) && cpu->branch_pc == issued &&
                     cpu->npc == f->callee_return ? issued : 0;
    sc01_room_remember(f, cpu);
}

/* The entry wrapper calls the table dispatcher directly. Its saved return
 * stays in the dispatcher's frame while command 1 and its children run. */
static int sc01_room_caller(MusashiBootMemory *ram, const FormatterCpu *cpu, uint32_t sp)
{
    uint32_t resume;
    return sp <= UINT32_MAX - 20u && !(sp & 3u) && intrinsic_span(cpu, sp + 16u, 4u) &&
           musashi_boot_read32(ram, sp + 16u, &resume) && resume == SC01_ROOM_ENTRY + 16u;
}

static int sc01_room_code_matches(MusashiBootMemory *ram, const FormatterCpu *cpu,
                                  const Sc01RoomCode *code)
{
    uint64_t digest;
    return code && intrinsic_span(cpu, code->start, code->end - code->start) &&
           span_digest(ram, code->start, (code->end - code->start) / 4u, &digest) &&
           digest == code->digest;
}

/* Three resident geometry services are called from the saved room. A matching
 * return address alone can belong to a different overlay. Require the actual
 * command frame, live caller and callee images, and the retired direct call. */
static int sc01_room_geometry_owner(MusashiBootMemory *ram, const FormatterCpu *cpu)
{
    static const Sc01RoomCode geometry[] = {
        { 0x8004914cu, 0x8004917cu, UINT64_C(0xb8b645118a73e8b0) },
        { 0x800491acu, 0x800491ccu, UINT64_C(0x14d170b3b2922850) },
        { 0x8004945cu, 0x8004948cu, UINT64_C(0xc09cc2180769b058) }
    };
    const Sc01RoomFrame *f = &cpu->sc01_room;
    if (f->memory != ram || !sc01_room_foreground(cpu) || !sc01_room_position(f, cpu) ||
        !f->callee_return || cpu->r[31] != f->callee_return || cpu->r[29] != f->callee_sp)
        return 0;
    const uint32_t call = f->callee_return - 8u;
    const Sc01RoomCode *caller = sc01_room_code(call);
    if (!caller || caller->start != 0x8012f14cu) return 0;
    for (size_t i = 0; i < sizeof geometry / sizeof geometry[0]; ++i) {
        const Sc01RoomCode *service = &geometry[i];
        if (cpu->pc >= service->start && cpu->pc < service->end) {
            uint32_t link;
            return sc01_room_caller(ram, cpu, f->caller_sp) && sc01_room_image(ram, cpu) &&
                   sc01_room_code_matches(ram, cpu, caller) && sc01_room_code_matches(ram, cpu, service) &&
                   musashi_boot_read32(ram, call, &link) && link == MIPS_JAL(service->start);
        }
    }
    return 0;
}

static int sc01_room_word(MusashiBootMemory *memory, const FormatterCpu *cpu, uint32_t *out)
{
    const Sc01RoomFrame *f = &cpu->sc01_room;
    if (f->memory != memory || !sc01_room_position(f, cpu) ||
        !sc01_room_caller(memory, cpu, f->caller_sp) || !sc01_room_image(memory, cpu) ||
        (f->request_phase && !sc01_request_image(memory, cpu, f->request_phase))) return 0;
    if (cpu->pc != SC01_ROOM_CALL + 4u && !sc01_room_code_matches(memory, cpu, sc01_frame_code(f, cpu->pc)))
        return 0;
    return musashi_boot_read32(memory, cpu->pc, out) &&
           (!f->request_phase || sc01_request_access(cpu, *out));
}

/* Only a successfully retired table JALR arms the frame. For a nested call
 * handled by the existing formatter, wait for the saved stack and its actual
 * return PC. The owning CPU carries this state over scheduler checkpoints. */
static void sc01_room_retired(MusashiBootMemory *memory, FormatterCpu *cpu,
                              uint32_t issued, int issuing, uint32_t old_branch)
{
    Sc01RoomFrame *f = &cpu->sc01_room;
    if (issued == SC01_ROOM_CALL && sc01_room_foreground(cpu)) {
        uint16_t command;
        sc01_room_end(f);
        if (cpu->pc != SC01_ROOM_CALL + 4u || cpu->npc != cpu->r[2] ||
            cpu->branch_pc != SC01_ROOM_CALL || !cpu->delay_slot ||
            cpu->r[31] != SC01_ROOM_CALL + 8u ||
            !intrinsic_span(cpu, 0x800b99f6u, 2u) ||
            !musashi_boot_read16(memory, 0x800b99f6u, &command) || command != cpu->r[3] ||
            !sc01_room_caller(memory, cpu, cpu->r[29])) return;
        if (command == 2u && cpu->npc == SC01_REQUEST &&
            sc01_request_image(memory, cpu, SC01_REQUEST_CHECKING)) {
            f->request_phase = SC01_REQUEST_CHECKING;
        } else if (command != 1u || cpu->npc != SC01_ROOM_COMMAND) {
            return;
        }
        if (!sc01_room_image(memory, cpu)) {
            sc01_room_end(f);
            return;
        }
        f->memory = memory;
        f->caller_sp = cpu->r[29];
    } else if (!issuing) {
        return;
    } else if (!sc01_frame_code(f, cpu->pc)) {
        if (!old_branch || cpu->r[31] != old_branch + 8u || !sc01_frame_code(f, cpu->r[31])) {
            sc01_room_end(f);
            return;
        }
        if (f->request_phase && cpu->pc == 0x80011c10u && cpu->r[31] == SC01_REQUEST_ADVANCE_RETURN)
            f->request_phase = SC01_REQUEST_ADVANCING;
        f->callee_return = cpu->r[31];
        f->callee_sp = cpu->r[29];
        f->return_jump = 0;
    }
    sc01_room_remember(f, cpu);
}
