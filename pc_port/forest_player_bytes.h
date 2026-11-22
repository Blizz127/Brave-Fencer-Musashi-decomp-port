#ifndef MUSASHI_FOREST_PLAYER_BYTES_H
#define MUSASHI_FOREST_PLAYER_BYTES_H

#include <stdint.h>

static inline uint16_t u16le(const uint8_t *p) {
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static inline uint32_t u32le(const uint8_t *p) {
    return (uint32_t)u16le(p) | ((uint32_t)u16le(p + 2) << 16);
}

static inline int16_t s16le(const uint8_t *p) {
    uint16_t n = u16le(p);
    return (int16_t)((int32_t)(n ^ 0x8000u) - 0x8000);
}

#endif
