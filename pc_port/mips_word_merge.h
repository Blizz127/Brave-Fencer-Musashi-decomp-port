/* SPDX-License-Identifier: MIT */
#ifndef BFM_MIPS_WORD_MERGE_H
#define BFM_MIPS_WORD_MERGE_H

#include <stdbool.h>
#include <stdint.h>

/* Little-endian MIPS I. The caller fetches the instruction and owns address
 * translation, bus access, device clocks, exceptions and register retirement. */
struct bfm_merge_pending_load {
    unsigned target;
    uint32_t value;
};

struct bfm_word_merge {
    uint32_t byte_address;
    uint32_t aligned_address;
    uint32_t operand;
    unsigned target;
    unsigned shift;
    bool left;
    bool store;
};

struct bfm_merge_store_word {
    uint32_t data;
    unsigned byte_enable; /* Bit i enables the byte at aligned_address + i. */
};

/* regs contains the architectural GPRs as seen on issue. A non-NULL pending
 * load is the older write still in the load-delay pipeline. Only a merge load
 * forwards it to its merge operand. Base registers and stores use issue GPRs.
 * For any other opcode, leave result untouched. */
static inline bool bfm_decode_word_merge(uint32_t instruction,
                                         const uint32_t regs[32],
                                         const struct bfm_merge_pending_load *pending,
                                         struct bfm_word_merge *result)
{
    const unsigned opcode = instruction >> 26;
    bool left;
    bool store;
    switch (opcode) {
    case 0x22: left = true;  store = false; break; /* LWL */
    case 0x26: left = false; store = false; break; /* LWR */
    case 0x2a: left = true;  store = true;  break; /* SWL */
    case 0x2e: left = false; store = true;  break; /* SWR */
    default: return false;
    }

    const unsigned base = (instruction >> 21) & 31u;
    const unsigned target = (instruction >> 16) & 31u;
    uint32_t offset = instruction & UINT32_C(0xffff);
    if ((offset & UINT32_C(0x8000)) != 0) {
        offset |= UINT32_C(0xffff0000);
    }
    const uint32_t address = (base ? regs[base] : 0u) + offset;
    const unsigned lane = address & 3u;
    uint32_t operand = target ? regs[target] : 0u;
    if (!store && target && pending && pending->target == target) {
        operand = pending->value;
    }

    result->byte_address = address;
    result->aligned_address = address & ~UINT32_C(3);
    result->operand = operand;
    result->target = target;
    result->shift = (left ? 3u - lane : lane) * 8u;
    result->left = left;
    result->store = store;
    return true;
}

/* Call with a decoded load after a successful aligned word read. Schedule the
 * result through the normal delayed-load path if target is nonzero. A zero
 * target still performs the checked bus access and can raise an exception. */
static inline uint32_t bfm_merge_loaded_word(const struct bfm_word_merge *access,
                                             uint32_t word)
{
    if (access->left) {
        const uint32_t replaced = UINT32_MAX << access->shift;
        return (access->operand & ~replaced) | (word << access->shift);
    }
    const uint32_t replaced = UINT32_MAX >> access->shift;
    return (access->operand & ~replaced) | (word >> access->shift);
}

/* A store drives just the selected byte lanes in one bus operation. */
static inline struct bfm_merge_store_word
bfm_merge_store_lanes(const struct bfm_word_merge *access)
{
    struct bfm_merge_store_word result;
    const unsigned bytes = access->shift / 8u;
    if (access->left) {
        result.data = access->operand >> access->shift;
        result.byte_enable = 15u >> bytes;
    } else {
        result.data = access->operand << access->shift;
        result.byte_enable = (15u << bytes) & 15u;
    }
    return result;
}

#endif
