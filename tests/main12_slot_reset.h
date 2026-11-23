/* The mounted member supplies each instruction. These checks describe the
 * slot layout, register changes and timing independently of its encoding. */
enum {
    SLOT_RESET_ENTRY = 0x80129258u, SLOT_RESET_RETURN = 0x800d1d7cu,
    SLOT_ARRAY = 0x8011db30u, SLOT_STRIDE = 56u, SLOT_COUNT = 128u
};

static void check_slot_retirement(FormatterCpu *cpu, uint32_t issued, uint32_t expected_pc,
                                  uint32_t expected_npc, uint32_t expected_branch,
                                  unsigned changed_reg, uint32_t changed_value,
                                  uint32_t halfword_store)
{
    uint32_t registers[32], fetched, mounted;
    const unsigned calls = clock_calls;
    const uint64_t previous_cycles = cycles;
    memcpy(registers, cpu->r, sizeof registers);
    if (changed_reg) registers[changed_reg] = changed_value;
    CHECK(cpu->pc == issued);
    if (!formatter_fetch(&machine, cpu, &fetched)) {
        fprintf(stderr, "Room slot reset fetch refused at %08x\n", issued);
        exit(1);
    }
    CHECK(musashi_boot_read32(&machine, issued, &mounted) && mounted == fetched);
    MusashiCpuContext context;
    CHECK(musashi_boot_cpu_context(cpu, MUSASHI_CPU_CONTEXT_SOURCE, &context) &&
          context.identity == cpu && context.gpr == cpu->r && context.instruction_valid &&
          context.instruction == mounted && context.pc == cpu->pc && context.npc == cpu->npc &&
          context.branch_pc == cpu->branch_pc && context.delay_slot == cpu->delay_slot);
    if (halfword_store) {
        /* The mounted instruction is SH. Check its effective address and
         * source value independently of the native slot implementation. */
        const unsigned base = (mounted >> 21) & 31u, data = (mounted >> 16) & 31u;
        const int16_t displacement = (int16_t)(mounted & 0xffffu);
        CHECK((mounted >> 26) == 0x29u &&
              cpu->r[base] + (uint32_t)(int32_t)displacement == halfword_store &&
              (uint16_t)cpu->r[data] == 0);
        CHECK(musashi_boot_write16(&before_step, halfword_store, 0));
    }
    slot_ram_access.memory = &machine;
    slot_ram_access.first = SLOT_ARRAY - 2u;
    slot_ram_access.limit = SLOT_ARRAY + SLOT_COUNT * SLOT_STRIDE + 2u;
    slot_ram_access.count = 0;
    CHECK(formatter_step(&machine, cpu));
    slot_ram_access.memory = NULL;
    CHECK(slot_ram_access.count == (halfword_store ? 1u : 0u));
    if (halfword_store)
        CHECK(slot_ram_access.address == halfword_store && slot_ram_access.width == sizeof(uint16_t));
    CHECK(cpu->pc == expected_pc && cpu->npc == expected_npc &&
          cpu->delay_slot == (expected_branch != 0) && cpu->branch_pc == expected_branch);
    CHECK(!memcmp(registers, cpu->r, sizeof registers));
    CHECK(clock_calls == calls + 1u && cycles == previous_cycles + (halfword_store ? 3u : 2u));
    CHECK(!memcmp(&before_step, &machine, sizeof machine));
}

enum SlotPopulation { SLOTS_ALL_ACTIVE, SLOTS_MIXED, SLOTS_MIXED_WRITE_WITNESS };

/* Exercise both byte lanes of an active synthetic type. The mixed case
 * includes populated edge slots, adjacent populated slots and empty slots. */
static uint16_t slot_initial_type(enum SlotPopulation population, unsigned row)
{
    if (population == SLOTS_ALL_ACTIVE || row == 0 || row == SLOT_COUNT - 1u ||
        row == SLOT_COUNT / 2u - 1u || row == SLOT_COUNT / 2u)
        return 0x0101u;
    return 0;
}

static void check_slot_fields(const uint8_t *guarded, unsigned cleared, enum SlotPopulation population)
{
    CHECK(guarded[0] == '<' && guarded[1] == '[');
    const uint8_t *records = guarded + 2;
    for (unsigned row = 0; row < SLOT_COUNT; ++row) {
        const uint8_t *record = records + row * SLOT_STRIDE;
        const uint16_t type = (uint16_t)(record[0] | (uint16_t)record[1] << 8);
        CHECK(type == (row < cleared ? 0u : slot_initial_type(population, row)));
        for (unsigned pos = sizeof type; pos < SLOT_STRIDE; ++pos)
            CHECK(record[pos] == 'P');
    }
    const uint8_t *end = records + SLOT_COUNT * SLOT_STRIDE;
    CHECK(end[0] == ']' && end[1] == '>');
}

static void check_slot_reset_loop(FormatterCpu *cpu, enum SlotPopulation population)
{
    const uint32_t pc = SLOT_RESET_ENTRY, ret = SLOT_RESET_RETURN;
    uint8_t *slots = musashi_boot_ram_span(&machine, SLOT_ARRAY - 2u,
                                           SLOT_COUNT * SLOT_STRIDE + 4u);
    CHECK(slots && cpu->pc == pc && cpu->npc == pc + 4u && !cpu->delay_slot &&
          !cpu->branch_pc && cpu->r[31] == ret);
    slots[0] = '<';
    slots[1] = '[';
    uint8_t *records = slots + 2;
    memset(records, 'P', SLOT_COUNT * SLOT_STRIDE);
    for (unsigned row = 0; row < SLOT_COUNT; ++row)
        CHECK(musashi_boot_write16(&machine, SLOT_ARRAY + row * SLOT_STRIDE,
                                   slot_initial_type(population, row)));
    uint8_t *end = records + SLOT_COUNT * SLOT_STRIDE;
    end[0] = ']';
    end[1] = '>';
    check_slot_fields(slots, 0, population);
    memcpy(&before_step, &machine, sizeof machine);
    const uint64_t start_cycles = cycles;
    const unsigned start_calls = clock_calls;
    check_slot_retirement(cpu, pc, pc + 4u, pc + 8u, 0, 3, 0x80120000u, 0);
    check_slot_retirement(cpu, pc + 4u, pc + 8u, pc + 12u, 0, 3, SLOT_ARRAY, 0);
    check_slot_retirement(cpu, pc + 8u, pc + 12u, pc + 16u, 0, 2, SLOT_COUNT - 1u, 0);
    unsigned witnessed = 0;
    for (unsigned row = 0; row < SLOT_COUNT; ++row) {
        const uint32_t address = SLOT_ARRAY + row * SLOT_STRIDE;
        const uint32_t remaining = SLOT_COUNT - 1u - row;
        const uint32_t target = row + 1u < SLOT_COUNT ? pc + 12u : pc + 28u;
        CHECK(cpu->r[2] == remaining && cpu->r[3] == address);
        const int inactive_witness = population == SLOTS_MIXED_WRITE_WITNESS &&
                                     !slot_initial_type(population, row);
        uint8_t cell[] = {'[', 'L', 'H', ']'};
        if (inactive_witness) {
            uint16_t prior;
            CHECK(musashi_boot_read16(&machine, address, &prior) && prior == 0);
            slot_ram_access.witness_address = address;
            slot_ram_access.witness = cell + 1;
        }
        check_slot_retirement(cpu, pc + 12u, pc + 16u, pc + 20u, 0, 0, 0, address);
        slot_ram_access.witness = NULL;
        if (inactive_witness) {
            CHECK(cell[0] == '[' && cell[1] == 0 && cell[2] == 0 && cell[3] == ']');
            ++witnessed;
        }
        check_slot_fields(slots, row + 1u, population);
        check_slot_retirement(cpu, pc + 16u, pc + 20u, pc + 24u, 0, 2, remaining - 1u, 0);
        check_slot_retirement(cpu, pc + 20u, pc + 24u, target, pc + 20u, 0, 0, 0);
        check_slot_retirement(cpu, pc + 24u, target, target + 4u, 0, 3, address + SLOT_STRIDE, 0);
    }
    check_slot_retirement(cpu, pc + 28u, pc + 32u, ret, pc + 28u, 0, 0, 0);
    check_slot_retirement(cpu, pc + 32u, ret, ret + 4u, 0, 0, 0, 0);
    CHECK(witnessed == (population == SLOTS_MIXED_WRITE_WITNESS ? SLOT_COUNT - 4u : 0u));
    CHECK(clock_calls == start_calls + 517u && cycles == start_cycles + 1162u);
}

static FormatterCpu enter_slot_reset(void)
{
    const uint32_t call = SLOT_RESET_RETURN - 8u;
    const uint32_t origin = 0x800cedf8u, start = 0x20000u, size = 365404u;
    const uint8_t *record = archive + 8u + 10u * 8u;
    const uint32_t sector = little32(record), length = little32(record + 4u);
    CHECK(length == 497664u && start + size <= length &&
          sector <= (sizeof archive - length) / 2048u);
    seed_machine();
    uint8_t *loaded = musashi_boot_ram_span(&machine, origin, size);
    CHECK(loaded);
    memcpy(loaded, archive + sector * 2048u + start, size);
    musashi_boot_select_overlay_0010_words(1);
    install_by_guest();

    /* The cold File 1 boundary has returned from the preceding resident
     * reset. The genuine mounted JAL and its delay slot must arm this call. */
    FormatterCpu cpu = entry(call);
    cpu.r[2] = 0;
    cpu.r[3] = cpu.r[4] = 0x800b97d8u;
    cpu.r[5] = 0x84u;
    cpu.r[9] = 0x801807dcu;
    cpu.r[10] = 0x800ae6a8u;
    cpu.r[16] = 0x800af630u;
    cpu.r[29] = 0x801ffef0u;
    cpu.r[31] = call;
    CHECK(musashi_boot_write32(&machine, cpu.r[29] + 16u, 0x8017bfe8u));
    CHECK(musashi_boot_write16(&machine, 0x80078e50u, 0x3001u));
    CHECK(musashi_boot_write16(&machine, 0x800b9a08u, 0x3001u));
    CHECK(musashi_boot_write16(&machine, 0x800b9a0au, 0x3000u));
    CHECK(musashi_boot_write16(&machine, 0x800b9a0cu, 0x3000u));
    machine.bytes[0xb9a11u] = 1u;
    machine.bytes[0xb9a15u] = 0;

    FormatterCpu direct = cpu;
    direct.pc = SLOT_RESET_ENTRY;
    direct.npc = direct.pc + 4u;
    direct.r[31] = SLOT_RESET_RETURN;
    expect_refused(&machine, &direct);
    direct.pc += 12u;
    direct.npc = direct.pc + 4u;
    expect_refused(&machine, &direct);
    memcpy(&before_step, &machine, sizeof machine);
    check_slot_retirement(&cpu, call, call + 4u, SLOT_RESET_ENTRY, call, 31, SLOT_RESET_RETURN, 0);
    check_slot_retirement(&cpu, call + 4u, SLOT_RESET_ENTRY, SLOT_RESET_ENTRY + 4u, 0, 0, 0, 0);
    return cpu;
}

static void check_slot_entry_refusals(FormatterCpu cpu)
{
    FormatterCpu bad = cpu;
    bad.r[31] += 4u;
    expect_refused(&machine, &bad);
    bad = cpu;
    bad.pc += 4u;
    bad.npc = bad.pc + 4u;
    expect_refused(&machine, &bad);
    bad = cpu;
    bad.npc += 4u;
    expect_refused(&machine, &bad);
    bad = cpu;
    bad.callback_device = NULL;
    expect_refused(&machine, &bad);
    bad = cpu;
    bad.irq_image = &irq;
    expect_refused(&machine, &bad);
    bad = cpu;
    memcpy(&other_machine, &machine, sizeof machine);
    expect_refused(&other_machine, &bad);
    musashi_boot_select_overlay_0010_words(0);
    bad = cpu;
    expect_refused(&machine, &bad);
    musashi_boot_select_overlay_0010_words(1);
}

static void check_slot_image_changes(void)
{
    /* Mutate outside the call's JAL/delay, inside the callee's last word,
     * and inside the scene entry shared by colliding overlay addresses. */
    const uint32_t sites[] = {0x800d1d24u, SLOT_RESET_ENTRY + 32u, 0x80128158u + 504u};
    for (unsigned i = 0; i < sizeof sites / sizeof sites[0]; ++i) {
        FormatterCpu cpu = enter_slot_reset();
        uint8_t *byte = musashi_boot_ram_span(&machine, sites[i], 1u);
        CHECK(byte);
        *byte ^= 1u;
        expect_refused(&machine, &cpu);
        if (i == 0) {
            /* Repeat the JAL/delay on a fresh CPU while the rest of its
             * containing MAIN/10 routine is already incorrect. */
            FormatterCpu unarmed = entry(SLOT_RESET_RETURN - 8u);
            CHECK(formatter_step(&machine, &unarmed) && formatter_step(&machine, &unarmed));
            CHECK(unarmed.pc == SLOT_RESET_ENTRY && unarmed.r[31] == SLOT_RESET_RETURN);
            expect_refused(&machine, &unarmed);
        }
    }
}

static void check_slot_store_refusals(void)
{
    FormatterCpu cpu = enter_slot_reset();
    for (unsigned i = 0; i < 3; ++i) CHECK(formatter_step(&machine, &cpu));
    CHECK(cpu.pc == SLOT_RESET_ENTRY + 12u && cpu.r[3] == SLOT_ARRAY);
    FormatterCpu bad = cpu;
    bad.r[3] += 2u;
    expect_refused(&machine, &bad);
    bad = cpu;
    bad.r[2] -= 1u;
    expect_refused(&machine, &bad);

    FileRowClock change = {musashi_boot_ram_span(&machine, 0x800d1d24u, 1u), 0};
    MusashiExecutionClock mutated = {&change, change_file_row_sentinel};
    CHECK(change.address && musashi_boot_write16(&machine, SLOT_ARRAY, 0x7513u));
    memcpy(&before_step, &machine, sizeof machine);
    before_step.bytes[0xd1d24u] ^= 1u;
    cpu.clock = &mutated;
    CHECK(!formatter_step(&machine, &cpu) && cpu.pc == SLOT_RESET_ENTRY + 12u);
    CHECK(change.calls == 1u && !memcmp(&before_step, &machine, sizeof machine));
}

/* The optional native SC02/0035 function occupies the same address and
 * has the same function digest. Count dispatches through both lookup APIs. */
static unsigned slot_peer_calls, slot_direct_calls;
static int slot_direct_result;
static const uint64_t slot_body_digest = UINT64_C(0xb9cb72d7e1fd5c28);

static int count_slot_peer(MusashiNativeLaneCall *call)
{
    (void)call;
    ++slot_peer_calls;
    return 1;
}

static int query_slot_peer(MusashiNativeLaneCall *call)
{
    (void)call;
    slot_direct_result = musashi_native_lane_overlay_resident(SLOT_RESET_ENTRY, 36, slot_body_digest);
    ++slot_direct_calls;
    return 1;
}

static void install_slot_peer(void)
{
    MusashiNativeLaneOverlayEntry peer = {
        {SLOT_RESET_ENTRY, 18, count_slot_peer, "sc02_0035__func_80129258", 0, 0},
        36, slot_body_digest, "sc02_0035"
    };
    MusashiNativeLaneEntry observer = {return_pc, 2, query_slot_peer, "slot residency probe", 0, 0};
    CHECK(musashi_native_lane_install_overlay(&peer, 1) == 1);
    CHECK(musashi_native_lane_install(&observer, 1) == 1);
}

static void check_slot_direct_query(int expected)
{
    FormatterCpu caller = entry(return_pc);
    caller.r[31] = return_pc + 8u;
    const unsigned count = slot_direct_calls;
    CHECK(lane_try(&machine, &caller) == 1 && slot_direct_calls == count + 1u &&
          slot_direct_result == expected);
}

static void check_slot_peer_on_scene(const char *path)
{
    /* Supply the project decoder's SC02 member 35 from the supported disc. */
    const size_t size = 616687u;
    uint8_t *scene = musashi_boot_ram_span(&machine, load_address, size);
    FILE *file = fopen(path, "rb");
    CHECK(scene && file && fread(scene, 1, size, file) == size && fgetc(file) == EOF &&
          !ferror(file) && !fclose(file));
    check_digest(scene, size, "6fcb360e5153a15d587216d4ae12f3ab05997a4055f3056b68b63c540085a145");
    uint64_t digest;
    CHECK(span_digest(&machine, SLOT_RESET_ENTRY, 9, &digest) && digest == slot_body_digest);
    FormatterCpu cpu = entry(SLOT_RESET_ENTRY);
    const unsigned count = slot_peer_calls;
    CHECK(formatter_step(&machine, &cpu) && slot_peer_calls == count + 1u && cpu.pc == return_pc);
    check_slot_direct_query(1);
    /* Corrupt the scene's entry while preserving all nine colliding words. */
    scene[504] ^= 1u;
    cpu = entry(SLOT_RESET_ENTRY);
    expect_refused(&machine, &cpu);
    check_slot_direct_query(0);
    CHECK(slot_peer_calls == count + 1u);
    puts("SC02 slot dispatch and direct membership followed the mounted scene.");
}

/* Resume the existing mounted file-load fixture while its CD/event
 * leases and the foreground invocation that called the room initializer
 * are still live. MAIN/10's caller bytes come from the same owned archive. */
#include "main12_scene_setup.h"

static void check_foreground_slot_reset(FormatterCpu *cpu)
{
    const uint32_t caller = 0x800d1d24u, called_from = MAIN12_ROOM_RETURN + 8u;
    const uint8_t *row = archive + 8u + 10u * 8u;
    const uint32_t sector = little32(row), length = little32(row + 4u);
    const size_t caller_offset = 0x20000u + (caller - 0x800cedf8u);
    uint8_t *code = musashi_boot_ram_span(&machine, caller, 112u);
    CHECK(code && length == 497664u && caller_offset + 112u <= length &&
          sector <= (sizeof archive - length) / 2048u);
    memcpy(code, archive + sector * 2048u + caller_offset, 112u);
    musashi_boot_select_overlay_0010_words(1);
    CHECK(cpu->pc == MAIN12_ROOM_RETURN && cpu->main12_foreground_owner == owner &&
          owner->restore.foreground_result_written && !cpu->main12_room_next);
    check_latch_missing_call(*cpu);
    CHECK(formatter_step(&machine, cpu) && cpu->delay_slot && cpu->npc == caller);
    CHECK(formatter_step(&machine, cpu) && cpu->pc == caller && cpu->r[31] == called_from);
    check_latch_mode_fields(*cpu);
    check_latch_call(cpu, caller, caller + 64u, 0x8001c0c8u);
    run_latch_service(cpu, caller + 72u);
    check_latch_call(cpu, caller + 72u, caller + 72u, 0x80015310u);
    run_latch_service(cpu, caller + 80u);
    check_latch_call(cpu, caller + 80u, caller + 80u, SLOT_RESET_ENTRY);
    CHECK(cpu->pc == SLOT_RESET_ENTRY && cpu->r[31] == SLOT_RESET_RETURN &&
          cpu->r[29] + 48u == cpu->main12_foreground_sp);

    FormatterCpu direct = entry(SLOT_RESET_ENTRY);
    direct.main12_foreground_owner = owner;
    direct.main12_foreground_sp = cpu->main12_foreground_sp;
    direct.r[29] = cpu->r[29];
    direct.r[31] = SLOT_RESET_RETURN;
    expect_refused(&machine, &direct);
    FormatterCpu bad = *cpu;
    MusashiMain12Owner foreign = *owner;
    bad.main12_foreground_owner = &foreign;
    expect_refused(&machine, &bad);

    const uint32_t links[] = {cpu->r[29] + 16u, cpu->main12_foreground_sp - 8u};
    const uint32_t expected[] = {called_from, MAIN12_MENU_EXIT_RETURN};
    for (unsigned i = 0; i < 2u; ++i) {
        uint32_t value;
        CHECK(musashi_boot_read32(&machine, links[i], &value) && value == expected[i]);
        CHECK(musashi_boot_write32(&machine, links[i], value + 4u));
        bad = *cpu;
        expect_refused(&machine, &bad);
        CHECK(musashi_boot_write32(&machine, links[i], value));
    }
    FormatterCpu write_witness = *cpu;
    check_slot_reset_loop(cpu, SLOTS_MIXED);
    /* Repeat from the authenticated entry. A store to an inactive slot
     * leaves real RAM unchanged. In this pass alone, supply two marked
     * bytes at that slot's address so a read or partial write is visible. */
    check_slot_reset_loop(&write_witness, SLOTS_MIXED_WRITE_WITNESS);
    CHECK(cpu->main12_foreground_owner == owner && !cpu->main12_room_next);
    check_scene_setup(cpu, 1);
    musashi_boot_select_overlay_0010_words(0);
    puts("File load retained its foreground owner across both calls and the slot reset.");
}

static void check_mounted_slot_reset(void)
{
    if (mapped_window) install_slot_peer();
    FormatterCpu cpu = enter_slot_reset();
    if (mapped_window) check_slot_direct_query(0);
    check_slot_entry_refusals(cpu);
    check_slot_reset_loop(&cpu, SLOTS_ALL_ACTIVE);
    check_slot_image_changes();
    check_slot_store_refusals();
    musashi_boot_select_overlay_0010_words(0);
    if (mapped_window) {
        CHECK(slot_peer_calls == 0);
        const char *scene = getenv("MUSASHI_TEST_SC02_SLOT_MEMBER");
        if (scene && *scene) check_slot_peer_on_scene(scene);
        CHECK(musashi_native_lane_install_overlay(NULL, 0) == 0);
        CHECK(musashi_native_lane_install(NULL, 0) == 0);
    }
    puts("Room slot reset returned after 517 steps, 1162 cycles and 128 halfword stores.");
}
