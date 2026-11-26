/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_SKY_H
#define BFM_VILLAGE_SKY_H

#include <stddef.h>
#include <stdint.h>

/* The descriptor contains the heights, offsets and RGB endpoints of two
 * screen-aligned sky bands. Draw the background before submitting the mesh. */
int bfm_village_sky_draw(const uint8_t *room, size_t bytes,
                        uint32_t pixels[240][320]);

/* Draw the lower village sky at the settled light level for a game-clock hour. */
int bfm_lower_village_sky_draw(const uint8_t *scene, size_t bytes, unsigned hour,
                               uint32_t pixels[240][320]);

#endif
