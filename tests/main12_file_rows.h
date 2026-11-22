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

static void check_file_row_display(FormatterCpu cpu, const MetadataFile *file, int change_after_clock)
{
    metadata_at(&cpu, 0x8017d070u);
    cpu.r[31] = 0x8017c2e4u;
    cpu.r[29] = 0x1f800380u;
    cpu.r[5] = 0x80075b40u;
    cpu.r[6] = 0x80075cd0u;
    cpu.r[7] = 0x20u;
    cpu.r[9] = 0x36u;
    cpu.r[10] = 0xb0u;

    unsigned steps = 0;
    while (cpu.pc != 0x8017d26cu && steps++ < 1500) file_row_step(&cpu);
    CHECK(cpu.pc == 0x8017d26cu && cpu.delay_slot && cpu.npc == 0x8017d730u);
    CHECK(!cpu.main12_building_row && owner->row_pointer_written && !owner->display.started);
    uint32_t pointer;
    CHECK(musashi_boot_read32(&machine, MUSASHI_MAIN12_ROW_CELL, &pointer) &&
          pointer == MUSASHI_MAIN12_ROW_DATA);
    CHECK(!memcmp(owner->mailbox, owner->mailbox + 8, 8));
    FormatterCpu bad = cpu;
    bad.r[5] += 4u;
    expect_refused(&machine, &bad);
    CHECK(formatter_step(&machine, &cpu));
    CHECK(cpu.pc == 0x8017d730u && cpu.main12_building_row && !owner->display.started);

    bad = cpu;
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
    CHECK(dispatches == MAIN12_COLUMNS && occupied_slots == 1 && empty_slots == MAIN12_ROW_COUNT - 1u);
    CHECK(little32(owner->display.descriptors[0]) == 30u);
    for (unsigned i = 1; i < MAIN12_ROW_COUNT; ++i) {
        CHECK(little32(owner->display.descriptors[i]) == 4u);
        CHECK(memcmp(owner->display.strings[0], owner->display.strings[i], sizeof owner->display.strings[i]));
    }
    CHECK(!memcmp(owner->menu_metadata.bytes, file->payload, sizeof file->payload));
    CHECK(!memcmp(machine.bytes + 0x75a40u, file->payload, sizeof file->payload));
    CHECK(!memcmp(musashi_boot_ram_span(&machine, MAIN12_ROW_DESCRIPTORS, sizeof owner->display.descriptors),
                  owner->display.descriptors, sizeof owner->display.descriptors));
    CHECK(!memcmp(musashi_boot_ram_span(&machine, MAIN12_ROW_STRINGS, sizeof owner->display.strings),
                  owner->display.strings, sizeof owner->display.strings));
    puts("Mounted guest code formatted one occupied file row and three empty slots, then returned to its caller.");
}
