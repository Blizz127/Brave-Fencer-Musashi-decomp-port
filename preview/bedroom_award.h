/* SPDX-License-Identifier: MIT */
#ifndef BFM_BEDROOM_AWARD_H
#define BFM_BEDROOM_AWARD_H

#include "bedroom_field.h"
void bfm_bedroom_award_draw(const uint8_t geometry[32], const BfmBedroomView *view,
                            const int16_t position[3], unsigned age, uint32_t pixels[240][320]);
/* Draw one completed pickup field. Age zero is the renderer allocation field.
 * Bits 0..2 identify the allocated rays in emission order; bit 3 is the ring.
 * Position is the player's root before the effect's vertical offset. */
void bfm_bedroom_award_draw_slots(const uint8_t geometry[32], const BfmBedroomView *view,
                                  const int16_t position[3], unsigned age, unsigned visible_slots,
                                  uint32_t pixels[240][320]);
#endif
