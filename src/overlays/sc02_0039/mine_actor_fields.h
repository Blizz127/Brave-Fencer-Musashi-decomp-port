#ifndef BFM_MINE_ACTOR_FIELDS_H
#define BFM_MINE_ACTOR_FIELDS_H

#include <stdint.h>

static inline uint16_t mine_actor_load16(const uint8_t *field)
{
    return (uint16_t)(field[0] | (uint16_t)field[1] << 8);
}

static inline uint32_t mine_actor_load32(const uint8_t *field)
{
    return field[0] | (uint32_t)field[1] << 8 | (uint32_t)field[2] << 16 |
           (uint32_t)field[3] << 24;
}

static inline void mine_actor_store16(uint8_t *field, uint16_t value)
{
    field[0] = (uint8_t)value;
    field[1] = (uint8_t)(value >> 8);
}

static inline void mine_actor_store32(uint8_t *field, uint32_t value)
{
    field[0] = (uint8_t)value;
    field[1] = (uint8_t)(value >> 8);
    field[2] = (uint8_t)(value >> 16);
    field[3] = (uint8_t)(value >> 24);
}

static inline int mine_actor_signed16(uint16_t value)
{
    return (int)(value ^ 0x8000u) - 32768;
}
#endif
