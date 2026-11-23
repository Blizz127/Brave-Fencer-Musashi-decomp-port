#ifndef MUSASHI_SCENE_COUNTER_H
#define MUSASHI_SCENE_COUNTER_H

#include <stdint.h>

static inline void musashi_clear_scene_halfword(uint8_t *field)
{
    field[0] = 0;
    field[1] = 0;
}

/* fields points at D_800AF630 + 0xA3B4 in little-endian game RAM. */
static inline uint32_t musashi_advance_scene_fields(uint8_t *fields)
{
    uint32_t next = (uint32_t)(fields[0] | ((uint32_t)fields[1] << 8)) + 1u;
    fields[0] = (uint8_t)next;
    fields[1] = (uint8_t)(next >> 8);
    musashi_clear_scene_halfword(fields + 6);
    musashi_clear_scene_halfword(fields + 10);
    musashi_clear_scene_halfword(fields + 2);
    musashi_clear_scene_halfword(fields + 8);
    return next;
}

#endif
