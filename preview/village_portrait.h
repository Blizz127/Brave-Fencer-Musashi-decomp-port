/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_PORTRAIT_H
#define BFM_VILLAGE_PORTRAIT_H

#include <stdint.h>

/* Stage the daylight restaurant resident's portrait in dialogue slot two
 * after the village tile upload and before the message art is decoded. */
void bfm_village_macho_portrait_load(uint16_t tiles[512][1024]);

void bfm_village_mayor_portrait_load(uint16_t tiles[512][1024]);
#endif
