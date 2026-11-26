/* SPDX-License-Identifier: MIT */
#include "village_greeting.h"

BfmForestMessage *bfm_village_greeting_open(const uint8_t *scene, size_t bytes,
                                            BfmVillageSpeech speech)
{
    /* Each mounted actor command contains a pointer to its field-message
     * program. The size includes the closing command and its operand. */
    size_t operand, length;
    switch (speech) {
    case BFM_VILLAGE_ARRIVAL:
        operand = 0x61674;
        length = 87;
        break;
    case BFM_VILLAGE_HUNGER:
        operand = 0x6168c;
        length = 180;
        break;
    default:
        return NULL;
    }
    if (!scene || bytes < operand + 4) return NULL;
    const uint8_t *p = scene + operand;
    uint32_t address = (uint32_t)p[0] | (uint32_t)p[1] << 8 |
                       (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
    const uint32_t base = UINT32_C(0x80128158);
    if (address < base || length > bytes || address - base > bytes - length) return NULL;
    const uint8_t *program = scene + (address - base);
    if (program[0] != 0x05 || program[length - 2] != 0x03 || program[length - 1] != 0)
        return NULL;
    return bfm_field_message_borrow(program, length);
}
