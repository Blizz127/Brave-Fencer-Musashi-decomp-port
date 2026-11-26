/* SPDX-License-Identifier: MIT */
#include "village_clock.h"

static unsigned read16(const uint8_t *p)
{
    return (unsigned)p[0] | (unsigned)p[1] << 8;
}

static void write16(uint8_t *p, unsigned value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

void bfm_village_clock_step(BfmSaveRecord *state, int accelerated)
{
    if (!state || !(state->flags[127 / 8] & (1u << (127 % 8)))) return;

    uint8_t *p = state->progress;
    unsigned minute = read16(p + 0x1a);
    unsigned fraction = read16(p + 0x18) + (accelerated ? 43688u : 5461u);
    minute += fraction >> 16;

    if (minute >= 24 * 60) {
        unsigned day = read16(p + 0x34) + 1;
        write16(p + 0x34, day <= 365 ? day : 1);
        p[0x42] = (uint8_t)((p[0x42] + 1u) % 7u);
        minute = 0;
        fraction = 0;
    }

    write16(p + 0x18, fraction);
    write16(p + 0x1a, minute);
    p[0x36] = minute >= 7 * 60 && minute < 19 * 60;
    if (minute < 5 * 60 || minute >= 22 * 60) p[0x37] = 4;
    else if (minute < 7 * 60) p[0x37] = 0;
    else if (minute < 16 * 60) p[0x37] = 1;
    else if (minute < 19 * 60) p[0x37] = 2;
    else p[0x37] = 3;
    p[0x38] = (uint8_t)(minute % 60);
    p[0x39] = (uint8_t)(minute / 60);
}
