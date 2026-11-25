/* SPDX-License-Identifier: MIT */
#ifndef BFM_MIPS_SCALAR_MEMORY_H
#define BFM_MIPS_SCALAR_MEMORY_H

#include <stdbool.h>
#include <stdint.h>

struct bfm_scalar_memory {
    uint32_t address;
    uint32_t store_value;
    unsigned target;
    unsigned width;
    bool store;
    bool signed_load;
};

/* Decode a natural-width MIPS I data access using the GPRs visible at issue.
 * Memory reads return a right-justified value at the specified byte address.
 * The caller owns bus authorization, exceptions and delayed register writes. */
static inline bool bfm_decode_scalar_memory(uint32_t instruction,
                                            const uint32_t regs[32],
                                            struct bfm_scalar_memory *access)
{
    unsigned width;
    bool store = false;
    bool signed_load = false;
    switch (instruction >> 26) {
    case 0x20: width = 1; signed_load = true; break; /* LB */
    case 0x21: width = 2; signed_load = true; break; /* LH */
    case 0x23: width = 4; break; /* LW */
    case 0x24: width = 1; break; /* LBU */
    case 0x25: width = 2; break; /* LHU */
    case 0x28: width = 1; store = true; break; /* SB */
    case 0x29: width = 2; store = true; break; /* SH */
    case 0x2b: width = 4; store = true; break; /* SW */
    default: return false;
    }
    const unsigned base = (instruction >> 21) & 31u;
    const unsigned target = (instruction >> 16) & 31u;
    const uint32_t displacement = ((instruction & UINT32_C(0xffff)) ^
                                   UINT32_C(0x8000)) - UINT32_C(0x8000);
    access->address = (base ? regs[base] : 0u) + displacement;
    access->target = target;
    access->width = width;
    access->store = store;
    access->signed_load = signed_load;
    access->store_value = target ? regs[target] : 0u;
    return true;
}

static inline uint32_t bfm_scalar_load_value(const struct bfm_scalar_memory *access,
                                             uint32_t value)
{
    if (access->width == 4u)
        return value;
    const unsigned bits = 8u * access->width;
    value &= (UINT32_C(1) << bits) - 1u;
    if (access->signed_load) {
        const uint32_t sign = UINT32_C(1) << (bits - 1u);
        value = (value ^ sign) - sign;
    }
    return value;
}

#endif
