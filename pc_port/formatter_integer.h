/* Side-effect-free integer/branch portion shared by the ordinary formatter
 * and its isolated exceptional CPU witness. All guest state changes go only
 * to the supplied GPRs and next-PC scalar. Unsupported operations are left
 * untouched for the ordinary handler. */
#ifndef MUSASHI_FORMATTER_INTEGER_H
#define MUSASHI_FORMATTER_INTEGER_H
#include <stdint.h>

static uint32_t formatter_sra(uint32_t value, uint32_t shift) {
    uint32_t result = value >> shift;
    if (shift && (value & 0x80000000u))
        result |= UINT32_MAX << (32u - shift);
    return result;
}

static inline int formatter_integer_effect(uint32_t instruction, uint32_t pc,
    uint32_t r[32], uint32_t *next) {
    unsigned rs = (instruction >> 21) & 31u, rt = (instruction >> 16) & 31u;
    unsigned rd = (instruction >> 11) & 31u;
    uint32_t immediate = instruction & 0xffffu;
    int16_t signed_immediate = (int16_t)immediate;
    switch (instruction >> 26) {
    case 0:
        switch (instruction & 63u) {
        case 0: r[rd] = r[rt] << ((instruction >> 6) & 31u); break;
        case 2: r[rd] = r[rt] >> ((instruction >> 6) & 31u); break;
        case 3: r[rd] = formatter_sra(r[rt], (instruction >> 6) & 31u); break;
        case 4: r[rd] = r[rt] << (r[rs] & 31u); break;
        case 6: r[rd] = r[rt] >> (r[rs] & 31u); break;
        case 7: {
            uint32_t amount = r[rs] & 31u;
            r[rd] = formatter_sra(r[rt], amount);
            break;
        }
        case 8: *next = r[rs]; break;
        case 33: r[rd] = r[rs] + r[rt]; break;
        case 35: r[rd] = r[rs] - r[rt]; break;
        case 36: r[rd] = r[rs] & r[rt]; break;
        case 37: r[rd] = r[rs] | r[rt]; break;
        case 38: r[rd] = r[rs] ^ r[rt]; break;
        case 39: r[rd] = ~(r[rs] | r[rt]); break;
        case 42: r[rd] = (int32_t)r[rs] < (int32_t)r[rt]; break;
        case 43: r[rd] = r[rs] < r[rt]; break;
        default: return 0;
        }
        break;
    case 1:
        if (rt > 1u) return 0;
        if ((rt == 0u && (int32_t)r[rs] < 0) || (rt == 1u && (int32_t)r[rs] >= 0))
            *next = pc + 4u + (uint32_t)((int32_t)signed_immediate * 4);
        break;
    case 2: *next = ((pc + 4u) & 0xf0000000u) | ((instruction & 0x03ffffffu) << 2); break;
    case 4: if (r[rs] == r[rt]) *next = pc + 4u + (uint32_t)((int32_t)signed_immediate * 4); break;
    case 5: if (r[rs] != r[rt]) *next = pc + 4u + (uint32_t)((int32_t)signed_immediate * 4); break;
    case 6: if ((int32_t)r[rs] <= 0) *next = pc + 4u + (uint32_t)((int32_t)signed_immediate * 4); break;
    case 7: if ((int32_t)r[rs] > 0) *next = pc + 4u + (uint32_t)((int32_t)signed_immediate * 4); break;
    case 9: r[rt] = r[rs] + (int32_t)signed_immediate; break;
    case 10: r[rt] = (int32_t)r[rs] < (int32_t)signed_immediate; break;
    case 11: r[rt] = r[rs] < immediate; break;
    case 12: r[rt] = r[rs] & immediate; break;
    case 13: r[rt] = r[rs] | immediate; break;
    case 14: r[rt] = r[rs] ^ immediate; break;
    case 15: r[rt] = immediate << 16; break;
    default: return 0;
    }
    return 1;
}
#endif
