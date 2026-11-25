/* SPDX-License-Identifier: MIT */
#ifndef BFM_PALACE_DIALOGUE_H
#define BFM_PALACE_DIALOGUE_H

#include "musashi_disc_media.h"
#include <stddef.h>
#include <stdint.h>

typedef struct PalaceDialogue PalaceDialogue;

/* Only the first palace speech is supported. Resource bytes are copied, so
 * the scene and its uploaded graphics can be released after this call. */
PalaceDialogue *bfm_palace_dialogue_load(const MusashiDiscMedia *disc,
                                         const uint8_t *scene, size_t scene_size,
                                         const uint16_t vram[512][1024]);
void bfm_palace_dialogue_free(PalaceDialogue *dialogue);
/* fields counts display fields from the opening moon, starting at zero.
 * Compose after drawing the room and actors at 320 x 240. */
void bfm_palace_dialogue_draw(const PalaceDialogue *dialogue, unsigned fields,
                              uint32_t pixels[240][320]);
#endif
