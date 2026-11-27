/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_JON_PORTRAIT_H
#define BFM_VILLAGE_JON_PORTRAIT_H

#include "forest_message_art.h"

typedef struct {
    BfmForestMessageArt *art;
    /* Index zero is the man in the stocks. Index one is Musashi. */
    int anchor[2], right[2];
} BfmVillageJonPortrait;

/* Supply lower Grillin's scene, the Jon event payload and the current room
 * tiles. The caller owns the returned art and frees it with the field art API. */
int bfm_village_jon_portrait_load(BfmVillageJonPortrait *out, const MusashiDiscMedia *disc,
                                 const uint8_t *scene, size_t scene_bytes,
                                 const uint8_t *module, size_t module_bytes,
                                 uint16_t tiles[512][1024]);

#endif
