/* Generated LZ input and synthetic clock. No disc or guest executable is used. */
#include "stream_call.h"
#include <assert.h>
#include <string.h>

#define INPUT   0x800e0000u
#define OUTPUT  0x80100000u
#define PHASE   0x800c7d24u
#define CURSOR  0x800747b0u
#define MASK    0x800747a0u
#define FLAGS   0x800747a4u
#define DEST    0x800747acu
#define TOKEN   0x800747b4u

enum { groups = 227, literals = groups * 8 + 3, copies = 11 };
static MusashiBootMemory memory, before;
static MusashiStreamBus bus;
static unsigned owner_ready, charges, cycles;

static uint32_t get(uint32_t address, unsigned width) {
    uint32_t value;
    assert(bus.read(&memory, address, width, &value));
    return value;
}

static int current(void *owner, uint32_t expected_phase) {
    assert(owner == &memory);
    return owner_ready && get(PHASE, 4) == expected_phase;
}

static int tick(void *owner, uint32_t cost) {
    assert(owner == &memory && (cost == 2 || cost == 3));
    ++charges;
    cycles += cost;
    return 1;
}

int main(void) {
    /* These paths either complete natively or stop before the first issue.
     * No exceptional instruction fetch is needed. */
    static const uint8_t unused_image[MUSASHI_STREAM_CODE_BYTES];
    const uint32_t targets[MUSASHI_STREAM_STATE_COUNT] = {4, 8, 12, 16, 20};
    const MusashiExecutionClock clock = {&memory, tick};
    MusashiCdIrqFrame frame = {.busy = 1};
    unsigned budget = 100000;
    MusashiStreamCall call = {unused_image, &clock, &frame, &budget, &memory, current, NULL, 1};
    MusashiStreamCpu cpu = {.pc = MUSASHI_STREAM_ENTRY, .npc = MUSASHI_STREAM_ENTRY + 4};
    MusashiStreamWork work = {.issues = 73};
    uint8_t expected[literals + copies], dictionary[MUSASHI_SCRATCHPAD_SIZE];
    bus = musashi_stream_memory_bus(&memory, targets);
    memset(&memory, 0xa5, sizeof(memory));
    for (unsigned i = 0; i < MUSASHI_STREAM_STATE_COUNT; ++i) {
        assert(bus.write(&memory, 0x80072a38u + 4u * i, 4, targets[i]));
    }
    assert(bus.write(&memory, PHASE, 4, 1));
    assert(bus.write(&memory, DEST, 4, OUTPUT));
    assert(bus.write(&memory, TOKEN, 2, 0xad80u));

    /* 227 flag/literal groups use 2043 bytes. Three more literals leave
     * precisely one byte for a match whose next sector supplies the high byte. */
    uint8_t *first = musashi_boot_ram_span(&memory, INPUT, 2048);
    uint8_t *next = musashi_boot_ram_span(&memory, INPUT + 2048, 2048);
    assert(first && next);
    for (unsigned i = 0; i < groups; ++i) {
        first[9 * i] = 0xff;
        for (unsigned j = 0; j < 8; ++j) {
            unsigned index = i * 8 + j;
            expected[index] = (uint8_t)(index * 13u + 29u);
            first[9 * i + 1 + j] = expected[index];
        }
    }
    first[2043] = 7;
    memcpy(first + 2044, "Jz4", 3);
    memcpy(expected + groups * 8, "Jz4Jz4Jz4Jz4Jz", 3 + copies);
    unsigned from = (1u + groups * 8u) % MUSASHI_SCRATCHPAD_SIZE;
    unsigned match = ((copies - 2u) << 10) | from;
    first[2047] = (uint8_t)match;
    next[0] = (uint8_t)(match >> 8);
    next[1] = 0;
    next[2] = 0x34; /* Nonzero upper six bits, zero dictionary index. */
    next[3] = next[4] = 0; /* The first marker must stop before these bytes. */

    cpu.r[4] = INPUT;
    cpu.r[31] = 0x80012000u;
    const MusashiStreamCpu entry = cpu;
    before = memory;
    assert(musashi_stream_execute_work(&call, &bus, &cpu, &work) == MUSASHI_STREAM_DEFER);
    assert(!charges && !frame.instructions && budget == 100000 && work.issues == 73);
    assert(!memcmp(&cpu, &entry, sizeof(cpu)) && !memcmp(&memory, &before, sizeof(memory)));
    owner_ready = 1;
    budget = 0;
    assert(musashi_stream_execute_work(&call, &bus, &cpu, &work) == MUSASHI_STREAM_FAULT);
    assert(frame.budget_exhausted == 2 && !frame.instructions && !charges && !budget && work.issues == 73);
    assert(!memcmp(&cpu, &entry, sizeof(cpu)) && !memcmp(&memory, &before, sizeof(memory)));

    frame.budget_exhausted = 0;
    budget = 100000;
    assert(musashi_stream_execute_work(&call, &bus, &cpu, &work) == MUSASHI_STREAM_RETURNED);
    assert(cpu.pc == entry.r[31] && cpu.npc == cpu.pc + 4 && cpu.r[2] == 1 && cpu.r[3] == 3);
    assert(work.literals == literals && !work.copied && work.issues == charges && work.cycles == cycles);
    assert(frame.instructions == charges && budget == 100000 - charges);
    assert(get(PHASE, 4) == 3 && get(CURSOR, 4) == (1u + literals) % MUSASHI_SCRATCHPAD_SIZE);
    assert(get(DEST, 4) == OUTPUT + literals && get(MASK, 1) == 8 && get(FLAGS, 1) == 7);
    assert(get(TOKEN, 2) == (0xad00u | (uint8_t)match));
    uint8_t *output = musashi_boot_ram_span(&memory, OUTPUT, sizeof(expected) + 1);
    assert(output && !memcmp(output, expected, literals) && output[literals] == 0xa5);

    cpu = entry;
    cpu.r[4] = INPUT + 2048;
    call.phase = 3;
    charges = cycles = 0;
    assert(musashi_stream_execute_work(&call, &bus, &cpu, &work) == MUSASHI_STREAM_RETURNED);
    assert(!cpu.r[2] && cpu.r[3] == 0x3400u && cpu.r[7] == INPUT + 2048 + 3 && cpu.r[8] == 2045);
    assert(!work.literals && work.copied == copies && work.issues == charges && work.cycles == cycles);
    assert(get(PHASE, 4) == 0 && get(TOKEN, 2) == (0xad00u | (uint8_t)match));
    assert(get(CURSOR, 4) == (1u + literals) % MUSASHI_SCRATCHPAD_SIZE);
    assert(get(DEST, 4) == OUTPUT + literals && get(MASK, 1) == 8 && get(FLAGS, 1) == 7);
    assert(!memcmp(output, expected, sizeof(expected)) && output[sizeof(expected)] == 0xa5);
    memset(dictionary, 0xa5, sizeof(dictionary));
    for (unsigned i = 0; i < sizeof(expected); ++i)
        dictionary[(1u + i) % sizeof(dictionary)] = expected[i];
    assert(!memcmp(memory.scratchpad, dictionary, sizeof(dictionary)));
    return 0;
}
