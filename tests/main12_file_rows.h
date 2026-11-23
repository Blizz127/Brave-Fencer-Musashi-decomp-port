/* The directory and file contents come from the synthetic IRQ device. The
 * display code, its lookup tables and its helpers come from the mounted disc. */
typedef struct {
    uint8_t *address;
    unsigned calls;
} FileRowClock;

static int change_file_row_sentinel(void *userdata, uint32_t cost)
{
    FileRowClock *clock = userdata;
    CHECK(cost == 3u);
    ++clock->calls;
    *clock->address ^= 1u;
    return 1;
}

static void file_row_step(FormatterCpu *cpu)
{
    if (!formatter_step(&machine, cpu)) {
        fprintf(stderr, "File row instruction refused at %08x\n", cpu->pc);
        exit(1);
    }
}

static void check_row_call_rejects_changed_arguments(const FormatterCpu *cpu)
{
    for (unsigned argument = 4; argument <= 7; ++argument) {
        FormatterCpu bad = *cpu;
        bad.r[argument] ^= argument <= 5 ? 128u : 4u;
        expect_refused(&machine, &bad);
    }
}

static void check_file_row_display(FormatterCpu cpu, const MetadataFile *file, int change_after_clock)
{
    metadata_at(&cpu, 0x8017d070u);
    cpu.r[31] = 0x8017c2e4u;
    cpu.r[29] = 0x1f800380u;
    /* End cursors observed after the mounted checksum service, by last
     * populated header. A checksum failure still advances the reader. */
    static const uint32_t summary_end[] = {0x80075b40u, 0x80075bc0u, 0x80075c40u, 0x80075cc0u};
    cpu.r[5] = 0x1f800380u;
    for (unsigned slot = 0; slot < MAIN12_ROW_COUNT; ++slot)
        if (file->occupied_mask & (1u << slot)) cpu.r[5] = summary_end[slot];
    cpu.r[6] = 0x80075cd0u;
    cpu.r[7] = 0x20u;
    cpu.r[9] = 0x36u;
    cpu.r[10] = 0xb0u;

    unsigned steps = 0;
    unsigned valid_mask = file->occupied_mask & ~file->damaged_mask;
    if (!valid_mask) {
        /* Empty or entirely damaged saves take the no-file return. */
        while (cpu.pc != 0x8017d270u && steps++ < 1500) file_row_step(&cpu);
        CHECK(cpu.pc == 0x8017d270u && cpu.r[19] == 0x13u && !owner->display.started &&
              !owner->row_pointer_mask &&
              owner->row_pointer_written == (file->occupied_mask ? MAIN12_ROW_COUNT : 0u));
        while (cpu.pc != 0x8017d280u && steps++ < 1510) file_row_step(&cpu);
        uint32_t result;
        CHECK(cpu.pc == 0x8017d280u && cpu.r[31] == 0x8017c2e4u &&
              musashi_boot_read32(&machine, MAIN12_MENU_WORD, &result) && result == 19u &&
              owner->display.menu_code == 19u && main12_current(owner, &machine));
        return;
    }
    while (cpu.pc != 0x8017d26cu && steps++ < 1500) {
        if (cpu.pc == 0x8017d268u) check_row_call_rejects_changed_arguments(&cpu);
        if (main12_pointer_store_pc(cpu.pc)) {
            FormatterCpu bad = cpu;
            ++bad.r[16];
            expect_copy_access_refused(&bad);
            bad = cpu;
            ++bad.r[17];
            expect_copy_access_refused(&bad);
            bad = cpu;
            ++bad.r[18];
            expect_copy_access_refused(&bad);
        }
        file_row_step(&cpu);
    }
    CHECK(cpu.pc == 0x8017d26cu && cpu.delay_slot && cpu.npc == 0x8017d730u);
    CHECK(!cpu.main12_building_row && !owner->display.started &&
          owner->row_pointer_written == MAIN12_ROW_COUNT && owner->row_pointer_mask == valid_mask);
    for (unsigned row = 0; row < MAIN12_ROW_COUNT; ++row) {
        uint32_t pointer;
        CHECK(musashi_boot_read32(&machine, MUSASHI_MAIN12_ROW_CELL + row * 4u, &pointer));
        CHECK(pointer == (valid_mask & (1u << row) ? MUSASHI_MAIN12_ROW_DATA + row * 128u : 0u));
        uint8_t *cell = musashi_boot_ram_span(&machine, MUSASHI_MAIN12_ROW_CELL + row * 4u, 1u);
        CHECK(cell);
        *cell ^= 1u;
        CHECK(!main12_identity(owner, &machine, 1));
        *cell ^= 1u;
        CHECK(main12_identity(owner, &machine, 1));
    }
    CHECK(!memcmp(owner->mailbox, owner->mailbox + 8, 8));
    check_row_call_rejects_changed_arguments(&cpu);
    CHECK(formatter_step(&machine, &cpu));
    CHECK(cpu.pc == 0x8017d730u && cpu.main12_building_row && !owner->display.started);
    check_row_call_rejects_changed_arguments(&cpu);

    FormatterCpu bad = cpu;
    bad.r[31] += 4u;
    expect_refused(&machine, &bad);
    bad = cpu;
    bad.pc = bad.main12_row_pc = 0x8017dd28u;
    bad.npc = bad.main12_row_npc = bad.pc + 4u;
    expect_refused(&machine, &bad);

    MetadataClock clock = {.alter_payload=1};
    MusashiExecutionClock alter_read = {&clock, metadata_store_clock};
    cpu.clock = &alter_read;
    CHECK(!formatter_step(&machine, &cpu));
    CHECK(clock.calls == 1 && clock.cost == 2u && cpu.pc == 0x8017d730u &&
          cpu.r[29] == 0x1f800380u && !owner->display.started);
    machine.bytes[0x75a40u + sizeof file->payload - 1] ^= 1u;
    cpu.clock = &clock_source;
    CHECK(formatter_step(&machine, &cpu) && owner->display.started);
    CHECK(cpu.r[29] == 0x1f800348u);

    const uint32_t last_descriptor = MAIN12_ROW_DESCRIPTORS + sizeof owner->display.descriptors - 1u;
    const uint32_t last_string = MAIN12_ROW_STRINGS + sizeof owner->display.strings - 1u;
    if (change_after_clock) {
        /* The injected write must be the sole RAM change. A revoked owner
         * cannot resume by restoring the byte after the rejected stack store. */
        FileRowClock change = {musashi_boot_ram_span(&machine, last_string, 1), 0};
        CHECK(change.address && cpu.pc == 0x8017d734u);
        MusashiExecutionClock hold = {&change, change_file_row_sentinel};
        cpu.clock = &hold;
        FormatterCpu saved = cpu;
        memcpy(&before_step, &machine, sizeof machine);
        before_step.bytes[last_string & 0x1fffffu] ^= 1u;
        CHECK(!formatter_step(&machine, &cpu) && change.calls == 1 && !owner->installed);
        CHECK(!memcmp(&cpu, &saved, sizeof cpu) && !memcmp(&machine, &before_step, sizeof machine));
        *change.address ^= 1u;
        CHECK(!formatter_step(&machine, &cpu) && change.calls == 1);
        return;
    }
    const uint32_t sentinels[] = {last_descriptor, last_string};
    for (unsigned i = 0; i < sizeof sentinels / sizeof sentinels[0]; ++i) {
        uint8_t *sentinel = musashi_boot_ram_span(&machine, sentinels[i], 1);
        CHECK(sentinel);
        *sentinel ^= 1u;
        CHECK(!main12_identity(owner, &machine, 1));
        *sentinel ^= 1u;
        CHECK(main12_identity(owner, &machine, 1));
    }

    unsigned dispatches = 0, occupied_slots = 0, empty_slots = 0;
    for (steps = 0; cpu.pc != 0x8017dc44u && steps < 25000; ++steps) {
        if (cpu.pc == 0x8017d7d0u) ++dispatches;
        if (cpu.pc == 0x8017db70u) ++empty_slots;
        if (cpu.pc == 0x8017d84cu) {
            /* An otherwise valid guest store must stay in its current row. */
            uint32_t word;
            CHECK(formatter_fetch(&machine, &cpu, &word) && word >> 26 == 43u);
            bad = cpu;
            bad.r[(word >> 21) & 31u] += MAIN12_COLUMNS * 4u;
            expect_copy_access_refused(&bad);
            ++occupied_slots;
        }
        file_row_step(&cpu);
    }
    CHECK(cpu.pc == 0x8017dc44u && cpu.delay_slot && cpu.npc == 0x8017d270u);
    bad = cpu;
    bad.r[31] += 4u;
    expect_refused(&machine, &bad);
    CHECK(formatter_step(&machine, &cpu) && cpu.pc == 0x8017d270u && cpu.r[29] == 0x1f800380u);
    CHECK(owner->display.complete && !cpu.main12_building_row);
    unsigned expected_occupied = 0;
    for (unsigned slot = 0; slot < MAIN12_ROW_COUNT; ++slot) {
        int occupied = (valid_mask & (1u << slot)) != 0;
        expected_occupied += occupied;
        CHECK(little32(owner->display.descriptors[slot]) == (occupied ? 30u : 4u));
        if (occupied != (valid_mask & 1u))
            CHECK(memcmp(owner->display.strings[0], owner->display.strings[slot], sizeof owner->display.strings[slot]));
    }
    CHECK(dispatches == MAIN12_COLUMNS * expected_occupied && occupied_slots == expected_occupied &&
          empty_slots == MAIN12_ROW_COUNT - expected_occupied);
    CHECK(!memcmp(owner->menu_metadata.bytes, file->payload, sizeof file->payload));
    CHECK(!memcmp(machine.bytes + 0x75a40u, file->payload, sizeof file->payload));
    CHECK(!memcmp(musashi_boot_ram_span(&machine, MAIN12_ROW_DESCRIPTORS, sizeof owner->display.descriptors),
                  owner->display.descriptors, sizeof owner->display.descriptors));
    CHECK(!memcmp(musashi_boot_ram_span(&machine, MAIN12_ROW_STRINGS, sizeof owner->display.strings),
                  owner->display.strings, sizeof owner->display.strings));
    printf("Mounted file rows matched valid mask %x and damaged mask %x.\n", valid_mask, file->damaged_mask);
}
