/* Endpoint and bus checks. The player supplies the program being executed. */
#include "../pc_port/mips_formatter.c"
#include <assert.h>

extern const MusashiNativeLaneEntry musashi_native_lane_generated[];
extern const size_t musashi_native_lane_generated_count;
extern int32_t func_80012AB0(int32_t, int32_t);

enum { DIFFERENCE = 0x80012ab0u, CALLER = 0x80012a60u, EXIT_PC = 0x80070000u };

typedef struct {
    FormatterCpu *cpu;
    unsigned calls, spent, refuse_at;
    int change_input;
    MusashiCdIrqFrame *fault_on_charge;
} Clock;

static int advance(void *data, uint32_t cycles) {
    Clock *clock = data;
    if (++clock->calls == clock->refuse_at) return 0;
    clock->spent += cycles;
    if (clock->fault_on_charge) clock->fault_on_charge->faulted = 1;
    if (clock->change_input) {
        clock->cpu->r[4] = 8190u;
        clock->change_input = 0;
    }
    return 1;
}

static FormatterCpu enter(uint32_t target, uint32_t a, uint32_t b) {
    FormatterCpu cpu = {.pc=target, .npc=target + 4u, .hi=127, .lo=256};
    for (unsigned i = 1; i < 32; ++i) cpu.r[i] = i * 11u;
    cpu.r[4] = a;
    cpu.r[5] = b;
    cpu.r[29] = 0x801ff000u;
    cpu.r[31] = EXIT_PC;
    return cpu;
}

static void until_return(MusashiBootMemory *memory, FormatterCpu *cpu) {
    for (unsigned i = 0; i < 256 && cpu->pc != EXIT_PC; ++i)
        assert(formatter_step(memory, cpu));
    assert(cpu->pc == EXIT_PC && cpu->npc == EXIT_PC + 4u && !cpu->delay_slot);
}

static void difference(MusashiBootMemory *memory, uint32_t a, uint32_t b, unsigned cut) {
    FormatterCpu cpu = enter(DIFFERENCE, a, b);
    Clock clock = {.cpu=&cpu, .refuse_at=cut};
    MusashiExecutionClock binding = {&clock, advance};
    cpu.clock = &binding;
    FormatterCpu entry = cpu;
    for (unsigned step = 1; step <= 3; ++step) {
        FormatterCpu before = cpu;
        if (step == cut) {
            assert(!formatter_step(memory, &cpu));
            assert(cpu.pc == before.pc && cpu.npc == before.npc &&
                   cpu.delay_slot == before.delay_slot && cpu.branch_pc == before.branch_pc);
            assert(!memcmp(cpu.r, before.r, sizeof cpu.r) && cpu.hi == before.hi && cpu.lo == before.lo);
        }
        assert(formatter_step(memory, &cpu));
    }
    assert(cpu.pc == EXIT_PC && cpu.npc == EXIT_PC + 4u && !cpu.delay_slot);
    assert(clock.calls == 3u + (cut != 0) && clock.spent == 6u);
    entry.r[2] = (a - b) & 4095u;
    assert((uint32_t)func_80012AB0((int32_t)a, (int32_t)b) == entry.r[2]);
    assert(!memcmp(cpu.r, entry.r, sizeof cpu.r) && cpu.hi == entry.hi && cpu.lo == entry.lo);
}

static void charged_irq_fault(MusashiBootMemory *memory) {
    FormatterCpu cpu = enter(DIFFERENCE, 8, 5);
    MusashiCdIrqFrame irq = {0};
    Clock clock = {.cpu=&cpu, .fault_on_charge=&irq};
    MusashiExecutionClock binding = {&clock, advance};
    cpu.clock = &binding;
    cpu.cd_irq_frame = &irq;
    FormatterCpu before = cpu;
    assert(!formatter_step(memory, &cpu));
    assert(irq.faulted && clock.calls == 1 && clock.spent == 2);
    assert(cpu.pc == before.pc && cpu.npc == before.npc && !memcmp(cpu.r, before.r, sizeof cpu.r));
    irq.faulted = 0;
    clock.fault_on_charge = NULL;
    until_return(memory, &cpu);
    assert(cpu.r[2] == 3 && clock.spent == 8);
}

static unsigned reads, provider_calls;
static int fetch_available;
static int owns(uint32_t address, size_t size) {
    return address == DIFFERENCE && size == 4u;
}

static int read_instruction(void *data, uint32_t address, uint32_t *word) {
    assert(owns(address, 4u));
    ++reads;
    *word = *(uint32_t *)data;
    return fetch_available;
}

static int alternate(MusashiNativeLaneCall *call) {
    ++provider_calls;
    call->r[2] = 0x5678u;
    return 1;
}

static void live_owners(MusashiBootMemory *memory) {
    FormatterCpu cpu = enter(DIFFERENCE, 0, 3);
    Clock clock = {.cpu=&cpu, .change_input=1};
    MusashiExecutionClock binding = {&clock, advance};
    cpu.clock = &binding;
    until_return(memory, &cpu);
    assert(cpu.r[2] == ((8190u - 3u) & 4095u) && clock.calls == 3);

    /* An unrelated OR-immediate instruction marks which memory owner ran. */
    uint32_t replacement = 0x34083a5bu, original;
    assert(musashi_boot_read32(memory, DIFFERENCE, &original));
    assert(musashi_boot_write32(memory, DIFFERENCE, replacement));
    cpu = enter(DIFFERENCE, 7, 9);
    cpu.clock = &binding;
    cpu.npc = EXIT_PC;
    cpu.delay_slot = 1;
    cpu.branch_pc = DIFFERENCE - 4u;
    assert(formatter_step(memory, &cpu));
    assert(cpu.pc == EXIT_PC && cpu.r[8] == 0x3a5bu && clock.calls == 4);
    assert(musashi_boot_write32(memory, DIFFERENCE, original));

    MusashiBiosImageDevice bus = {.userdata=&replacement, .intersects=owns, .read32=read_instruction};
    cpu = enter(DIFFERENCE, 0, 0);
    cpu.bios_image = &bus;
    cpu.clock = &binding;
    assert(!formatter_step(memory, &cpu));
    assert(cpu.pc == DIFFERENCE && reads == 1 && clock.calls == 4);
    fetch_available = 1;
    assert(formatter_step(memory, &cpu));
    assert(cpu.r[8] == 0x3a5bu && reads == 2 && clock.calls == 5);

    MusashiNativeLaneEntry override = {DIFFERENCE, 7u, alternate, "alternate", 2, 1};
    assert(musashi_native_lane_install(&override, 1) == 1);
    cpu = enter(DIFFERENCE, 0, 0);
    cpu.clock = &binding;
    assert(formatter_step(memory, &cpu));
    assert(cpu.pc == EXIT_PC && cpu.r[2] == 0x5678u && provider_calls == 1);
    assert(clock.calls == 6 && clock.spent == 17);
}

static void caller(MusashiBootMemory *memory, uint32_t a, uint32_t b) {
    FormatterCpu reference = enter(CALLER, a, b), generated = reference;
    musashi_native_lane_suspend(1);
    until_return(memory, &reference);
    musashi_native_lane_suspend(0);
    Clock clock = {.cpu=&generated};
    MusashiExecutionClock binding = {&clock, advance};
    generated.clock = &binding;
    assert(formatter_step(memory, &generated));
    assert(generated.pc == EXIT_PC && generated.npc == EXIT_PC + 4u);
    assert(generated.r[2] == reference.r[2] && generated.r[29] == reference.r[29]);
    /* One generated parent charge, plus the individual guest child issues. */
    assert(clock.calls == 4 && clock.spent == musashi_native_lane_generated[0].cost_cycles + 6u);
}

int main(int argc, char **argv) {
    assert(argc == 2);
    FILE *file = fopen(argv[1], "rb");
    assert(file && fseek(file, 0, SEEK_END) == 0);
    long size = ftell(file);
    assert(size > 0 && fseek(file, 0, SEEK_SET) == 0);
    uint8_t *exe = malloc((size_t)size);
    assert(exe && fread(exe, 1, (size_t)size, file) == (size_t)size);
    fclose(file);
    int aliased;
    MusashiBootMemory *memory = musashi_native_lane_map_guest(sizeof *memory, &aliased);
    assert(memory && musashi_boot_map_exe(memory, exe, (size_t)size));
    free(exe);
    assert(musashi_native_lane_generated_count == 1 && musashi_native_lane_generated[0].guest_pc == CALLER);
    assert(musashi_native_lane_install_generated(musashi_native_lane_generated, 1) == 1);
    uint32_t seed = 71u;
    for (unsigned i = 0; i < 128; ++i) {
        seed = seed * 1664525u + 1013904223u;
        uint32_t a = seed;
        seed = seed * 1664525u + 1013904223u;
        difference(memory, a, seed, i % 4u);
        caller(memory, a, seed);
    }
    /* Run a timed generated parent and nested guest through the startup loop. */
    FormatterCpu cpu = enter(CALLER, 2700, 1000);
    Clock clock = {.cpu=&cpu};
    MusashiExecutionClock binding = {&clock, advance};
    static const MusashiCallbackDevice devices;
    cpu.callback_device = &devices;
    cpu.clock = &binding;
    cpu.r[31] = UINT_MAX;
    int32_t result = 0;
    MusashiResetGraphPrefixStop stop = {0};
    assert(run_startup_cpu(memory, &cpu, lane_reference_sink, NULL, &result, &stop,
                           NULL, NULL, NULL, 0) == MUSASHI_RESETGRAPH_PREFIX_COMPLETE);
    const uint32_t args[] = {2700, 1000};
    uint32_t expected;
    assert(musashi_native_lane_reference_call(memory, CALLER, args, 2u, cpu.r[29], 0,
                                              &expected, NULL));
    assert(cpu.r[2] == expected && clock.calls == 4 &&
           clock.spent == musashi_native_lane_generated[0].cost_cycles + 6u);
    charged_irq_fault(memory);
    live_owners(memory);
    return 0;
}
