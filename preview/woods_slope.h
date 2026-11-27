/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_SLOPE_H
#define BFM_WOODS_SLOPE_H

#include <stdint.h>

enum { BFM_WOODS_SLOPE_BODY = 17 };

typedef struct {
    int32_t downhill_q16[3];
    unsigned remaining;
    int settling;
} BfmWoodsSlope;

typedef enum {
    BFM_WOODS_SLOPE_CONTINUE, BFM_WOODS_SLOPE_IDLE, BFM_WOODS_SLOPE_FALL
} BfmWoodsSlopeResult;

/* Enter from a steep scene contact. The action inherits the player's countdown
 * and heading. Select body 17 while leaving the independent face track alone. */
void bfm_woods_slope_begin(BfmWoodsSlope *s, const int16_t normal_q12[3], unsigned countdown);

/* Call once before collision. Use the ordinary camera-relative heading when
 * steering, or the previous heading on neutral. Both vectors are signed Q16.
 * The local vector remains available to the ordinary fall action on exit.
 * During settling, newly accepted ordinary actions precede this call. */
void bfm_woods_slope_move(BfmWoodsSlope *s, int steering, unsigned yaw,
                          int32_t delta_q16[3], int32_t local_q16[3]);

/* Call after applying delta, resolving moving actors and querying the scene.
 * Terrain is the scene's packed result (zero on miss), independent of actor
 * support. normal_q12 is required when terrain describes a steep face. */
BfmWoodsSlopeResult bfm_woods_slope_contact(BfmWoodsSlope *s, unsigned terrain,
                                           int actor_support, const int16_t normal_q12[3]);

#endif
