/* The ordered PCs and four destinations come from the published room latch.
 * The separate scalar oracle reads the instruction actually mounted by the
 * File 1 fixture and predicts its register, branch, RAM and clock effects. */
typedef struct {
    const FormatterCpu *cpu;
    uint32_t word, cost;
    uint8_t *changed;
    unsigned calls;
} RoomLatchPause;

static int pause_room_latch(void *userdata, uint32_t cost)
{
    RoomLatchPause *pause = userdata;
    MusashiCpuContext ctx;
    const FormatterCpu *cpu = pause->cpu;
    CHECK(++pause->calls == 1u && cost == pause->cost);
    CHECK(musashi_boot_cpu_context(cpu, MUSASHI_CPU_CONTEXT_SOURCE, &ctx) &&
          ctx.identity == cpu && ctx.gpr == cpu->r && ctx.pc == cpu->pc &&
          ctx.npc == cpu->npc && ctx.branch_pc == cpu->branch_pc &&
          ctx.delay_slot == cpu->delay_slot && ctx.instruction_valid &&
          ctx.instruction == pause->word);
    if (!pause->changed) return 0;
    *pause->changed ^= 1u;
    return 1;
}

static void check_latch_boundary(FormatterCpu *cpu, uint32_t fetched)
{
    CHECK(cpu->room_latch.owner == owner && room_latch_active(cpu) &&
          room_latch_valid(&machine, cpu));
    FormatterCpu trial = *cpu;
    trial.room_latch.next += 4u;
    expect_refused(&machine, &trial);
    trial = *cpu;
    trial.npc += 4u;
    expect_refused(&machine, &trial);
    trial = *cpu;
    memset(&trial.room_latch, 0, sizeof trial.room_latch);
    expect_refused(&machine, &trial);

    /* Interrupt at every position, including JAL/JR delay slots. A serviced
     * clock may instead return success after the borrowed code has changed. */
    trial = *cpu;
    const unsigned op = fetched >> 26;
    RoomLatchPause pause = {&trial, fetched, op >= 32u ? 3u : 2u, NULL, 0};
    MusashiExecutionClock hold = {&pause, pause_room_latch};
    trial.clock = &hold;
    const FormatterCpu before = trial;
    CHECK(!formatter_step(&machine, &trial) && pause.calls == 1u);
    CHECK(!memcmp(&trial, &before, sizeof trial) && !memcmp(&before_step, &machine, sizeof machine));
    if (cpu->pc == ROOM_RESET_CALLER + 32u) { /* store of the ready flag */
        pause.changed = musashi_boot_ram_span(&machine, ROOM_RESET_CALLER + 111u, 1u);
        pause.calls = 0;
        CHECK(pause.changed && !formatter_step(&machine, &trial) && pause.calls == 1u);
        *pause.changed ^= 1u;
        CHECK(!memcmp(&trial, &before, sizeof trial) && !memcmp(&before_step, &machine, sizeof machine));
    }
}

/* Run an uninterrupted part of the caller, stopping at the real first
 * instruction of its callee. Rebuild the oracle after each callee returns,
 * since its registers and RAM may have changed in the meantime. */
static void check_latch_call(FormatterCpu *cpu, uint32_t start, uint32_t call, uint32_t target)
{
    CHECK(cpu->pc == start && cpu->npc == start + 4u && !cpu->delay_slot && !cpu->branch_pc);
    SceneOracle model = {.pc = cpu->pc, .npc = cpu->npc};
    memcpy(model.r, cpu->r, sizeof model.r);
    memcpy(&before_step, &machine, sizeof machine);
    const unsigned start_calls = clock_calls;
    const uint64_t start_cycles = cycles;
    for (uint32_t pc = start; pc <= call + 4u; pc += 4u) {
        uint32_t mounted, fetched;
        CHECK(cpu->pc == pc && musashi_boot_read32(&machine, pc, &mounted) &&
              formatter_fetch(&machine, cpu, &fetched) && mounted == fetched);
        check_latch_boundary(cpu, fetched);
        scene_oracle_issue(&model, &before_step);
        CHECK(formatter_step(&machine, cpu) && cpu->pc == model.pc && cpu->npc == model.npc &&
              cpu->branch_pc == model.branch && cpu->delay_slot == (model.branch != 0));
        CHECK(!memcmp(cpu->r, model.r, sizeof model.r) && !memcmp(&machine, &before_step, sizeof machine));
        CHECK(clock_calls == start_calls + model.instructions && cycles == start_cycles + model.cycles);
        if (pc == call)
            CHECK(cpu->pc == pc + 4u && cpu->npc == target && cpu->delay_slot &&
                  cpu->branch_pc == pc && cpu->r[31] == pc + 8u);
    }
    CHECK(cpu->pc == target && cpu->npc == target + 4u && !cpu->delay_slot && !cpu->branch_pc &&
          cpu->r[31] == call + 8u && cpu->room_latch.owner == owner);
    /* A caller PC and matching saved link do not retire a child return. */
    FormatterCpu skipped = *cpu;
    skipped.pc = call + 8u;
    skipped.npc = skipped.pc + 4u;
    expect_refused(&machine, &skipped);
}

static void run_latch_service(FormatterCpu *cpu, uint32_t resume)
{
    const uint32_t sp = cpu->r[29];
    unsigned steps = 0;
    while (cpu->pc != resume && steps < 150000u) {
        CHECK(formatter_step(&machine, cpu));
        ++steps;
    }
    CHECK(steps && steps < 150000u && cpu->pc == resume && cpu->npc == resume + 4u &&
          !cpu->branch_pc && !cpu->delay_slot && cpu->r[29] == sp && cpu->room_latch.owner == owner);
}

/* The restored room can have the same previous and current high nibbles.
 * Replay only the prefix on a copy of this authenticated foreground CPU to
 * exercise a distinct old transition and a pending mode with low bits set. */
static void check_latch_mode_fields(FormatterCpu cpu)
{
    const uint16_t pending = 0xb307u, previous = 0x4210u;
    const uint32_t stack = cpu.r[29], return_pc = cpu.r[31];
    memcpy(&before_step, &machine, sizeof machine);
    CHECK(musashi_boot_write16(&machine, 0x80078e50u, pending));
    CHECK(musashi_boot_write16(&machine, 0x800b9a0au, previous));
    for (unsigned word = 0; word < 16u; ++word) {
        CHECK(cpu.pc == ROOM_RESET_CALLER + 4u * word && formatter_step(&machine, &cpu));
    }
    CHECK(cpu.pc == ROOM_RESET_CALLER + 64u && cpu.r[29] == stack - 24u &&
          cpu.r[31] == return_pc && scene_oracle_read(&machine, stack - 8u, 4u) == return_pc);
    CHECK(scene_oracle_read(&machine, 0x80078e50u, 2u) == pending &&
          scene_oracle_read(&machine, 0x800b9a08u, 2u) == pending &&
          scene_oracle_read(&machine, 0x800b9a0au, 2u) == (pending & 0xf000u) &&
          scene_oracle_read(&machine, 0x800b9a0cu, 2u) == previous &&
          scene_oracle_read(&machine, 0x800b9a11u, 1u) == 1u);
    memcpy(&machine, &before_step, sizeof machine);
}

/* Recreating the visible incoming delay slot must not manufacture the
 * caller's continuation without retiring its foreground JAL. */
static void check_latch_missing_call(FormatterCpu trial)
{
    trial.branch_pc = trial.pc;
    trial.pc += 4u;
    trial.npc = ROOM_RESET_CALLER;
    trial.delay_slot = 1;
    trial.r[31] = MAIN12_ROOM_RETURN + 8u;
    if (formatter_step(&machine, &trial)) {
        CHECK(trial.pc == ROOM_RESET_CALLER);
        expect_refused(&machine, &trial);
    }
    CHECK(!trial.room_latch.owner);
}
