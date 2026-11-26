/* SPDX-License-Identifier: MIT */
#ifndef BFM_BEDROOM_EFFECTS_H
#define BFM_BEDROOM_EFFECTS_H
#include "musashi_disc_media.h"

typedef struct BfmBedroomEffects BfmBedroomEffects;
enum { BFM_ROOM_AWARD_SOUND = 0x6a6, BFM_ROOM_MOVE_SOUND = 0x62f, BFM_ROOM_ACCEPT_SOUND = 0x630 };
enum { BFM_FOREST_SWING_SOUND = 0x400, BFM_FOREST_HIT_SOUND = 0x437,
       BFM_FOREST_GUNSHOT_SOUND = 0x476 };
enum { BFM_VILLAGE_KNOCK_SOUND = 0x516 };
BfmBedroomEffects *bfm_village_knock_sound_load(const MusashiDiscMedia *disc);
BfmBedroomEffects *bfm_forest_swing_sound_load(const MusashiDiscMedia *disc);
BfmBedroomEffects *bfm_menu_effects_load(const MusashiDiscMedia *disc);
BfmBedroomEffects *bfm_bedroom_effects_load(const MusashiDiscMedia *disc, unsigned member, uint32_t room);
void bfm_bedroom_effects_free(BfmBedroomEffects *effects);
/* Serialized with the sound callback. All source decoding occurs during load. */
void bfm_bedroom_effects_start(BfmBedroomEffects *effects, unsigned command);
void bfm_bedroom_effects_mix(BfmBedroomEffects *effects, int16_t *stereo, size_t frames);
#endif
