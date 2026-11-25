/* Admission of stack and data transactions in the mounted room-selector family. */
#ifndef MUSASHI_MAIN12_ROOM_ACCESS_H
#define MUSASHI_MAIN12_ROOM_ACCESS_H

static int room_ram_value(MusashiBootMemory *ram, uint32_t address, uint32_t expected)
{
    uint32_t actual;
    return musashi_boot_read32(ram, address, &actual) && actual == expected;
}

/* Both game functions use ordinary o32 frames. Discover the point at which
 * each saved register acquires a backing slot from the mounted prologue.
 * Before that store the register itself still holds the incoming value. */
static int room_saved_register(MusashiBootMemory *ram, const FormatterCpu *cpu,
                               uint32_t entry, uint32_t frame, unsigned reg,
                               unsigned offset, uint32_t incoming)
{
    const unsigned stack = 29;
    for (unsigned i = 1; i <= 4; ++i) {
        uint32_t word;
        uint32_t address = entry + i * 4;
        if (!musashi_boot_read32(ram, address, &word)) return 0;
        if ((word >> 26) == 43 && ((word >> 21) & 31) == stack &&
            ((word >> 16) & 31) == reg && (uint16_t)word == offset) {
            return cpu->pc > address
                ? room_ram_value(ram, frame + offset, incoming)
                : cpu->r[reg] == incoming;
        }
    }
    return 0;
}

static int room_call_arguments(MusashiBootMemory *ram, const FormatterCpu *cpu,
                               uint32_t call, uint32_t slot, int inner)
{
    uint32_t target = ((call & 0x03ffffffu) << 2) | (cpu->pc & 0xf0000000u);
    uint32_t a0 = cpu->r[4], a1 = cpu->r[5];
    unsigned op = slot >> 26;
    unsigned dest = (slot >> 16) & 31;
    if (op == 9 && (dest == 4 || dest == 5)) {
        uint32_t value = cpu->r[(slot >> 21) & 31] + (int16_t)slot;
        if (dest == 4) a0 = value;
        else a1 = value;
    } else if (!op && (slot & 63) == 33) {
        dest = (slot >> 11) & 31;
        uint32_t value = cpu->r[(slot >> 21) & 31] + cpu->r[(slot >> 16) & 31];
        if (dest == 4) a0 = value;
        if (dest == 5) a1 = value;
    }
    if (target == 0x80029504u) return 1; /* Query current progress. */
    if (!inner) {
        if (target == MAIN12_ROOM_INIT) return 1;
        const uint32_t first_record = 0x801807bcu;
        const unsigned record_bytes = 36;
        uint32_t offset = a0 - first_record;
        return target == 0x800d185cu && offset < 8 * record_bytes &&
               offset % record_bytes == 0 && musashi_boot_ram_span(ram, a0, record_bytes);
    }
    switch (target) {
    case 0x800291dcu: return a0 == 8;             /* Test the reset flag. */
    case 0x800291c8u: return a0 == 8 && !a1;      /* Clear the reset flag. */
    case 0x80162120u: return 1;
    case 0x80029124u: return (a0 == 26 || a0 == 142 || a0 == 323) && !a1;
    case 0x80165a50u: return a0 == 0x80126b58u;
    case 0x80029514u: return a0 == 1200;
    default: return 0;
    }
}

static int room_is_control(uint32_t word)
{
    unsigned op = word >> 26;
    return op == 2 || op == 3 || op == 4 || op == 5 || (!op && (word & 63) == 8);
}

/* Validate the destination latched by the branch owner, including its callee
 * argument at the delay boundary. No transfer or delay operation is issued. */
static int room_control(MusashiBootMemory *ram, const FormatterCpu *cpu,
                        uint32_t pc, uint32_t word, uint32_t slot,
                        uint32_t entry, uint32_t end, uint32_t caller,
                        int in_delay)
{
    unsigned op = word >> 26;
    uint32_t target;
    if (op == 3 || op == 2) {
        target = ((pc + 4) & 0xf0000000u) | ((word & 0x03ffffffu) << 2);
        if (op == 3) {
            if (!room_call_arguments(ram, cpu, word, slot, entry == MAIN12_ROOM_INIT)) return 0;
            if (in_delay && cpu->r[31] != pc + 8) return 0;
        } else if (target < entry || target >= end) return 0;
    } else if (op == 4 || op == 5) {
        int equal = cpu->r[(word >> 21) & 31] == cpu->r[(word >> 16) & 31];
        target = (equal == (op == 4)) ? pc + 4 + (int16_t)word * 4 : pc + 8;
        if (target < entry || target >= end) return 0;
    } else if (!op && (word & 63) == 8 && ((word >> 21) & 31) == 31) {
        target = cpu->r[31];
        if (target != caller) return 0;
    } else {
        return 0;
    }
    return !in_delay || cpu->npc == target;
}
#endif
