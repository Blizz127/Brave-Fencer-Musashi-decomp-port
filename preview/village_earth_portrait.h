/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_EARTH_PORTRAIT_H
#define BFM_VILLAGE_EARTH_PORTRAIT_H

#include "forest_message_art.h"

typedef BfmForestMessageArt BfmVillageEarthPortrait;

/* Supply the upper village scene and its mounted texture pages. The returned
 * art owns the decoded portraits and does not keep a reference to the pages. */
BfmVillageEarthPortrait *bfm_village_earth_portrait_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, uint16_t tiles[512][1024]);
void bfm_village_earth_portrait_free(BfmVillageEarthPortrait *portraits);

/* Scene commands select a village resident (24 for Jon, 2 for Leno).
 * Each message also chooses a portrait slot, so Musashi can speak while
 * either resident is selected. Offset and side belong to the active speaker. */
int bfm_village_earth_portrait_draw(const BfmVillageEarthPortrait *portraits,
    int current_selector, const BfmForestMessageFrame *frame, int16_t offset,
    int right, uint32_t pixels[240][320]);

#endif
