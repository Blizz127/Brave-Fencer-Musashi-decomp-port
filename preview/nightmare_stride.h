/* SPDX-License-Identifier: MIT */
#ifndef BFM_NIGHTMARE_STRIDE_H
#define BFM_NIGHTMARE_STRIDE_H

#include "bedroom_travel.h"

typedef struct BfmNightmareStride BfmNightmareStride;

/* The caller keeps the recovery room's expanded scene mounted. */
BfmNightmareStride *bfm_nightmare_stride_open(const uint8_t *scene, size_t bytes,
                                               uint32_t room_address);
void bfm_nightmare_stride_free(BfmNightmareStride *stride);

/* Begin one update after selecting the upright jump pose. The hop changes
 * only height. The caller draws its resident body program until landing. */
int bfm_nightmare_stride_hop(BfmNightmareStride *stride, BfmBedroomRoot *root);

/* Call on the first waypoint update and every following update until
 * arrival. The mounted waypoint supplies world XYZ. Both functions return
 * 1 on arrival, 0 while moving and -1 on a malformed or unsupported field. */
int bfm_nightmare_stride_walk(BfmNightmareStride *stride, BfmBedroomRoot *root,
                              const int16_t waypoint[3],
                              const int32_t eye[3], const int32_t target[3]);

#endif
