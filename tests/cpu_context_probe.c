#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "musashi_boot_memory.h"

/* The test launcher links the boot-memory API and this translation unit. */
#include "../pc_port/mips_formatter.c"
#include "../pc_port/startup_bindings.c"
#include "../pc_port/gpu_dma2.c"
#include "../pc_port/callback_bindings.c"
#define MUSASHI_NATIVE_MEMSET_BINDING 1
#define MUSASHI_NATIVE_TABLE_CLEAR_BINDING 1
#include "../src/main/80016714.c"
#include "../src/main/80029044.c"
#include "../src/main/8002906c.c"

enum BranchKind { PLAIN, JUMP, CALL, JUMP_REG, CALL_REG, EQUAL, UNEQUAL, SIGN };
typedef struct Trace {
    MusashiBootMemory *memory;
    uint32_t seed_ra;
    unsigned checkpoints, host_views, sequential_slots, redirected_slots, kinds;
    unsigned checked_cases;
    int retiring_slot;
    uint32_t after_slot;
    enum BranchKind previous;
    uint32_t previous_pc, expected_target;
    const void *previous_identity;
    int met, error;
} Trace;

#define CHECK(test) do { if (!(test)) { \
    fprintf(stderr, "CPU context line %d at checkpoint %u: %s\n", \
            __LINE__, trace->checkpoints, #test); \
    trace->error = 1; return 0; } } while (0)

/* Predict the successor from the live instruction and its input registers. */
static enum BranchKind next_branch(const MusashiCpuContext *c, uint32_t *target)
{
    uint32_t w = c->instruction;
    unsigned major = w >> 26, rs = (w >> 21) & 31u, rt = (w >> 16) & 31u;
    int taken;
    enum BranchKind kind;
    switch (major) {
    case 0:
        if ((w & 63u) != 8 && (w & 63u) != 9) return PLAIN;
        *target = c->gpr[rs];
        return (w & 63u) == 8 ? JUMP_REG : CALL_REG;
    case 2: case 3:
        *target = ((c->pc + 4u) & UINT32_C(0xf0000000)) |
                  ((w & UINT32_C(0x03ffffff)) << 2);
        return major == 2 ? JUMP : CALL;
    case 4: case 5:
        kind = major == 4 ? EQUAL : UNEQUAL;
        taken = (c->gpr[rs] == c->gpr[rt]) == (major == 4);
        break;
    case 1:
        if (rt != 0 && rt != 1 && rt != 16 && rt != 17) return PLAIN;
        kind = SIGN;
        taken = (c->gpr[rs] >> 31) != (rt & 1u);
        break;
    default:
        return PLAIN;
    }
    uint32_t displacement = (w & UINT32_C(0xffff)) * 4u;
    if (w & UINT32_C(0x8000)) displacement -= UINT32_C(0x40000);
    *target = taken ? c->pc + 4u + displacement : c->pc + 8u;
    return kind;
}

typedef struct TestClock {
    unsigned cycles, calls;
    int permit;
} TestClock;

static int charge(void *user, uint32_t cycles)
{
    TestClock *clock = user;
    ++clock->calls;
    if (!clock->permit) return 0;
    clock->cycles += cycles;
    return 1;
}

static unsigned branch_case(enum BranchKind kind, const MusashiCpuContext *c)
{
    unsigned rs = c->instruction >> 21 & 31u;
    unsigned rt = c->instruction >> 16 & 31u;
    switch (kind) {
    case EQUAL: return c->gpr[rs] == c->gpr[rt] ? 1u : 2u;
    case UNEQUAL: return c->gpr[rs] != c->gpr[rt] ? 4u : 8u;
    case JUMP_REG: return 16u;
    case CALL: return 32u;
    case JUMP: return 64u;
    default: return 0u;
    }
}

/* Pick a register-only slot so its acceptance has no external side effects. */
static int plain_register_slot(uint32_t word)
{
    unsigned major = word >> 26;
    if (major == 0)
        return (word & 63u) == 0 || (word & 63u) == 2 || (word & 63u) == 33;
    return major == 9 || major == 12 || major == 13 || major == 14 || major == 15;
}

static int controlled_branch(Trace *trace, const MusashiCpuContext *live)
{
    uint32_t target = 0, slot_word;
    enum BranchKind kind = next_branch(live, &target);
    unsigned bit = branch_case(kind, live);
    if (!bit || (trace->checked_cases & bit) || live->delay_slot) return 1;
    CHECK(musashi_boot_read32(trace->memory, live->pc + 4u, &slot_word));
    if (!plain_register_slot(slot_word)) return 1;

    FormatterCpu cpu = {0};
    TestClock meter = {0, 0, 1};
    MusashiExecutionClock clock = {&meter, charge};
    MusashiCpuContext source;
    cpu.clock = &clock;
    cpu.pc = live->pc;
    cpu.npc = live->npc;
    memcpy(cpu.r, live->gpr, sizeof(cpu.r));
    CHECK(musashi_boot_cpu_context(&cpu, MUSASHI_CPU_CONTEXT_SOURCE, &source));
    CHECK(source.identity == &cpu && source.gpr == cpu.r);
    CHECK(source.pc == cpu.pc && source.npc == cpu.npc && !source.delay_slot && !source.branch_pc);
    CHECK(formatter_step(trace->memory, &cpu));
    CHECK(meter.calls == 1 && meter.cycles == 2);
    CHECK(musashi_boot_cpu_context(&cpu, MUSASHI_CPU_CONTEXT_SOURCE, &source));
    CHECK(source.delay_slot && source.pc == live->pc + 4u && source.branch_pc == live->pc);
    CHECK(source.npc == target && source.instruction_valid && source.instruction == slot_word);
    if (kind == CALL) CHECK(source.gpr[31] == live->pc + 8u);

    FormatterCpu before;
    memcpy(&before, &cpu, sizeof(cpu));
    meter.permit = 0;
    CHECK(!formatter_step(trace->memory, &cpu));
    CHECK(!memcmp(&before, &cpu, sizeof(cpu)));
    CHECK(meter.calls == 2 && meter.cycles == 2);
    meter.permit = 1;
    CHECK(formatter_step(trace->memory, &cpu));
    CHECK(meter.calls == 3 && meter.cycles == 4);
    CHECK(cpu.pc == target && cpu.npc == target + 4u && !cpu.delay_slot && !cpu.branch_pc);

    /* Reuse this registered branch as an instruction inside a delay slot. */
    cpu.pc = live->pc;
    cpu.npc = target;
    cpu.delay_slot = 1;
    cpu.branch_pc = live->pc - 4u;
    memcpy(&before, &cpu, sizeof(cpu));
    CHECK(!formatter_step(trace->memory, &cpu));
    CHECK(!memcmp(&before, &cpu, sizeof(cpu)) && meter.calls == 3 && meter.cycles == 4);
    trace->checked_cases |= bit;
    return 1;
}

static int refused_source(const FormatterCpu *cpu)
{
    MusashiCpuContext out, before;
    memset(&out, 0x73, sizeof(out));
    memcpy(&before, &out, sizeof(before));
    return !musashi_boot_cpu_context(cpu, MUSASHI_CPU_CONTEXT_SOURCE, &out) &&
           !memcmp(&before, &out, sizeof(out));
}

static int service_table(void *user, uint32_t *table)
{
    unsigned *calls = user;
    (void)table;
    ++*calls;
    return 0;
}

/* Find a qualifying caller from a fresh CPU running the mounted instructions.
 * Restrict the search to register-only work ending in an indirect call. */
static int table_service_context(Trace *trace, const MusashiCpuContext *live,
                                 MusashiBiosImageDevice *image)
{
    enum { BIOS_B = 0xb0, GET_C0_TABLE = 0x56 };
    uint32_t base = live->pc & ~(MUSASHI_RAM_SIZE - 1u);
    unsigned accepted = 0;
    for (uint32_t offset = 4u; offset + 8u <= MUSASHI_RAM_SIZE; offset += 4u) {
        uint32_t address = base + offset, word, slot;
        CHECK(musashi_boot_read32(trace->memory, address, &word));
        if ((word >> 26) != 0 || (word & 63u) != 9u) continue;
        CHECK(musashi_boot_read32(trace->memory, address + 4u, &slot));
        if (!plain_register_slot(slot)) continue;
        for (unsigned back = 1; back <= 8 && 4u * back <= offset; ++back) {
            uint32_t start = address - 4u * back;
            CHECK(musashi_boot_read32(trace->memory, start, &word));
            if (!plain_register_slot(word)) break;
            FormatterCpu frame = {0};
            frame.pc = start;
            frame.npc = start + 4u;
            unsigned steps = 0;
            while (steps < back + 2u && formatter_step(trace->memory, &frame)) ++steps;
            if (steps != back + 2u || frame.pc != BIOS_B || frame.r[9] != GET_C0_TABLE ||
                frame.r[10] != BIOS_B || frame.r[31] != address + 8u) continue;
            MusashiCpuContext host;
            frame.bios_image = image;
            if (musashi_boot_continuation_host_context(&frame, &host)) {
                ++accepted;
                CHECK(!frame.delay_slot && frame.npc == frame.pc + 4u);
                CHECK(host.identity == &frame && host.gpr == frame.r);
                CHECK(host.provenance == MUSASHI_CPU_CONTEXT_HOST_BIOS_SERVICE);
                CHECK(!host.pc && !host.npc && !host.instruction && !host.branch_pc);
                CHECK(!host.instruction_valid && !host.delay_slot);

                frame.r[31] += 4u;
                CHECK(!musashi_boot_continuation_host_context(&frame, &host));
                frame.r[31] -= 4u;
                ++frame.r[9];
                CHECK(!musashi_boot_continuation_host_context(&frame, &host));
                --frame.r[9];
                ++frame.r[10];
                CHECK(!musashi_boot_continuation_host_context(&frame, &host));
                --frame.r[10];
                frame.npc += 4u;
                CHECK(!musashi_boot_continuation_host_context(&frame, &host));
                frame.npc -= 4u;
                frame.delay_slot = 1;
                frame.branch_pc = frame.pc - 4u;
                CHECK(!musashi_boot_continuation_host_context(&frame, &host));
                frame.delay_slot = 0;
                frame.branch_pc = 0;
                frame.bios_image = NULL;
                CHECK(!musashi_boot_continuation_host_context(&frame, &host));
                MusashiBiosImageDevice empty_image = {0};
                frame.bios_image = &empty_image;
                CHECK(!musashi_boot_continuation_host_context(&frame, &host));
            }
            break;
        }
    }
    CHECK(accepted == 1);
    return 1;
}

static int context_boundaries(Trace *trace, const MusashiCpuContext *live)
{
    FormatterCpu cpu = {0};
    MusashiCpuContext context;
    cpu.pc = live->pc;
    cpu.npc = cpu.pc + 4u;
    CHECK(musashi_boot_cpu_context(&cpu, MUSASHI_CPU_CONTEXT_SOURCE, &context));
    cpu.r[19] = 571u;
    CHECK(context.identity == &cpu && context.gpr == cpu.r && context.gpr[19] == 571u);
    cpu.delay_slot = 1;
    cpu.branch_pc = cpu.pc + 28u;
    CHECK(refused_source(&cpu));
    cpu.delay_slot = 0;
    cpu.branch_pc = 0;
    ++cpu.pc;
    cpu.npc = cpu.pc + 4u;
    CHECK(refused_source(&cpu));
    cpu.pc = UINT32_C(0xd7345400);
    cpu.npc = cpu.pc + 4u;
    CHECK(refused_source(&cpu));
    CHECK(!musashi_boot_continuation_host_context(&cpu, &context));
    CHECK(musashi_boot_cpu_context(&cpu, MUSASHI_CPU_CONTEXT_HOST_BIOS_SERVICE, &context));
    CHECK(context.identity == &cpu && context.gpr == cpu.r);
    CHECK(context.provenance == MUSASHI_CPU_CONTEXT_HOST_BIOS_SERVICE);
    CHECK(!context.instruction_valid && !context.delay_slot && !context.branch_pc);
    CHECK(!context.pc && !context.npc && !context.instruction);

    unsigned calls = 0;
    MusashiBiosImageDevice image = {0};
    image.userdata = &calls;
    image.table = service_table;
    cpu.bios_image = &image;
    cpu.pc = FORMATTER_SEARCH;
    cpu.npc = cpu.pc + 4u;
    CHECK(musashi_boot_continuation_host_context(&cpu, &context));
    CHECK(context.identity == &cpu && context.gpr == cpu.r && !calls);
    CHECK(table_service_context(trace, live, &image));
    CHECK(calls == 0);
    return 1;
}

static int checkpoint(void *user, const void *continuation)
{
    Trace *trace = user;
    MusashiCpuContext source, again, host;
    uint32_t in_memory;
    ++trace->checkpoints;
    CHECK(trace->checkpoints <= 1000000);
    CHECK(musashi_boot_cpu_context(continuation, MUSASHI_CPU_CONTEXT_SOURCE, &source));
    CHECK(source.provenance == MUSASHI_CPU_CONTEXT_SOURCE && source.instruction_valid);
    CHECK(source.identity && source.gpr && source.gpr[0] == 0);
    CHECK(musashi_boot_read32(trace->memory, source.pc, &in_memory));
    CHECK(in_memory == source.instruction);
    CHECK(controlled_branch(trace, &source));
    CHECK(musashi_boot_cpu_context(continuation, MUSASHI_CPU_CONTEXT_SOURCE, &again));
    CHECK(again.gpr == source.gpr && again.identity == source.identity);
    CHECK(again.pc == source.pc && again.npc == source.npc);
    CHECK(again.instruction == source.instruction && again.delay_slot == source.delay_slot);
    CHECK(again.branch_pc == source.branch_pc);
    if (trace->checkpoints == 1) {
        CHECK(source.gpr[31] == trace->seed_ra);
        CHECK(context_boundaries(trace, &source));
        MusashiCpuContext invalid, saved;
        memset(&invalid, 0x97, sizeof(invalid));
        memcpy(&saved, &invalid, sizeof(saved));
        CHECK(!musashi_boot_cpu_context(continuation, (MusashiCpuContextProvenance)42, &invalid));
        CHECK(!memcmp(&invalid, &saved, sizeof(invalid)));
        uint32_t scratch = ~in_memory;
        CHECK(musashi_boot_write32(trace->memory, source.pc, scratch));
        int exported = musashi_boot_cpu_context(continuation, MUSASHI_CPU_CONTEXT_SOURCE, &again);
        CHECK(musashi_boot_write32(trace->memory, source.pc, in_memory));
        CHECK(exported && again.instruction == source.instruction && again.instruction != scratch);
    }
    if (musashi_boot_continuation_host_context(continuation, &host)) {
        CHECK(host.provenance == MUSASHI_CPU_CONTEXT_HOST_BIOS_SERVICE);
        CHECK(host.identity == source.identity && host.gpr == source.gpr);
        CHECK(!host.instruction_valid && !host.delay_slot && !host.branch_pc);
        CHECK(!host.pc && !host.npc && !host.instruction);
        ++trace->host_views;
    }
    if (trace->retiring_slot && source.identity == trace->previous_identity) {
        CHECK(!source.delay_slot && !source.branch_pc);
        CHECK(source.pc == trace->after_slot);
    }
    if (trace->previous != PLAIN) {
        CHECK(source.identity == trace->previous_identity);
        CHECK(source.pc == trace->previous_pc + 4u);
    }
    if (source.identity == trace->previous_identity && source.pc == trace->previous_pc + 4u) {
        CHECK((source.delay_slot != 0) == (trace->previous != PLAIN));
        if (trace->retiring_slot && source.identity == trace->previous_identity) {
        CHECK(!source.delay_slot && !source.branch_pc);
        CHECK(source.pc == trace->after_slot);
    }
    if (trace->previous != PLAIN) {
            CHECK(source.branch_pc == trace->previous_pc && source.npc == trace->expected_target);
            trace->kinds |= 1u << (unsigned)trace->previous;
            if (source.npc == source.pc + 4u) ++trace->sequential_slots;
            else ++trace->redirected_slots;
        }
    }
    if (!source.delay_slot) CHECK(!source.branch_pc);
    trace->retiring_slot = source.delay_slot;
    trace->after_slot = source.npc;
    trace->previous = next_branch(&source, &trace->expected_target);
    trace->previous_pc = source.pc;
    trace->previous_identity = source.identity;
    const unsigned all = (1u << (SIGN + 1)) - 2u;
    trace->met = trace->kinds == all && trace->host_views &&
                 trace->sequential_slots && trace->redirected_slots && trace->checked_cases == 127u;
    return !trace->met;
}

static void discard_console(void *user, const uint8_t *bytes, int32_t size)
{
    (void)user; (void)bytes; (void)size;
}

static int map_input(MusashiBootMemory *memory, const char *path)
{
    FILE *input = fopen(path, "rb");
    if (!input) return 0;
    long size = -1;
    if (fseek(input, 0, SEEK_END) == 0) size = ftell(input);
    uint8_t *data = size > 0 ? malloc((size_t)size) : NULL;
    int mapped = data && !fseek(input, 0, SEEK_SET) &&
                 fread(data, 1, (size_t)size, input) == (size_t)size &&
                 musashi_boot_map_exe(memory, data, (size_t)size);
    free(data);
    fclose(input);
    return mapped;
}

static int run(const char *path, uint32_t seed_ra)
{
    Trace trace = {0};
    trace.seed_ra = seed_ra;
    trace.memory = calloc(1, sizeof(*trace.memory));
    if (!trace.memory) return 0;
    int mapped = map_input(trace.memory, path);
    if (mapped) {
        MusashiCallbackDevice listener = {0};
        MusashiCdDevice cd = {0};
        MusashiStartupPrefixDevice devices = {0};
        MusashiEntryRunStop stop = {0};
        listener.userdata = &trace;
        listener.checkpoint = checkpoint;
        devices.callback = &listener;
        devices.cd = &cd;
        (void)musashi_boot_run_entry_with_devices(trace.memory, seed_ra, &devices,
            discard_console, NULL, NULL, NULL, &stop);
    }
    free(trace.memory);
    if (!trace.met || trace.error) fprintf(stderr, "controlled_mask=%u\n",trace.checked_cases);
    if (!trace.met || trace.error)
        fprintf(stderr, "checkpoints=%u branch_mask=%u host=%u sequential=%u redirected=%u\n",
                trace.checkpoints, trace.kinds, trace.host_views,
                trace.sequential_slots, trace.redirected_slots);
    return mapped && trace.met && !trace.error;
}

int main(int argc, char **argv)
{
    MusashiCpuContext context, sentinel;
    memset(&context, 0xa7, sizeof(context));
    memcpy(&sentinel, &context, sizeof(context));
    if (musashi_boot_cpu_context(NULL, MUSASHI_CPU_CONTEXT_SOURCE, &context) ||
        musashi_boot_continuation_host_context(NULL, &context) ||
        memcmp(&sentinel, &context, sizeof(context))) return 1;
    if (argc != 2 || !run(argv[1], 14248u) || !run(argv[1], 83632u)) return 1;
    puts("CPU_CONTEXT_PASS source_branches=7 host_physical_context=UNCLAIMED");
    return 0;
}
