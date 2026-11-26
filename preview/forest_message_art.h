/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_MESSAGE_ART_H
#define BFM_FOREST_MESSAGE_ART_H

#include "forest_message.h"
#include "forest_terrain.h"

typedef struct BfmForestMessageArt BfmForestMessageArt;

/* SC01/77 supplies the inset offsets and uploaded palettes. MAIN/10 owns the
 * font and contours. The returned object owns its decoded graphics. */
BfmForestMessageArt *bfm_forest_message_art_load(const MusashiDiscMedia *disc,
                                                const uint8_t *scene, size_t bytes,
                                                const BfmForestTerrain *terrain);
/* The room owns its contour inset and portrait already present in mounted VRAM. */
BfmForestMessageArt *bfm_bedroom_message_art_load(const MusashiDiscMedia *disc,
                                                  const uint8_t *scene, size_t bytes,
                                                  const uint16_t tiles[512][1024]);
void bfm_forest_message_art_free(BfmForestMessageArt *art);
void bfm_forest_message_art_draw(const BfmForestMessageArt *art,
                                 const BfmForestMessageFrame *frame,
                                 uint32_t pixels[240][320]);
void bfm_bedroom_message_art_draw(const BfmForestMessageArt *art, const BfmForestMessageFrame *frame,
                                  int speaker_x, uint32_t pixels[240][320]);
#endif
