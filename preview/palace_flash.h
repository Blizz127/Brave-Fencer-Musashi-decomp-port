/* SPDX-License-Identifier: MIT */
#ifndef BFM_PALACE_FLASH_H
#define BFM_PALACE_FLASH_H

#include "spu_sample.h"

/* The two centered voices of the first ritual flash. The caller supplies
 * decoded palace bank sample 3 and mixes the same result into both channels
 * at 37,800 Hz. The length includes the sample's natural decay. */
size_t bfm_palace_flash_frames(const BfmDecodedSpuSample *sample);
int16_t bfm_palace_flash_frame(const BfmDecodedSpuSample *sample, size_t frame);

/* The intervening sound on bank sample 6 shares the faster flash pitch.
 * Start it separately from sample 3 and mix the centered result at 37,800 Hz. */
size_t bfm_palace_after_flash_frames(const BfmDecodedSpuSample *sample);
int16_t bfm_palace_after_flash_frame(const BfmDecodedSpuSample *sample, size_t frame);

#endif
