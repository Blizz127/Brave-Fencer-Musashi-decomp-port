/* SPDX-License-Identifier: MIT */
#ifndef BFM_RITUAL_LIGHT_H
#define BFM_RITUAL_LIGHT_H
#include <stddef.h>
#include <stdint.h>
/* x and y are projected onto the 320 x 240 view. z is camera depth.
 * The bucket interleaves the lights with the chamber, crystal and cast. */
typedef void (*BfmRitualEmit)(void *context, unsigned vertices,
                              const int xyz[4][3], const unsigned rgb[4][3],
                              int bucket);
typedef struct {
    int16_t point[12][3];
    unsigned tint[3];
} BfmRitualNodes;
int bfm_ritual_light_load(BfmRitualNodes *nodes, const uint8_t *scene, size_t size);
unsigned bfm_ritual_extra_yaw(unsigned field);
void bfm_ritual_rings(unsigned field, const int camera[3][3], const int translation[3],
                      const unsigned tint[3], BfmRitualEmit emit, void *context);
void bfm_ritual_light_prepare(const BfmRitualNodes *nodes, unsigned field, const int view[3][3],
                              const int origin[3], BfmRitualEmit emit,
                              void *context);
#endif
