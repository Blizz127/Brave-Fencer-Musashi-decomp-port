/* Mounted overlay and resident code come from the existing owned-disc test.
 * The directory grant and asynchronous device below are synthetic. The three
 * registered interrupts and their guest calls use the normal IRQ executor. */
#include "irq_guest_fixture.h"

typedef struct {
    MusashiBiosEvents bank;
    unsigned seeks, reads, polls, closes, captures, occupied_mask, damaged_mask;
    uint64_t open_id, read_id;
    uint8_t payload[640];
    int closed, fail_close, change_at_close;
    uint32_t position;
    int pending;
} MetadataFile;

static const MusashiBiosEvents *metadata_poll_owner(void *userdata)
{
    return &((MetadataFile *)userdata)->bank;
}

static int metadata_poll(void *userdata, int32_t handle, int32_t *result)
{
    MetadataFile *file = userdata;
    ++file->polls;
    return musashi_bios_events_test(&file->bank, handle, result);
}

static uint64_t metadata_id(void *userdata, const MusashiBootMemory *ram,
    const MusashiBiosEvents *bank, int32_t fd, uint32_t position, uint32_t count)
{
    const MetadataFile *file = userdata;
    return ram == &machine && bank == &file->bank && fd == 2 && !file->closed && !file->pending &&
        position == file->position && count == (position == 512 ? 640u : 0u) ? file->open_id : 0u;
}

static uint64_t metadata_copy(void *userdata, const MusashiBootMemory *ram,
    const MusashiBiosEvents *bank, int32_t fd, uint64_t open_id, uint8_t out[640])
{
    MetadataFile *file = userdata;
    if (!open_id || metadata_id(userdata, ram, bank, fd, 1152, 0) != open_id ||
        memcmp(ram->bytes + 0x75a40u, file->payload, sizeof file->payload)) return 0;
    if (out) {
        ++file->captures;
        memcpy(out, file->payload, sizeof file->payload);
    }
    return file->read_id;
}

static int metadata_vector(void *userdata, uint32_t vector, uint32_t selector,
                           const uint32_t args[4], uint32_t *result)
{
    MetadataFile *file = userdata;
    CHECK(vector == 0xb0u && args[0] == 2u && !file->pending);
    if (selector == 0x36u) {
        CHECK(!file->closed && file->position == 1152);
        ++file->closes;
        file->closed = !file->fail_close;
        if (file->change_at_close) machine.bytes[0x75a40u + 319u] ^= 1;
        *result = file->closed ? 2u : UINT32_MAX;
    } else if (selector == 0x33u) {
        CHECK(!file->position && args[1] == 512 && !args[2]);
        ++file->seeks;
        *result = file->position = 512;
    } else {
        CHECK(selector == 0x34u && file->position == 512 && args[1] == 0x80075a40u && args[2] == 640);
        ++file->reads;
        file->pending = 1;
        *result = 0;
    }
    return 1;
}

static void metadata_put(uint32_t address, uint32_t value)
{
    CHECK(musashi_boot_write32(&machine, address, value));
}

static unsigned metadata_irq(const MusashiGuestIrqCard *card, int expected)
{
    FixtureVblankBus pulse = {1u, 0};
    MusashiIrqDispatchDevice io = {.userdata=&pulse, .read16=fixture_vblank_read,
        .write16=fixture_vblank_write, .return_from_exception=fixture_vblank_return};
    MusashiGuestIrqServices service = {.card=card};
    MusashiGuestIrqStop stop;
    uint32_t installed[12];
    for (unsigned i = 0; i < 12; ++i)
        CHECK(musashi_boot_read32(&machine, 0x8006bb34u + 4u*i, &installed[i]));
    int result = musashi_boot_execute_guest_irq_with_services(&irq, &machine, 0x8006bb34u,
        installed, &io, &clock_source, NULL, &service, &stop);
    if (expected)
        CHECK(result == MUSASHI_IRQ_CONTEXT_RETURN_DELEGATED && stop.pc == 0xb0u && pulse.returns == 1);
    else
        CHECK(!result && stop.pc == 0x80062950u && !pulse.returns);
    CHECK(stop.instructions > 0);
    return stop.instructions;
}

enum MetadataCloseCase { CLOSE_OK, CLOSE_NO_WITNESS, CLOSE_NEW_EVENT, CLOSE_FAILED, CLOSE_CHANGED, CLOSE_PRELOADED, CLOSE_ROW_CHANGED };

static void metadata_at(FormatterCpu *cpu, uint32_t pc)
{
    cpu->pc = pc; cpu->npc = pc + 4u;
}

static FormatterCpu metadata_foreground(MusashiCdDevice *files, MusashiEventDevice *polls,
                                        const uint8_t row[40])
{
    FormatterCpu cpu = entry(callback_pc);
    cpu.cd_device = files; cpu.event_device = polls;
    CHECK(formatter_step(&machine, &cpu) && cpu.main12_callback_frame == owner);
    metadata_at(&cpu, MAIN12_SERVICE);
    cpu.r[6] = MAIN12_MAILBOX; cpu.r[31] = MAIN12_SERVICE_RETURN;
    CHECK(formatter_step(&machine, &cpu) && cpu.main12_card_service);

    /* Supply the synthetic directory return and opened file descriptor to the same
     * foreground hooks used by the mounted player. No completed read is seeded. */
    memcpy(machine.scratchpad + 0x2f8u, row, 40);
    metadata_at(&cpu, 0xb0u);
    cpu.r[9] = 0x42; cpu.r[31] = 0x80062128u;
    cpu.r[4] = 0x1f8002d8u; cpu.r[5] = 0x1f8002f8u;
    main12_card_file_result(&machine, &cpu, cpu.r[5], 0);
    CHECK(cpu.main12_directory_found && owner->directory_valid);
    metadata_at(&cpu, 0x8002c2b8u); cpu.r[31] = 0x8002c148u;
    CHECK(cpu_write8(&machine, &cpu, MAIN12_MAILBOX + 1u, 1));
    CHECK(cpu_write8(&machine, &cpu, MAIN12_MAILBOX + 1u, 7));
    metadata_at(&cpu, 0x8017d0bcu);
    CHECK(cpu_write8(&machine, &cpu, MAIN12_MAILBOX + 9u, 7));

    metadata_at(&cpu, 0xb0u);
    cpu.r[9] = 0x32; cpu.r[31] = 0x800602c8u; cpu.r[29] = 0x1f800338u;
    cpu.r[4] = 0x80078ca0u; cpu.r[5] = 0x8001u;
    main12_card_file_result(&machine, &cpu, 2, 0);
    CHECK(owner->directory_fd_token && owner->metadata_read.files == files);
    return cpu;
}

static int metadata_stop_after_vector(void *unused, const void *state)
{
    (void)unused;
    return ((const FormatterCpu *)state)->pc == 0xb0u;
}

static void refused_mailbox_step(FormatterCpu cpu)
{
    const FormatterCpu before = cpu;
    const unsigned previous_calls = clock_calls, stage = owner->menu_metadata.stage;
    const uint64_t previous_cycles = cycles;
    memcpy(&before_step, &machine, sizeof machine);
    int stepped = formatter_step(&machine, &cpu);
    if (stepped) fprintf(stderr, "Unexpected metadata store at %08x with base %08x and value %u\n",
                         before.pc, before.r[19], before.r[2]);
    CHECK(!stepped);
    CHECK(before.pc == cpu.pc && before.npc == cpu.npc && !memcmp(before.r, cpu.r, sizeof cpu.r));
    CHECK(!memcmp(&before_step, &machine, sizeof machine) && stage == owner->menu_metadata.stage);
    CHECK(clock_calls == previous_calls && cycles == previous_cycles);
}

typedef struct { unsigned calls, cost; int alter_payload; } MetadataClock;

static int metadata_store_clock(void *userdata, uint32_t cycles_requested)
{
    MetadataClock *clock = userdata;
    ++clock->calls;
    clock->cost += cycles_requested;
    if (!clock->alter_payload) return 0;
    machine.bytes[0x75a40u + 639u] ^= 1;
    return 1;
}

#include "main12_file_rows.h"

static void metadata_close(FormatterCpu cpu, MetadataFile *file, enum MetadataCloseCase which)
{
    const int clean_close = which == CLOSE_OK || which == CLOSE_ROW_CHANGED;
    uint32_t state;
    CHECK(musashi_boot_read32(&machine, 0x8007609cu, &state) && state == 7);
    if (which == CLOSE_PRELOADED) {
        metadata_put(0x8007609cu, 8);
    } else {
        metadata_at(&cpu, 0x8002b29cu);
        cpu.r[31] = 0x8002b0ecu; cpu.r[29] = 0x1f800368u;
        cpu.r[1] = 0x80070000u;
        cpu.r[3] = 8u;
        uint64_t before_cycles = cycles;
        CHECK(formatter_step(&machine, &cpu) && cycles == before_cycles + 3u);
    }
    CHECK(musashi_boot_read32(&machine, 0x8007609cu, &state) && state == 8);
    metadata_at(&cpu, 0xb0u);
    cpu.r[9] = 0x36; cpu.r[4] = 2; cpu.r[31] = 0x800603e8u; cpu.r[29] = 0x1f800350u;
    if (which == CLOSE_NO_WITNESS) {
        CHECK(!file->captures && owner->menu_metadata.stage == MAIN12_METADATA_NONE);
    } else {
        CHECK(file->captures == 1 && owner->menu_metadata.stage == MAIN12_METADATA_READ);
        CHECK(owner->menu_metadata.open_id == file->open_id && owner->menu_metadata.read_id == file->read_id);
        CHECK(!memcmp(owner->menu_metadata.bytes, file->payload, sizeof file->payload));
        CHECK(main12_closing_read(&machine, &cpu) == file->read_id);
        ++file->open_id; CHECK(!main12_closing_read(&machine, &cpu)); --file->open_id;
        ++file->read_id; CHECK(!main12_closing_read(&machine, &cpu)); --file->read_id;
        machine.bytes[0x75a40u + 639u] ^= 1;
        CHECK(!main12_closing_read(&machine, &cpu));
        machine.bytes[0x75a40u + 639u] ^= 1;
    }
    if (which == CLOSE_NEW_EVENT) {
        int32_t handle, status;
        const uint64_t previous = file->bank.allocation_serial[7];
        CHECK(musashi_bios_events_close(&file->bank, (int32_t)0xf1000007u, &status));
        CHECK(musashi_bios_events_open(&file->bank, 0xf0000011u, 0x2000, 0x1000,
                                      0x800617ccu + 7u * 20u, &handle));
        CHECK((uint32_t)handle == 0xf1000007u && file->bank.allocation_serial[7] != previous);
        CHECK(musashi_bios_events_enable(&file->bank, handle, &status));
        CHECK(!main12_closing_read(&machine, &cpu));
    }
    MusashiCallbackDevice pause = callbacks;
    pause.checkpoint = metadata_stop_after_vector;
    cpu.callback_device = &pause;
    MusashiResetGraphPrefixStop stop = {0};
    int32_t result = 0;
    CHECK(run_startup_cpu_loop(&machine, &cpu, NULL, NULL, &result, &stop, NULL, NULL, NULL) ==
          MUSASHI_RESETGRAPH_PREFIX_INVALID_INPUT);
    CHECK(stop.call_address == 0x800603e8u && cpu.pc == stop.call_address && file->closes == 1);
    CHECK(cpu.r[2] == (which == CLOSE_FAILED ? UINT32_MAX : 2u) && !owner->directory_fd_token);
    CHECK(file->closed == (which != CLOSE_FAILED));
    cpu.callback_device = &callbacks;
    CHECK(owner->menu_metadata.stage == (clean_close || which == CLOSE_PRELOADED ?
          MAIN12_METADATA_CLOSED : MAIN12_METADATA_NONE));

    /* Use the guest service caller stack and the synthetic card classification.
     * Every slot load and store below executes from the mounted program. */
    uint32_t slot_status = 0;
    for (unsigned slot = 0; slot < MAIN12_ROW_COUNT; ++slot) {
        unsigned bit = 1u << slot;
        unsigned status = (file->damaged_mask & bit) ? 2u : !!(file->occupied_mask & bit);
        slot_status |= status << (8u * slot);
    }
    metadata_put(0x800760a0u, slot_status);
    cpu.r[19] = MAIN12_MAILBOX; cpu.r[31] = 0x8002be1cu; cpu.r[29] = 0x1f800368u;
    metadata_at(&cpu, 0x8002c2c8u); cpu.r[2] = 1;
    refused_mailbox_step(cpu); /* Ready cannot precede menu state 8. */
    metadata_at(&cpu, 0x8002c2b8u); cpu.r[2] = 8;
    if (!clean_close) { refused_mailbox_step(cpu); return; }
    machine.bytes[0x75a40u] ^= 1;
    refused_mailbox_step(cpu);
    machine.bytes[0x75a40u] ^= 1;
    cpu.r[19] = 0x8007609bu; /* Displaced destination already contains 8. */
    refused_mailbox_step(cpu);
    cpu.r[19] = MAIN12_MAILBOX;
    metadata_put(0x8007609cu, 0x10008u);
    refused_mailbox_step(cpu);
    metadata_put(0x8007609cu, 8);
    MetadataClock clock = {0};
    MusashiExecutionClock hold = {&clock, metadata_store_clock};
    cpu.clock = &hold;
    refused_mailbox_step(cpu);
    CHECK(clock.calls == 1 && clock.cost == 3);
    clock.alter_payload = 1;
    CHECK(!formatter_step(&machine, &cpu) && cpu.pc == 0x8002c2b8u);
    CHECK(clock.calls == 2 && clock.cost == 6 && owner->menu_metadata.stage == MAIN12_METADATA_CLOSED);
    CHECK(owner->mailbox[1] == 7 && machine.bytes[(MAIN12_MAILBOX & 0x1fffffu) + 1] == 7);
    machine.bytes[0x75a40u + 639u] ^= 1;
    cpu.clock = &clock_source;
    unsigned start = clock_calls;
    uint64_t start_cycles = cycles;
    CHECK(formatter_step(&machine, &cpu) && owner->menu_metadata.stage == MAIN12_METADATA_STATE);
    CHECK(owner->mailbox[1] == 8 && machine.bytes[(MAIN12_MAILBOX & 0x1fffffu) + 1] == 8);
    CHECK(clock_calls == start + 1u && cycles == start_cycles + 3u);
    const uint32_t store_pc[] = {0x8002c2c8u, 0x8002c2dcu, 0x8002c2ecu, 0x8002c2fcu};
    for (unsigned slot = 0; slot < MAIN12_ROW_COUNT; ++slot) {
        while (cpu.pc < store_pc[slot]) CHECK(formatter_step(&machine, &cpu));
        CHECK(cpu.pc == store_pc[slot]);
        FormatterCpu displaced = cpu;
        displaced.r[19]++;
        refused_mailbox_step(displaced);
        unsigned value = (uint8_t)(slot_status >> (8u * slot));
        metadata_put(0x800760a0u, slot_status ^ (1u << (8u * slot)));
        refused_mailbox_step(cpu);
        CHECK(!main12_mailbox_store(&machine, &cpu, MAIN12_MAILBOX + 4u + slot,
                                    (uint8_t)(value ^ 1u), 0));
        metadata_put(0x800760a0u, slot_status);
        unsigned calls = clock_calls;
        uint64_t previous_cycles = cycles;
        CHECK(formatter_step(&machine, &cpu));
        CHECK(clock_calls == calls + 1u && cycles == previous_cycles + 3u);
        CHECK(owner->mailbox[4u + slot] == value);
        CHECK(machine.bytes[(MAIN12_MAILBOX & 0x1fffffu) + 4u + slot] == value);
        CHECK(owner->menu_metadata.stage == MAIN12_METADATA_STATE + slot + 1u);
    }
    CHECK(owner->menu_metadata.stage == MAIN12_METADATA_READY);
    check_file_row_display(cpu, file, which == CLOSE_ROW_CHANGED);
    if (which == CLOSE_OK) printf("Published four file statuses (occupied %x, damaged %x).\n",
                                  file->occupied_mask, file->damaged_mask);
}

static void check_room_initializer(MusashiCdDevice *files, MusashiEventDevice *polls)
{
    /* Reuse the mounted File 1 fixture's real event leases. Supply a synthetic
     * completed restore so the test can start at the flag reader's return. */
    owner->restore.files = files;
    owner->restore.events = files->metadata_events;
    owner->restore.open_id = owner->menu_metadata.open_id + 1u;
    memcpy(owner->restore.allocation, owner->menu_metadata.allocation, sizeof owner->restore.allocation);
    owner->restore.closed = owner->restore.consumed = owner->restore.request_cleared = 1;
    owner->restore.dismissed = owner->restore.exit_written = owner->restore.foreground_result_written = 1;
    owner->restore.copied = MAIN12_RESTORE_SIZE;
    owner->restore.label = 1;
    owner->display.confirm_phase = 2;
    memcpy(owner->restore.bytes, machine.bytes + 0x75cc0u, sizeof owner->restore.bytes);
    memcpy(musashi_boot_ram_span(&machine, MAIN12_RESTORE_BUFFER, MAIN12_RESTORE_SIZE),
           owner->restore.bytes, MAIN12_RESTORE_SIZE);
    metadata_put(MAIN12_CONFIRM_LABEL, 21u);
    metadata_put(MAIN12_LOAD_REQUEST, 0u);
    CHECK(musashi_boot_write16(&machine, MAIN12_FILE_RESULT, 0xffffu));
    CHECK(musashi_boot_write16(&machine, MAIN12_MENU_STATUS, 0xffffu));
    CHECK(musashi_boot_write16(&machine, 0x80115112u, 4u));
    CHECK(musashi_boot_write16(&machine, 0x80115118u, 0u));
    machine.bytes[0xb9a15u] = 0;

    const uint32_t top = 0x801fff20u, member = 0x800af630u + 0xa3dau;
    FormatterCpu cpu = entry(0x80145b44u);
    cpu.cd_device = files;
    cpu.event_device = polls;
    cpu.main12_foreground_owner = owner;
    cpu.main12_foreground_sp = top;
    cpu.main12_room_next = cpu.r[31] = cpu.pc;
    cpu.main12_room_resuming = 1;
    cpu.r[16] = 2u;
    cpu.r[29] = top - 80u;
    metadata_put(top - 8u, MAIN12_MENU_EXIT_RETURN);
    metadata_put(top - 32u, MAIN12_ROOM_RETURN);
    metadata_put(top - 56u, MAIN12_INIT_RETURN);
    metadata_put(top - 60u, cpu.main12_room_s1);
    metadata_put(top - 64u, cpu.main12_room_s0);
    CHECK(musashi_boot_write16(&machine, member, 0x1234u));

    for (unsigned steps = 0; cpu.pc != 0x80145bdcu && steps < 32u; ++steps)
        CHECK(formatter_step(&machine, &cpu));
    CHECK(cpu.pc == 0x80145bdcu && cpu.r[17] == 0x800af630u &&
          cpu.r[1] == 0x800bf630u && cpu.r[2] == 0x3000u);
    FormatterCpu stale = cpu;
    stale.r[1] -= 0x30u;
    uint64_t before_cycles = cycles;
    memcpy(&before_step, &machine, sizeof machine);
    CHECK(!formatter_step(&machine, &stale) && stale.pc == cpu.pc && cycles == before_cycles);
    CHECK(!memcmp(&before_step, &machine, sizeof machine));

    FileRowClock change = {musashi_boot_ram_span(&machine, top - 56u, 1u), 0};
    MusashiExecutionClock changed_return = {&change, change_file_row_sentinel};
    stale = cpu;
    stale.clock = &changed_return;
    CHECK(change.address && !formatter_step(&machine, &stale) && change.calls == 1u && stale.pc == cpu.pc);
    *change.address ^= 1u;
    CHECK(!memcmp(&before_step, &machine, sizeof machine));
    CHECK(musashi_boot_write16(&before_step, member, 0x3000u));
    CHECK(formatter_step(&machine, &cpu) && cpu.pc == 0x80145be0u);
    CHECK(!memcmp(&before_step, &machine, sizeof machine));
    for (unsigned steps = 0; cpu.pc != MAIN12_ROOM_RETURN && steps < 24u; ++steps)
        CHECK(formatter_step(&machine, &cpu));
    CHECK(cpu.pc == MAIN12_ROOM_RETURN && !cpu.main12_room_next && cpu.r[29] == top - 24u);
}

static void check_metadata_case(enum MetadataCloseCase which, unsigned occupied_mask, unsigned damaged_mask)
{
    MetadataFile file = {.open_id=37, .read_id=89, .occupied_mask=occupied_mask, .damaged_mask=damaged_mask, .fail_close=which == CLOSE_FAILED,
                         .change_at_close=which == CLOSE_CHANGED}, foreign = {0};
    MusashiCdDevice files = {.userdata=&file, .file_vector=metadata_vector,
        .metadata_events=&file.bank, .metadata_descriptor=metadata_id,
        .copy_completed_metadata=which == CLOSE_NO_WITNESS ? NULL : metadata_copy};
    MusashiEventDevice polls = {.userdata=&file, .b0_0b=metadata_poll, .registered_owner=metadata_poll_owner};
    MusashiGuestIrqCard card = {.memory=&machine, .files=&files, .events=&polls, .registered_events=&file.bank};
    seed_machine();
    install_by_guest();
    fixture_irq_vblank_setup(&machine);
    metadata_put(0x8006cbb4u, 0x800616d0u);
    metadata_put(0x80072a34u, 0);
    metadata_put(0x80078d78u, 0x800604c4u);
    metadata_put(0x80078c80u, 5);
    metadata_put(0x80078c88u, 0);
    metadata_put(0x80078d38u, 0);
    metadata_put(0x80078c90u, 2);
    metadata_put(0x80078c94u, 512);
    metadata_put(0x80078c98u, 640);
    metadata_put(0x80078c9cu, 0x80075a40u);
    for (unsigned i = 0; i < 8; ++i) {
        const uint32_t spec[] = {4, 0x8000, 0x100, 0x2000};
        int32_t handle, status;
        CHECK(musashi_bios_events_open(&file.bank, i < 4 ? 0xf4000001u : 0xf0000011u,
            spec[i % 4], 0x1000u, 0x800617ccu + 20*i, &handle));
        CHECK(musashi_bios_events_enable(&file.bank, handle, &status));
        metadata_put(0x80078c3cu + 4*i, (uint32_t)handle);
        metadata_put(0x80078c5cu + 4*i, i == 4);
    }
    uint8_t *row = musashi_boot_ram_span(&machine, 0x80076044u, sizeof owner->directory_row);
    CHECK(row);
    memset(row, 0, sizeof owner->directory_row);
    memcpy(row, "TEST", 4);
    FormatterCpu foreground = metadata_foreground(&files, &polls, row);
    metadata_put(0x8007609cu, 7);
    CHECK(owner->directory_fd_token == file.open_id);

    CHECK(irq_metadata_entry(&irq, &card, 0));
    row[0] ^= 1u;
    CHECK(!irq_metadata_entry(&irq, &card, 0));
    /* A refused IRQ has already run its resident prologue. Isolate this
     * negative case from the independent successful route below. */
    memcpy(&before_step, &machine, sizeof machine);
    metadata_irq(&card, 0);
    memcpy(&machine, &before_step, sizeof machine);
    row[0] ^= 1u;
    MusashiCdDevice second = files; /* Same descriptor number in a different device. */
    card.files = &second;
    CHECK(!irq_metadata_entry(&irq, &card, 0));
    card.files = &files;
    polls.userdata = &foreign;
    CHECK(!irq_metadata_entry(&irq, &card, 0));
    polls.userdata = &file;
    uint8_t *distant = musashi_boot_ram_span(&machine, 0x80060610u, 1u);
    CHECK(distant);
    *distant ^= 1u;
    CHECK(!irq_metadata_entry(&irq, &card, 0));
    *distant ^= 1u;
    CHECK(!file.seeks && !file.reads && !file.polls && !owner->metadata_read.phase);

    unsigned seek_steps = metadata_irq(&card, 1);
    CHECK(file.seeks == 1 && !file.reads && !file.polls && owner->metadata_read.phase == 20);
    ++file.bank.allocation_serial[7];
    CHECK(!irq_metadata_entry(&irq, &card, 20));
    --file.bank.allocation_serial[7];
    metadata_put(0x80078c9cu, 0x80075a44u);
    CHECK(!irq_metadata_entry(&irq, &card, 20));
    metadata_put(0x80078c9cu, 0x80075a40u);
    unsigned read_steps = metadata_irq(&card, 1);
    CHECK(file.reads == 1 && file.polls == 8 && file.pending && owner->metadata_read.phase == 30);
    metadata_put(0x80078c5cu, 1);
    CHECK(!irq_metadata_entry(&irq, &card, 30)); /* The completion flag alone cannot make the request ready. */
    file.pending = 0;
    file.position = 1152;
    if (which == CLOSE_OK || which == CLOSE_ROW_CHANGED) {
        /* Remembered File 4 on the full card; other fixtures exercise the four cursor values. */
        file.payload[0] = (uint8_t)(occupied_mask & 3u);
        /* Each 128-byte summary contains a 16-byte header and a checksummed body. */
        for (unsigned slot = 0; slot < MAIN12_ROW_COUNT; ++slot) {
            if (!(occupied_mask & (1u << slot))) continue;
            uint8_t *summary = file.payload + 128u * (slot + 1u);
            summary[0] = 0xffu;
            summary[16u + 78u] = 1u;
            summary[2] = (damaged_mask & (1u << slot)) ? 2u : 1u;
        }
    } else {
        for (unsigned i = 0; i < sizeof file.payload; ++i) file.payload[i] = (uint8_t)(i*11u + 9u);
    }
    memcpy(machine.bytes + 0x75a40u, file.payload, sizeof file.payload);
    unsigned complete_steps = metadata_irq(&card, 1);
    CHECK(file.seeks == 1 && file.reads == 1 && file.polls == 12 && !owner->metadata_read.phase);
    CHECK(cd_irq_ram_word(&machine, 0x80078c80u, 0) && cd_irq_ram_word(&machine, 0x80078c88u, 1));
    CHECK(cd_irq_ram_word(&machine, 0x80072a34u, UINT32_MAX));
    metadata_close(foreground, &file, which);
    if (which == CLOSE_OK && occupied_mask == 1u && !damaged_mask)
        check_room_initializer(&files, &polls);
    if (which == CLOSE_OK) printf("Metadata IRQ returned after %u/%u/%u guest steps with one seek, one read and 12 event polls.\n",
           seek_steps, read_steps, complete_steps);
}

static void check_metadata_irq(void)
{
    check_metadata_case(CLOSE_NO_WITNESS, 1u, 0u);
    check_metadata_case(CLOSE_NEW_EVENT, 1u, 0u);
    check_metadata_case(CLOSE_FAILED, 1u, 0u);
    check_metadata_case(CLOSE_CHANGED, 1u, 0u);
    check_metadata_case(CLOSE_PRELOADED, 1u, 0u);
    check_metadata_case(CLOSE_ROW_CHANGED, 1u, 0u);
    for (unsigned mask = 0; mask < 16u; ++mask) check_metadata_case(CLOSE_OK, mask, 0u);
    check_metadata_case(CLOSE_OK, 15u, 15u);
    for (unsigned slot = 0; slot < MAIN12_ROW_COUNT; ++slot)
        check_metadata_case(CLOSE_OK, 15u, 1u << slot);
}
