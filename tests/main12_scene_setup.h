#include "main12_scene_oracle.h"

static unsigned scene_clear_collisions;

static int count_scene_clear(MusashiNativeLaneCall *call)
{
    (void)call;
    ++scene_clear_collisions;
    return 1;
}

/* Damage one mounted byte on a copy of the live CPU. Restore both RAM
 * images so the independent model can continue from the same boundary. */
static void scene_reject_byte(FormatterCpu *cpu, uint32_t address)
{
    uint8_t *byte = musashi_boot_ram_span(&machine, address, 1u);
    FormatterCpu trial = *cpu;
    CHECK(byte);
    *byte ^= 1u;
    uint32_t leaked;
    if (formatter_fetch(&machine, &trial, &leaked)) {
        fprintf(stderr, "Scene at %08x accepted a changed byte at %08x\n", cpu->pc, address);
        exit(1);
    }
    expect_refused(&machine, &trial);
    *byte ^= 1u;
    memcpy(&before_step, &machine, sizeof machine);
}

static void scene_check_ownership(FormatterCpu *cpu)
{
    const uint32_t sites[] = {
        0x800d1d24u, 0x80128158u + 504u,
        SCENE_TEST_SETUP + 228u, SCENE_TEST_CHILD + 76u, SCENE_TEST_CLEAR + 160u,
        cpu->main12_foreground_sp - 8u
    };
    for (unsigned i = 0; i < sizeof sites / sizeof sites[0]; ++i)
        scene_reject_byte(cpu, sites[i]);
    FormatterCpu bad = *cpu;
    MusashiMain12Owner foreign = *owner;
    bad.main12_foreground_owner = &foreign;
    expect_refused(&machine, &bad);
    bad = *cpu;
    bad.r[31] += 4u;
    expect_refused(&machine, &bad);
    bad = *cpu;
    musashi_boot_select_overlay_0010_words(0);
    expect_refused(&machine, &bad);
    musashi_boot_select_overlay_0010_words(1);
}

static void scene_check_charge(FormatterCpu *cpu)
{
    /* Damage the child's last original word while the resident store is
     * charged. The current store and continuation must stay untouched. */
    FileRowClock change = {musashi_boot_ram_span(&machine, SCENE_TEST_CHILD + 76u, 1u), 0};
    MusashiExecutionClock clock = {&change, change_file_row_sentinel};
    FormatterCpu bad = *cpu;
    bad.clock = &clock;
    const FormatterCpu start = bad;
    CHECK(change.address && !formatter_step(&machine, &bad) && change.calls == 1u);
    CHECK(!memcmp(&start, &bad, sizeof bad));
    *change.address ^= 1u;
    CHECK(!memcmp(&before_step, &machine, sizeof machine));
}

/* An alternate SC02 member shares both addresses and the child's complete
 * body. Exercise an actual scene selection before restoring mounted MAIN. */
static unsigned scene_overlay_calls;

static int count_scene_overlay(MusashiNativeLaneCall *call)
{
    (void)call;
    ++scene_overlay_calls;
    return 1;
}

static void check_alternate_scene(const char *path)
{
    const uint32_t pcs[] = {SCENE_TEST_SETUP, SCENE_TEST_CHILD};
    const unsigned lengths[] = {232u, 80u};
    const size_t size = 588215u;
    memcpy(&other_machine, &machine, sizeof machine);
    uint8_t *source = musashi_boot_ram_span(&machine, load_address, size);
    FILE *file = fopen(path, "rb");
    CHECK(source && file && fread(source, 1, size, file) == size && fgetc(file) == EOF &&
          !ferror(file) && !fclose(file));
    check_digest(source, size, "5bb5b6364206ccd0c47d6cf6a1dc2627e13bbef3cda40d618103b92ccfef4db6");
    FormatterCpu on_scene = entry(load_address);
    CHECK(formatter_step(&machine, &on_scene) && g_overlay_sc02_0031_words);
    MusashiNativeLaneOverlayEntry peers[2];
    for (unsigned i = 0; i < 2; ++i) {
        uint64_t scene_digest, main_digest;
        CHECK(span_digest(&machine, pcs[i], lengths[i]/4u, &scene_digest) &&
              span_digest(&other_machine, pcs[i], lengths[i]/4u, &main_digest));
        CHECK((main_digest == scene_digest) == (i == 1));
        peers[i] = (MusashiNativeLaneOverlayEntry){
            {pcs[i], 2, count_scene_overlay, "SC02 scene probe", 0, 0}, lengths[i], scene_digest, "sc02_0031"
        };
    }
    CHECK(musashi_native_lane_install_overlay(peers, 2) == 2);
    const unsigned start = scene_overlay_calls;
    for (unsigned i = 0; i < 2; ++i) {
        uint32_t fetched, mounted;
        FormatterCpu active = entry(pcs[i]);
        CHECK(formatter_fetch(&machine, &active, &fetched) &&
              musashi_boot_read32(&machine, pcs[i], &mounted) && mounted == fetched);
        CHECK(formatter_step(&machine, &active) && active.pc == return_pc && scene_overlay_calls == start + i + 1u);
    }
    memcpy(&machine, &other_machine, sizeof machine);
    unsigned unexpected = 0;
    for (unsigned damage = 0; damage < 2; ++damage) {
        /* The scene flag still describes the preceding SC02/0031 image. */
        uint8_t *byte = musashi_boot_ram_span(&machine, load_address + 504u, 1);
        CHECK(byte && g_overlay_sc02_0031_words);
        if (damage) *byte ^= 1u;
        for (unsigned i = 0; i < 2; ++i) {
            FormatterCpu inactive = entry(pcs[i]);
            uint32_t fetched;
            int fetched_scene = formatter_fetch(&machine, &inactive, &fetched);
            int found_peer = lane_find_overlay(&machine, pcs[i]) != NULL;
            if (fetched_scene || found_peer) {
                fprintf(stderr, "Inactive scene PC %08x damage=%u fetch=%d optional=%d\n",
                        pcs[i], damage, fetched_scene, found_peer);
                ++unexpected;
            } else {
                expect_refused(&machine, &inactive);
            }
        }
        if (damage) *byte ^= 1u;
    }
    CHECK(!unexpected && scene_overlay_calls == start + 2u);
    puts("The mounted SC02/0031 scene dispatched both peers and the stale MAIN selection refused them.");
}

/* Four nonzero bytes identify a word, its destination, and its position.
 * Two base-255 digits keep both position bytes live even at word zero. */
static uint32_t scene_word_tag(unsigned region, unsigned word)
{
    return (uint32_t)'W' | ((region + 1u) << 8) |
           ((word / 255u + 1u) << 16) | ((word % 255u + 1u) << 24);
}

/* The three small records and the two child records are contiguous. Guard
 * each individual call for its own duration, then restore its neighbors
 * before the caller or the next clear has any reason to use them. */
static void scene_clear_edges(uint32_t start, unsigned size, uint8_t saved[2][4], int entering)
{
    static const uint8_t marks[2][4] = {{'L', 'E', 'F', 'T'}, {'R', 'G', 'H', 'T'}};
    for (unsigned side = 0; side < 2; ++side) {
        const uint32_t address = side ? start + size : start - 4u;
        uint8_t *live = musashi_boot_ram_span(&machine, address, 4u);
        uint8_t *model = musashi_boot_ram_span(&before_step, address, 4u);
        CHECK(live && model && !memcmp(live, model, 4u));
        if (entering) memcpy(saved[side], live, 4u);
        else CHECK(!memcmp(live, marks[side], 4u));
        const uint8_t *replacement = entering ? marks[side] : saved[side];
        memcpy(live, replacement, 4u);
        memcpy(model, replacement, 4u);
    }
}

static void check_scene_setup(FormatterCpu *cpu, int canaries)
{
    static const uint32_t destinations[] = {
        0x80127520u, 0x801269f0u, 0x80126a3cu, 0x80126a88u, 0x8011da80u, 0x8011da90u
    };
    static const unsigned sizes[] = {3068u, 76u, 76u, 76u, 16u, 16u};
    static const uint32_t returns[] = {
        0x80137918u, 0x80137938u, 0x80137938u, 0x80137938u, 0x8013a888u, 0x8013a888u
    };
    CHECK(cpu->pc == SCENE_TEST_CALL && cpu->npc == cpu->pc + 4u &&
          !cpu->delay_slot && !cpu->branch_pc && cpu->main12_foreground_owner == owner);
    const unsigned peer_start = scene_clear_collisions;
    if (mapped_window) {
        const char *scene = getenv("MUSASHI_TEST_SC02_ROOM_MEMBER");
        if (scene && *scene) check_alternate_scene(scene);
        MusashiNativeLaneEntry native = {SCENE_TEST_CLEAR, 82, count_scene_clear, "resident clear probe", 2, 0};
        CHECK(musashi_native_lane_install(&native, 1) == 1);
    }
    FormatterCpu detached = entry(SCENE_TEST_SETUP);
    detached.r[29] = cpu->r[29];
    detached.r[31] = SCENE_TEST_RETURN;
    detached.main12_foreground_sp = cpu->main12_foreground_sp;
    detached.main12_foreground_owner = owner;
    expect_refused(&machine, &detached);
    detached.pc = SCENE_TEST_CHILD;
    detached.npc = detached.pc + 4u;
    detached.r[31] = 0x801379b8u;
    expect_refused(&machine, &detached);
    if (canaries) {
        for (unsigned i = 0; i < 6; ++i) {
            CHECK((sizes[i] & 3u) == 0u);
            for (unsigned word = 0; word < sizes[i] / 4u; ++word)
                CHECK(musashi_boot_write32(&machine, destinations[i] + 4u * word, scene_word_tag(i, word)));
        }
    }
    SceneOracle model = {.pc = cpu->pc, .npc = cpu->npc};
    memcpy(model.r, cpu->r, sizeof model.r);
    memcpy(&before_step, &machine, sizeof machine);
    const unsigned start = clock_calls;
    const uint64_t begin_cycles = cycles;
    unsigned clear = 0, child = 0, halfword_stores = 0, zero_words = 0;
    unsigned completed = 0;
    uint8_t saved_edges[2][4];
    uint32_t changed_bytes = 0;
    int saw_return = 0, checked_charge = 0;
    while (cpu->pc != MAIN12_ROOM_RETURN + 8u && model.instructions < 3600u) {
        const uint32_t pc = model.pc;
        if (completed < clear && pc == returns[completed]) {
            if (canaries) {
                uint8_t *bytes = musashi_boot_ram_span(&machine, destinations[completed], sizes[completed]);
                CHECK(bytes);
                for (unsigned j = 0; j < sizes[completed]; ++j) CHECK(bytes[j] == 0);
                scene_clear_edges(destinations[completed], sizes[completed], saved_edges, 0);
            }
            ++completed;
        }
        if (pc == SCENE_TEST_CLEAR) {
            CHECK(completed == clear && clear < 6u && model.r[4] == destinations[clear] && model.r[5] == sizes[clear] &&
                  model.r[31] == returns[clear]);
            if (canaries) scene_clear_edges(destinations[clear], sizes[clear], saved_edges, 1);
            ++clear;
        }
        if (pc == SCENE_TEST_CHILD) ++child;
        if (pc == SCENE_TEST_RETURN) {
            CHECK(!saw_return && clear == 6u && child == 1u && model.instructions == 3589u &&
                  model.cycles == 8050u && model.reads == 20u && model.writes == 852u);
            saw_return = 1;
        }
        uint32_t mounted, fetched;
        MusashiCpuContext context;
        CHECK(cpu->pc == pc && cpu->npc == model.npc && cpu->branch_pc == model.branch &&
              cpu->delay_slot == (model.branch != 0) && !memcmp(cpu->r, model.r, sizeof model.r));
        CHECK(musashi_boot_read32(&machine, pc, &mounted));
        if (!formatter_fetch(&machine, cpu, &fetched)) {
            fprintf(stderr, "Scene setup fetch refused at %08x after %u instructions\n", pc, model.instructions);
            exit(1);
        }
        CHECK(mounted == fetched && musashi_boot_cpu_context(cpu, MUSASHI_CPU_CONTEXT_SOURCE, &context) &&
              context.gpr == cpu->r && context.pc == pc && context.npc == model.npc &&
              context.instruction_valid && context.instruction == mounted &&
              context.branch_pc == model.branch && context.delay_slot == cpu->delay_slot);
        if (pc == SCENE_TEST_SETUP) scene_check_ownership(cpu);
        if (pc == SCENE_TEST_CLEAR) {
            MusashiCpuContext host;
            CHECK(!musashi_boot_continuation_host_context(cpu, &host));
            FormatterCpu other = entry(SCENE_TEST_CLEAR);
            CHECK(musashi_boot_continuation_host_context(&other, &host) &&
                  host.provenance == MUSASHI_CPU_CONTEXT_HOST_BIOS_SERVICE);
        }
        if (pc == 0x8001677cu && !checked_charge) {
            scene_check_charge(cpu);
            checked_charge = 1;
        }
        if (pc == 0x8013a898u || pc == 0x801379b8u)
            scene_reject_byte(cpu, cpu->r[29] + (pc == 0x8013a898u ? 24u : 32u));
        scene_oracle_issue(&model, &before_step);
        if (model.store) {
            uint8_t *bytes = scene_oracle_bytes(&machine, model.address, model.width);
            uint8_t *expected = scene_oracle_bytes(&before_step, model.address, model.width);
            for (unsigned i = 0; i < model.width; ++i) changed_bytes += bytes[i] != expected[i];
            if (model.width == 2u) ++halfword_stores;
            if (pc >= SCENE_TEST_CLEAR && pc < SCENE_TEST_CLEAR + 164u) {
                CHECK(model.width == 4u && model.data == 0);
                if (canaries) {
                    CHECK(completed < 6u && clear == completed + 1u);
                    uint32_t offset = model.address - destinations[completed], before;
                    CHECK(offset < sizes[completed] && !(offset & 3u));
                    CHECK(musashi_boot_read32(&machine, model.address, &before) &&
                          before == scene_word_tag(completed, offset / 4u));
                }
                ++zero_words;
            }
        }
        /* The normal call dispatcher and an independent direct step must
         * both retire a single instruction at the registered clear. */
        const int by_call = pc == SCENE_TEST_CLEAR && (clear & 1u);
        if (!(by_call ? formatter_call(&machine, cpu, NULL, NULL) : formatter_step(&machine, cpu))) {
            fprintf(stderr, "Scene setup step refused at %08x before return\n", pc);
            exit(1);
        }
        CHECK(cpu->pc == model.pc && cpu->npc == model.npc && cpu->branch_pc == model.branch &&
              cpu->delay_slot == (model.branch != 0) && !memcmp(cpu->r, model.r, sizeof model.r));
        CHECK(clock_calls == start + model.instructions && cycles == begin_cycles + model.cycles);
        CHECK(!memcmp(&machine, &before_step, sizeof machine));
    }
    CHECK(saw_return && completed == 6u && checked_charge && model.instructions == 3593u && model.cycles == 8059u &&
          model.reads == 21u && model.writes == 852u && model.loads_delayed == 21u &&
          model.load_uses == 0u && halfword_stores == 12u && zero_words == 832u);
    CHECK(cpu->pc == MAIN12_ROOM_RETURN + 8u && cpu->main12_foreground_owner == owner);
    for (unsigned i = 0; i < 4u; ++i) {
        uint32_t word;
        CHECK(cpu->main12_foreground_owner == owner && cpu->pc == 0x8017bfe8u + 4u*i &&
              formatter_fetch(&machine, cpu, &word) && word == scene_oracle_read(&before_step, cpu->pc, 4u));
        scene_oracle_issue(&model, &before_step);
        CHECK(formatter_step(&machine, cpu) && cpu->pc == model.pc && cpu->npc == model.npc &&
              cpu->branch_pc == model.branch && cpu->delay_slot == (model.branch != 0) &&
              !memcmp(cpu->r, model.r, sizeof model.r) && !memcmp(&machine, &before_step, sizeof machine));
        CHECK(clock_calls == start + model.instructions && cycles == begin_cycles + model.cycles);
    }
    CHECK(cpu->pc == MAIN12_MENU_EXIT_RETURN && !cpu->main12_foreground_owner &&
          !cpu->main12_foreground_sp && model.instructions == 3597u && model.cycles == 8068u &&
          model.reads == 22u && model.loads_delayed == 22u && model.load_uses == 0u);
    if (canaries) CHECK(changed_bytes >= 3328u);
    CHECK(scene_clear_collisions == peer_start);
    if (mapped_window) {
        /* Prove that this registered peer can actually run on an independent
         * call after the foreground frame has been released. */
        FormatterCpu other = entry(SCENE_TEST_CLEAR);
        other.r[4] = destinations[0];
        other.r[5] = sizes[0];
        const unsigned clocks = clock_calls;
        const uint64_t start_cycles = cycles;
        CHECK(formatter_call(&machine, &other, NULL, NULL) && other.pc == return_pc &&
              clock_calls == clocks + 1u && cycles == start_cycles + 82u &&
              scene_clear_collisions == peer_start + 1u && !memcmp(&machine, &before_step, sizeof machine));
        CHECK(musashi_native_lane_install_overlay(NULL, 0) == 0);
        CHECK(musashi_native_lane_install(NULL, 0) == 0);
    }
    printf("Scene setup retired six clears, %u word stores, %u changed bytes, %u original steps, %u cycles.\n",
           zero_words, changed_bytes, model.instructions, model.cycles);
}
