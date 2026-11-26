/* SPDX-License-Identifier: MIT */
#ifndef BFM_BEDROOM_EFFECTS_H
#define BFM_BEDROOM_EFFECTS_H
#include "musashi_disc_media.h"

typedef struct BfmBedroomEffects BfmBedroomEffects;
enum { BFM_ROOM_AWARD_SOUND = 0x6a6, BFM_ROOM_MOVE_SOUND = 0x62f, BFM_ROOM_ACCEPT_SOUND = 0x630 };
BfmBedroomEffects *bfm_bedroom_effects_load(const MusashiDiscMedia *disc, unsigned member, uint32_t room);
void bfm_bedroom_effects_free(BfmBedroomEffects *effects);
/* Serialized with the sound callback. All source decoding occurs during load. */
void bfm_bedroom_effects_start(BfmBedroomEffects *effects, unsigned command);
void bfm_bedroom_effects_mix(BfmBedroomEffects *effects, int16_t *stereo, size_t frames);
#endif
