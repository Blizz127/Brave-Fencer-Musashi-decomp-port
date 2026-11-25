/* SC01 members 5 and 6 have the same decoded image. The saved-room scene
 * selects command 1 through its own live table. Follow that invocation across
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
        { 0x80129c40u, 0x80129cf8u, UINT64_C(0xbc5ca01e22b4970c) }
    };
    for (size_t i = 0; i < sizeof code / sizeof code[0]; ++i)
        if (pc >= code[i].start && pc < code[i].end) return &code[i];
    return NULL;
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
    return intrinsic_span(cpu, SC01_ROOM_ENTRY, SC01_ROOM_COMMAND - SC01_ROOM_ENTRY) &&
           intrinsic_span(cpu, SC01_ROOM_TARGET, sizeof target) &&
           span_digest(memory, SC01_ROOM_ENTRY,
                       (SC01_ROOM_COMMAND - SC01_ROOM_ENTRY) / 4u, &prefix) &&
           prefix == UINT64_C(0x4f4e44c8e1a9471a) &&
           musashi_boot_read32(memory, SC01_ROOM_TARGET, &target) && target == SC01_ROOM_COMMAND;
}

static int sc01_room_expected(const FormatterCpu *cpu)
{
    const Sc01RoomFrame *f = &cpu->sc01_room;
    return f->next && cpu->pc == f->next && sc01_room_foreground(cpu);
}

static int sc01_room_word(MusashiBootMemory *memory, const FormatterCpu *cpu, uint32_t *out)
{
    const Sc01RoomFrame *f = &cpu->sc01_room;
    const Sc01RoomCode *code;
    uint64_t digest;
    if (f->memory != memory || f->npc != cpu->npc || f->branch != cpu->branch_pc ||
        !!f->branch != !!cpu->delay_slot || f->sp != cpu->r[29] || f->ra != cpu->r[31] ||
        !sc01_room_image(memory, cpu)) return 0;
    if (cpu->pc != SC01_ROOM_CALL + 4u) {
        code = sc01_room_code(cpu->pc);
        if (!code || !intrinsic_span(cpu, code->start, code->end - code->start) ||
            !span_digest(memory, code->start, (code->end - code->start) / 4u, &digest) ||
            digest != code->digest) return 0;
    }
    return musashi_boot_read32(memory, cpu->pc, out);
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
        f->next = 0;
        if (cpu->pc != SC01_ROOM_CALL + 4u || cpu->npc != SC01_ROOM_COMMAND ||
            cpu->branch_pc != SC01_ROOM_CALL || !cpu->delay_slot ||
            cpu->r[2] != SC01_ROOM_COMMAND || cpu->r[3] != 1u ||
            cpu->r[31] != SC01_ROOM_CALL + 8u ||
            !intrinsic_span(cpu, 0x800b99f6u, 2u) ||
            !musashi_boot_read16(memory, 0x800b99f6u, &command) || command != 1u ||
            !sc01_room_image(memory, cpu)) return;
        f->memory = memory;
    } else if (!issuing) {
        return;
    } else if (!sc01_room_code(cpu->pc)) {
        /* A delay slot following JAL can enter a resident service (including
         * the normal memory clear). Preserve its return without admitting any
         * of that service under the scene's code identity. */
        f->next = 0;
        if (old_branch && cpu->r[31] == old_branch + 8u && sc01_room_code(cpu->r[31])) {
            f->next = cpu->r[31];
            f->npc = f->next + 4u;
            f->branch = 0;
            f->sp = cpu->r[29];
            f->ra = f->next;
        }
        return;
    }
    f->next = cpu->pc;
    f->npc = cpu->npc;
    f->branch = cpu->branch_pc;
    f->sp = cpu->r[29];
    f->ra = cpu->r[31];
}
