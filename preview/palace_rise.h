/* SPDX-License-Identifier: MIT */
#ifndef BFM_PALACE_RISE_H
#define BFM_PALACE_RISE_H

#include <stddef.h>
#include <stdint.h>
#include "palace_sound_bank.h"

enum { BFM_PALACE_RISE_FIELD = 4270, BFM_PALACE_RISE_RATE = 37800 };

/* Render SC01 member 1, sample 5. The caller owns the returned mono buffer
 * and plays it at the field above, equally on left and right. */
int bfm_palace_rise_render(const BfmSpuSample *instrument, int16_t **mono, size_t *frames);

#endif
