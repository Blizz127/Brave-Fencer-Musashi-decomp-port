/* SPDX-License-Identifier: MIT */
#ifndef BFM_BEDROOM_AWARD_H
#define BFM_BEDROOM_AWARD_H

#include "bedroom_field.h"
void bfm_bedroom_award_draw(const uint8_t geometry[32], const BfmBedroomView *view,
                            const int16_t position[3], unsigned age, uint32_t pixels[240][320]);
#endif
