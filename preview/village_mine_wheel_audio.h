/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MINE_WHEEL_AUDIO_H
#define BFM_VILLAGE_MINE_WHEEL_AUDIO_H

#include <stdint.h>

enum { BFM_MINE_WHEELS = 3, BFM_MINE_WHEEL_LOOP = 0x6dd };
/* Carry the wheel number, gain and key-off through the ordinary room queue. */
#define BFM_MINE_WHEEL_EVENT UINT32_C(0x80000000)
enum { BFM_MINE_WHEEL_KEY_OFF = 1u << 18 };
static inline unsigned bfm_mine_wheel_sound_event(unsigned command, unsigned value, unsigned channel)
{
    if (channel >= BFM_MINE_WHEELS) return 0;
    unsigned source = BFM_MINE_WHEEL_EVENT | channel << 16 | BFM_MINE_WHEEL_LOOP;
    if (command == 4 && value == BFM_MINE_WHEEL_LOOP) return source | BFM_MINE_WHEEL_KEY_OFF;
    if (command == BFM_MINE_WHEEL_LOOP && (value & ~127u) == 0x1000)
        return source | ((value & 127u) << 19);
    return 0;
}
#endif
