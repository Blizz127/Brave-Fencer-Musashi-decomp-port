/* Exercise the mounted graphics wrapper with test child results and a caller
 * frame. The fixture covers the wrapper's return ABI and exclusive clock. */
#include "../pc_port/mips_formatter.c"
#include "startup_mounted_fixture.h"

static int graphics_test_clock(void *userdata, uint32_t cycles) {
    uint32_t *elapsed = userdata;
    *elapsed += cycles;
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 2) return 1;
    MusashiBootMemory *memory;
    int result = startup_mounted_load(argv[1], &memory);
    if (result) return result;

    uint32_t elapsed = 0;
    const MusashiExecutionClock clock = {&elapsed, graphics_test_clock};
    FormatterCpu cpu = {.clock = &clock, .pc = BFM_GRAPHICS_ENTRY,
                        .npc = BFM_GRAPHICS_ENTRY + 4u};
    for (unsigned i = 16; i < 24; ++i) cpu.r[i] = i * 12345u;
    const uint32_t stack = 0x801efff0u, caller = 0x801e0000u;
    cpu.r[29] = stack;
    cpu.r[31] = caller;

    unsigned issues = 0, calls = 0;
    while (cpu.pc != caller) {
        if (cpu.pc - STARTUP_ENTRY < STARTUP_END - STARTUP_ENTRY) {
            if (++issues > 100u || !formatter_step(memory, &cpu)) return 1;
        } else {
            /* Only complete calls made by this mounted wrapper may return. */
            if (cpu.delay_slot || cpu.npc != cpu.pc + 4u ||
                cpu.r[31] - STARTUP_ENTRY >= STARTUP_END - STARTUP_ENTRY ||
                ++calls > 5u) return 1;
            cpu.r[2] = 100u + calls;
            if (!formatter_return(&cpu)) return 1;
        }
    }

    for (unsigned i = 16; i < 24; ++i)
        if (cpu.r[i] != i * 12345u) return 1;
    return calls != 5u || issues != 18u || elapsed != 38u || cpu.r[29] != stack ||
           cpu.r[31] != caller || cpu.r[2] != 100u + calls ||
           cpu.npc != caller + 4u || cpu.delay_slot;
}
