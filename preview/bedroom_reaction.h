/* SPDX-License-Identifier: MIT */
#ifndef BFM_BEDROOM_REACTION_H
#define BFM_BEDROOM_REACTION_H
#include "bedroom_field.h"
typedef struct {
    unsigned state, remaining;
    int scale, lift;
    const uint8_t *sprite;
} BfmBedroomReaction;
/* A field may supply its own bounded twelve-byte sprite record. */
int bfm_reaction_start_sprite(BfmBedroomReaction *, const uint8_t *sprite, size_t bytes);
/* The player action completes immediately. The separate effect continues to
 * track the player; its mounted sprite must outlive the effect. */
int bfm_bedroom_reaction_start(BfmBedroomReaction *, const uint8_t *scene, size_t bytes);
void bfm_bedroom_reaction_step(BfmBedroomReaction *);
int bfm_bedroom_reaction_depth(const BfmBedroomReaction *, const BfmBedroomView *,
                               const int16_t player[3]);
void bfm_bedroom_reaction_draw(const BfmBedroomReaction *, const BfmBedroomView *,
                               const int16_t player[3], const uint16_t tiles[512][1024],
                               uint32_t *pixels);
#endif
