#include "main12_scene_setup.h"

static void mount_room_caller(void)
{
    const uint8_t *row = archive + 8u + 10u * 8u;
    const uint32_t sector = little32(row), extent = 365404u;
    CHECK(little32(row + 4u) == 497664u && sector <= (sizeof archive - 497664u) / 2048u);
    const uint32_t offset = ROOM_RESET_CALLER - 0x800cedf8u;
    CHECK(offset <= extent - 112u);
    uint8_t *live = musashi_boot_ram_span(&machine, ROOM_RESET_CALLER, 112u);
    CHECK(live);
    /* The callback/foreground already own live writable data in this mount.
     * Supply only the caller text the fixture starts immediately before. */
    memcpy(live, archive + sector * 2048u + 0x20000u + offset, 112u);
    musashi_boot_select_overlay_0010_words(1);
}

static int pause_slot_clock(void *state, uint32_t cost)
{
    unsigned *remaining = state;
    if (!*remaining) return 0;
    --*remaining;
    return advance(NULL, cost);
}

static void check_slot_outcome(FormatterCpu *cpu)
{
    CHECK(cpu->pc == ROOM_RESET_ENTRY && cpu->room_reset_owner == owner);
    uint32_t regs[32];
    memcpy(regs, cpu->r, sizeof regs);
    regs[2] = UINT32_MAX;
    regs[3] = ROOM_SLOT_BASE + ROOM_SLOT_COUNT * ROOM_SLOT_SIZE;
    const uint32_t hi = cpu->hi, lo = cpu->lo;
    const uint64_t begin = cycles;
    const unsigned issued = clock_calls;
    uint8_t *slots = musashi_boot_ram_span(&machine, ROOM_SLOT_BASE, ROOM_SLOT_COUNT * ROOM_SLOT_SIZE);
    CHECK(slots);
    for (unsigned i = 0; i < ROOM_SLOT_COUNT * ROOM_SLOT_SIZE; ++i)
        slots[i] = (uint8_t)(i * 37u + 13u);
    for (unsigned i = 0; i < ROOM_SLOT_COUNT; i += 3u)
        CHECK(musashi_boot_write16(&machine, ROOM_SLOT_BASE + ROOM_SLOT_SIZE * i, 0u));
    slot_ram_access.memory = &machine;
    slot_ram_access.first = ROOM_SLOT_BASE;
    slot_ram_access.limit = ROOM_SLOT_BASE + ROOM_SLOT_COUNT * ROOM_SLOT_SIZE;
    slot_ram_access.count = 0u;
    uint8_t write_witness[2] = {0xa5u, 0x5au};
    slot_ram_access.witness_address = ROOM_SLOT_BASE;
    slot_ram_access.witness = write_witness;
    memcpy(&before_step, &machine, sizeof machine);
    for (unsigned i = 0; i < ROOM_SLOT_COUNT; ++i)
        CHECK(musashi_boot_write16(&before_step, ROOM_SLOT_BASE + ROOM_SLOT_SIZE * i, 0u));

    unsigned remaining = 133u;
    MusashiExecutionClock clock = {&remaining, pause_slot_clock};
    cpu->clock = &clock;
    while (remaining) CHECK(formatter_step(&machine, cpu));
    FormatterCpu paused = *cpu;
    MusashiBootMemory *at_pause = malloc(sizeof *at_pause);
    CHECK(at_pause);
    memcpy(at_pause, &machine, sizeof *at_pause);
    CHECK(!formatter_step(&machine, cpu));
    CHECK(!memcmp(&paused, cpu, sizeof paused) && !memcmp(at_pause, &machine, sizeof machine));
    free(at_pause);
    uint32_t fetched;
    uint8_t *code = musashi_boot_ram_span(&machine, ROOM_RESET_ENTRY, 1u);
    CHECK(code && formatter_fetch(&machine, cpu, &fetched));
    *code ^= 1u;
    CHECK(!formatter_fetch(&machine, cpu, &fetched));
    *code ^= 1u;
    CHECK(formatter_fetch(&machine, cpu, &fetched));

    cpu->clock = &clock_source;
    while (cpu->pc != ROOM_RESET_CALL + 8u && clock_calls - issued <= 517u)
        CHECK(formatter_step(&machine, cpu));
    CHECK(cpu->pc == regs[31] && cpu->npc == regs[31] + 4u && !cpu->delay_slot && !cpu->branch_pc);
    CHECK(clock_calls - issued == 517u && cycles - begin == 1162u);
    CHECK(!memcmp(cpu->r, regs, sizeof regs) && cpu->hi == hi && cpu->lo == lo);
    CHECK(!memcmp(&machine, &before_step, sizeof machine));
    CHECK(slot_ram_access.count == ROOM_SLOT_COUNT && slot_ram_access.width == sizeof(uint16_t));
    CHECK(!write_witness[0] && !write_witness[1]);
    slot_ram_access.witness = NULL;
    slot_ram_access.memory = NULL;
}

static unsigned slot_peer_calls;

static int observe_slot_peer(MusashiNativeLaneCall *call)
{
    CHECK(call->memory == &machine && musashi_native_lane_overlay_resident(
        ROOM_RESET_ENTRY, ROOM_RESET_BYTES, UINT64_C(0xb9cb72d7e1fd5c28)));
    ++slot_peer_calls;
    return 1;
}

static void check_other_slot_member(const char *path)
{
    const size_t size = 616687u;
    MusashiNativeLaneOverlayEntry peer = {
        {ROOM_RESET_ENTRY, 0u, observe_slot_peer, "SC02 slot probe", 0, 0},
        ROOM_RESET_BYTES, UINT64_C(0xb9cb72d7e1fd5c28), "sc02_0035"
    };
    CHECK(musashi_native_lane_install_overlay(&peer, 1u) == 1u);
    CHECK(!lane_find_overlay(&machine, ROOM_RESET_ENTRY));
    memcpy(&other_machine, &machine, sizeof machine);
    uint8_t *scene = musashi_boot_ram_span(&machine, load_address, size);
    FILE *file = fopen(path, "rb");
    CHECK(file && scene && fread(scene, 1u, size, file) == size && fgetc(file) == EOF &&
          !ferror(file) && !fclose(file));
    check_digest(scene, size, "6fcb360e5153a15d587216d4ae12f3ab05997a4055f3056b68b63c540085a145");
    FormatterCpu cpu = entry(ROOM_RESET_ENTRY);
    CHECK(lane_try(&machine, &cpu) && slot_peer_calls == 1u && cpu.pc == return_pc);
    *scene ^= 1u;
    CHECK(!lane_find_overlay(&machine, ROOM_RESET_ENTRY));
    CHECK(slot_peer_calls == 1u);
    memcpy(&machine, &other_machine, sizeof machine);
    CHECK(!lane_find_overlay(&machine, ROOM_RESET_ENTRY));
    CHECK(musashi_native_lane_install_overlay(NULL, 0u) == 0u);
}

static void check_mounted_slot_reset(void)
{
    seed_machine();
    install_by_guest();
    mount_room_caller();
    const char *other = getenv("MUSASHI_TEST_SC02_SLOT_MEMBER");
    if (mapped_window && other && *other) check_other_slot_member(other);
    FormatterCpu cpu = entry(ROOM_RESET_CALL);
    FormatterCpu outside = entry(ROOM_RESET_ENTRY);
    expect_refused(&machine, &outside);
    CHECK(formatter_step(&machine, &cpu) && formatter_step(&machine, &cpu));
    check_slot_outcome(&cpu);
    musashi_boot_select_overlay_0010_words(0);
}

static void check_foreground_slot_reset(FormatterCpu *cpu)
{
    mount_room_caller();
    check_latch_missing_call(*cpu);
    for (unsigned n = 0; cpu->pc != ROOM_RESET_ENTRY && n < 300000u; ++n) {
        if (cpu->pc == ROOM_RESET_CALLER) check_latch_mode_fields(*cpu);
        CHECK(formatter_step(&machine, cpu));
    }
    check_slot_outcome(cpu);
    check_scene_setup(cpu, 1);
    musashi_boot_select_overlay_0010_words(0);
}
