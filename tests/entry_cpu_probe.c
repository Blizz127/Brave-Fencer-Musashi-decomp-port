/* Pinned image, real CRT/caller words, deliberately unavailable IRQ devices.
 * This proves continuous CPU/frame behavior before the device refusal only. */
#include "musashi_boot_memory.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { EXCHANGE = 0x80043060u, EXCHANGE_RETURN = 0x800101b4u };

typedef struct Probe {
    MusashiBootMemory memory, expected;
    const void *continuation;
    unsigned checkpoints, crt_returns, graphics_entries;
    unsigned elapsed, exchange_start, exchange_steps, exchange_returns;
    int change_exchange;
} Probe;

static int advance(void *userdata, uint32_t cycles) {
    Probe *probe = userdata;
    probe->elapsed += cycles;
    return 1;
}

static int checkpoint(void *userdata, const void *continuation) {
    Probe *probe = userdata;
    assert(continuation);
    if (probe->continuation) assert(probe->continuation == continuation);
    probe->continuation = continuation;
    probe->checkpoints++;
    if (probe->crt_returns && !probe->exchange_returns) {
        MusashiCpuContext cpu;
        assert(musashi_boot_cpu_context(continuation, MUSASHI_CPU_CONTEXT_SOURCE, &cpu));
        if (cpu.pc >= EXCHANGE && cpu.pc < EXCHANGE + 24u) {
            static const unsigned clocks[] = {0, 2, 5, 7, 10, 12};
            const unsigned i = probe->exchange_steps++;
            assert(i < 6 && cpu.pc == EXCHANGE + 4u * i);
            if (i == 0) probe->exchange_start = probe->elapsed;
            assert(probe->elapsed - probe->exchange_start == clocks[i]);
            if (probe->change_exchange && i == 2) {
                uint32_t value;
                assert(musashi_boot_read32(&probe->memory, EXCHANGE+20u, &value));
                assert(musashi_boot_write32(&probe->memory, EXCHANGE+20u, value ^ 1u));
            }
        } else if (cpu.pc == EXCHANGE_RETURN) {
            assert(probe->exchange_steps == 6 && probe->elapsed - probe->exchange_start == 14);
            assert(cpu.gpr[1] == 0x80070000u && cpu.gpr[2] == 0);
            ++probe->exchange_returns;
        }
    }
    return 1;
}

static void observe(void *userdata, const MusashiEntryCpuSnapshot *cpu) {
    Probe *probe = userdata;
    uint32_t value;
    assert(cpu->r[0] == 0 && cpu->hi == 0 && cpu->lo == 0);
    if (cpu->pc == 0x80010178u) {
        probe->crt_returns++;
        assert(!memcmp(&probe->memory, &probe->expected, sizeof(probe->memory)));
        assert(cpu->npc == 0x8001017cu);
        assert(cpu->r[29] == 0x801ffff8u && cpu->r[30] == 0x801ffff8u);
        assert(cpu->r[28] == 0x80074750u && cpu->r[31] == 0x8001009cu);
        assert(cpu->r[4] == 0x800c7f08u && cpu->r[5] == 0x001374f0u);
        assert(cpu->r[2] == 0x001ffff8u && cpu->r[3] == 0x00000c00u);
    } else if (cpu->pc == 0x800141f0u) {
        probe->graphics_entries++;
        assert(cpu->r[29] == 0x801fffc0u && cpu->r[30] == 0x801fffc0u);
        assert(cpu->r[31] == 0x800101c8u && cpu->r[28] == 0x80074750u);
        assert(musashi_boot_read32(&probe->memory, 0x80062998u, &value) && value == 1);
        assert(musashi_boot_read32(&probe->memory, 0x80074778u, &value) && value == 0x3e0u);
    }
}

static void sink(void *userdata, const uint8_t *bytes, int32_t length) {
    (void)userdata;
    (void)bytes;
    assert(length >= 0);
}

int main(int argc, char **argv) {
    static Probe probe, changed, overflow;
    MusashiEntryState expected_entry;
    MusashiEntryRunStop stop;
    MusashiCallbackDevice device = {0};
    uint8_t *exe;
    FILE *file;
    assert(argc == 2);
    file = fopen(argv[1], "rb");
    if (!file) return 77;
    exe = malloc(0x65000u);
    assert(exe && fread(exe, 1, 0x65000u, file) == 0x65000u && fgetc(file) == EOF);
    assert(fclose(file) == 0);
    assert(musashi_boot_map_exe(&probe.memory, exe, 0x65000u));
    assert(musashi_boot_load_entry(&probe.expected, exe, 0x65000u, 0, &expected_entry));
    changed.memory = probe.memory;
    changed.expected = probe.expected;
    changed.change_exchange = 1;
    free(exe);
    device.userdata = &probe;
    device.checkpoint = checkpoint;
    MusashiExecutionClock clock = {&probe, advance};
    MusashiStartupPrefixDevice services = {.callback=&device, .clock=&clock};
    assert(musashi_boot_run_entry_with_devices(&probe.memory, 0, &services, sink, NULL,
        observe, &probe, &stop) == MUSASHI_RESETGRAPH_PREFIX_INVALID_INPUT);
    assert(probe.exchange_returns == 1);
    assert(probe.crt_returns == 1 && probe.graphics_entries == 1);
    assert(probe.checkpoints > 100000u);
    assert(stop.cpu.pc == stop.boundary.target_address);
    assert(stop.cpu.r[29] != 0x801fff80u); /* No old diagnostic frame reset. */
    /* Changing a later instruction invalidates the whole service at the
     * current fetch, before the following instruction can charge its clock. */
    clock.userdata = &changed;
    device.userdata = &changed;
    assert(musashi_boot_run_entry_with_devices(&changed.memory, 0, &services, sink, NULL,
        observe, &changed, &stop) == MUSASHI_RESETGRAPH_PREFIX_INVALID_INPUT);
    assert(stop.cpu.pc == EXCHANGE+8u && changed.exchange_steps == 3 && !changed.exchange_returns);
    assert(changed.elapsed - changed.exchange_start == 5);

    /* Test-owned malformed CRT stack metadata exercises ADDI's real signed
     * overflow refusal. The production image is never modified this way. */
    overflow.memory = probe.expected;
    assert(musashi_boot_write32(&overflow.memory, 0x800629bcu, 0x80000000u));
    device.userdata = &overflow;
    assert(musashi_boot_run_entry(&overflow.memory, 0, &device, NULL, sink, NULL,
        observe, &overflow, &stop) == MUSASHI_RESETGRAPH_PREFIX_INVALID_INPUT);
    assert(stop.cpu.pc == 0x80010030u && stop.cpu.npc == 0x80010034u);
    assert(stop.cpu.r[2] == 0x80000000u && stop.cpu.r[29] == 0);
    assert(overflow.crt_returns == 0 && overflow.graphics_entries == 0);
    puts("ENTRY CPU verified: exact CRT effects and continuous constructor/caller frame; IRQ device refused");
    return 0;
}
