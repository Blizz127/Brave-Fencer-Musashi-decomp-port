/* SPDX-License-Identifier: MIT */
#ifndef BFM_PALACE_DIALOGUE_H
#define BFM_PALACE_DIALOGUE_H

#include "musashi_disc_media.h"
#include <stddef.h>
#include <stdint.h>

typedef struct PalaceDialogue PalaceDialogue;

/* Load the staircase and crystal chamber messages from the mounted scene.
 * Their script and drawing resources can be released after this call. */
PalaceDialogue *bfm_palace_dialogue_load(const MusashiDiscMedia *disc,
                                         const uint8_t *scene, size_t scene_size,
                                         const uint16_t vram[512][1024]);
void bfm_palace_dialogue_free(PalaceDialogue *dialogue);
/* Query the script's due bank and cue using the same opening-field clock.
 * The request remains visible until it starts or the room moves on. */
int bfm_palace_dialogue_voice_due(const PalaceDialogue *dialogue, unsigned opening_field,
                                 unsigned *bank, unsigned *cue);
int bfm_palace_dialogue_voice_schedule(const PalaceDialogue *dialogue, unsigned opening_field,
                                      unsigned *bank, unsigned *cue, unsigned *issue_field);
/* Called on the game thread when the mounted voice stream becomes ready.
 * Bank and cue identify the script's request. opening_field uses the same
 * clock as draw. A duplicate notification preserves the first start. */
int bfm_palace_dialogue_voice_started(PalaceDialogue *dialogue, unsigned bank,
                                     unsigned cue, unsigned opening_field);
/* Return the final mounted court message's close field once its pages are ready. */
int bfm_palace_dialogue_departure_field(const PalaceDialogue *dialogue, unsigned *field);
/* fields counts display fields from the opening moon, starting at zero.
 * Compose after drawing the room and actors at 320 x 240. */
void bfm_palace_dialogue_draw(const PalaceDialogue *dialogue, unsigned fields,
                              uint32_t pixels[240][320]);
#endif
