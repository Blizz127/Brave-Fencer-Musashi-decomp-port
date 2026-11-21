#include "../pc_port/mips_formatter.c"
#include <assert.h>

enum { ENTRY_PC = 0x80012ab0u, RETURN_PC = 0x801ff180u };
static unsigned advances, refuse_at;

static int advance(void *user, uint32_t cost) {
    (void)user;
    assert(cost == 2);
    return ++advances != refuse_at;
}

static int unused_row(MusashiNativeLaneCall *call) {
    (void)call;
    abort();
}

static void run(MusashiBootMemory *memory, uint32_t a, uint32_t b, unsigned cut,
                int native, FormatterCpu phases[3]) {
    static const MusashiExecutionClock clock = {NULL, advance};
    FormatterCpu cpu = {0};
    for (unsigned i = 1; i < 32; ++i) cpu.r[i] = 0x5a000000u + i;
    cpu.r[4] = a; cpu.r[5] = b; cpu.r[31] = RETURN_PC;
    cpu.hi = 0x76543210; cpu.lo = 0xfedcba98;
    cpu.pc = ENTRY_PC; cpu.npc = ENTRY_PC + 4;
    cpu.clock = &clock;
    advances = 0; refuse_at = cut;
    if (!native) musashi_native_lane_suspend(1);
    for (unsigned i = 0; i < 3; ++i) {
        FormatterCpu before = cpu;
        /* The ordinary decoder binds its fetch source even when the clock
         * refuses. The compiled phases finish before that binding. */
        g_fetch_memory = NULL;
        int ok = formatter_step(memory, &cpu);
        assert((g_fetch_memory == NULL) == native);
        assert(advances == i + 1);
        phases[i] = cpu;
        if (i + 1 == cut) {
            assert(!ok && !memcmp(&cpu, &before, sizeof cpu));
            break;
        }
        assert(ok);
        assert(cpu.r[2] == (i < 2 ? a - b : (a - b) & 4095u));
        assert(cpu.pc == (i < 2 ? ENTRY_PC + 4u * (i + 1) : RETURN_PC));
        assert(cpu.npc == (i == 0 ? ENTRY_PC + 8 : i == 1 ? RETURN_PC : RETURN_PC + 4));
        assert(cpu.delay_slot == (i == 1));
        assert(cpu.branch_pc == (i == 1 ? ENTRY_PC + 4 : 0));
        before.r[2] = cpu.r[2];
        assert(!memcmp(cpu.r, before.r, sizeof cpu.r));
        assert(cpu.hi == before.hi && cpu.lo == before.lo);
    }
    if (!native) musashi_native_lane_suspend(0);
}

int main(int argc, char **argv) {
    extern const MusashiNativeLaneEntry musashi_native_lane_generated[];
    extern const size_t musashi_native_lane_generated_count;
    static const MusashiNativeLaneEntry unrelated = {RETURN_PC, 0, unused_row, "fixture", 0, 0};
    static const uint32_t operands[][2] = {{0, 1}, {0x80000000u, 1}, {9, 9}};
    assert(argc == 2);
    MusashiBootMemory *memory = musashi_native_lane_map_guest(sizeof(*memory), NULL);
    if (!memory) return 77;
    unsigned char *image = musashi_boot_ram_span(memory, ENTRY_PC, 12);
    FILE *f = fopen(argv[1], "rb");
    assert(f && image && fread(image, 1, 12, f) == 12 && fgetc(f) == EOF);
    fclose(f);
    assert(musashi_native_lane_generated_count == 1);
    assert(musashi_native_lane_install(musashi_native_lane_generated, 1) == 1);
    for (unsigned i = 0; i < sizeof operands / sizeof operands[0]; ++i) {
        for (unsigned cut = 0; cut <= 3; ++cut) {
            FormatterCpu reference[3] = {0}, actual[3] = {0};
            run(memory, operands[i][0], operands[i][1], cut, 0, reference);
            run(memory, operands[i][0], operands[i][1], cut, 1, actual);
            assert(!memcmp(reference, actual, sizeof actual));
        }
    }
    /* --only and --exclude may leave other native rows installed. A matching
     * extent alone cannot bypass exclusion of this row. */
    assert(musashi_native_lane_install(&unrelated, 1) == 1);
    FormatterCpu excluded = {.pc = ENTRY_PC, .npc = ENTRY_PC + 4};
    excluded.r[4] = 3; excluded.r[5] = 4;
    g_fetch_memory = NULL;
    assert(formatter_step(memory, &excluded) && g_fetch_memory == memory);
    assert(excluded.r[2] == UINT32_MAX);
    assert(musashi_native_lane_generated_count == 1);
    assert(musashi_native_lane_install(musashi_native_lane_generated, 1) == 1);
    /* Change the last byte of the compiler-produced extent; entry executes
     * its original first instruction through the ordinary decoder. */
    image[11] ^= 1;
    FormatterCpu cpu = {.pc = ENTRY_PC, .npc = ENTRY_PC + 4};
    cpu.r[4] = 3; cpu.r[5] = 4;
    g_fetch_memory = NULL;
    assert(formatter_step(memory, &cpu) && g_fetch_memory == memory);
    assert(cpu.r[2] == UINT32_MAX);
    return 0;
}
