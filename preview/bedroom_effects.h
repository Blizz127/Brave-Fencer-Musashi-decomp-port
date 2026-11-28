/* SPDX-License-Identifier: MIT */
#ifndef BFM_BEDROOM_EFFECTS_H
#define BFM_BEDROOM_EFFECTS_H
#include "musashi_disc_media.h"

typedef struct BfmBedroomEffects BfmBedroomEffects;
enum { BFM_VILLAGE_MINE_STRIKE_SOUND = 0x7e0, BFM_VILLAGE_MINE_UNLOCK_SOUND = 0x7a0 };
enum { BFM_MINE_POLE_SOUND = 0x6a7 };
BfmBedroomEffects *bfm_mine_pole_sound_load(const MusashiDiscMedia *disc);
enum { BFM_ROOM_AWARD_SOUND = 0x6a6, BFM_ROOM_MOVE_SOUND = 0x62f, BFM_ROOM_ACCEPT_SOUND = 0x630 };
enum { BFM_FOREST_SWING_SOUND = 0x400, BFM_FOREST_HIT_SOUND = 0x437,
       BFM_FOREST_GUNSHOT_SOUND = 0x476 };
enum { BFM_VILLAGE_KNOCK_SOUND = 0x516, BFM_VILLAGE_WELL_SOUND = 0x843,
       BFM_WOODS_SHRINK_CAST_SOUND = 0x479, BFM_WOODS_HOP_SOUND = 0x47a,
       BFM_WOODS_SHRINK_SOUND = 0x989, BFM_BODY_LEVEL_SOUND = 0xb2c,
       BFM_WOODS_KING_STOMP_SOUND = 0x6a8, BFM_WOODS_KING_BITE_SOUND = 0xad3,
       BFM_WOODS_GUARD_SOUND = 0x43d, BFM_WOODS_HURT_SOUND = 0x451,
       BFM_WOODS_WATER_ENTRY_SOUND = 0x441, BFM_WOODS_CARRY_THROW_SOUND = 0x5f5 };
enum { BFM_WOODS_TERRAIN_HIT_FIRST = 0x426, BFM_WOODS_TERRAIN_HIT_LAST = 0x42d,
       BFM_WOODS_FUSION_FINISH_HIT_SOUND = 0x438,
       BFM_WOODS_FUSION_FOLLOWUP_SOUND = 0x446, BFM_WOODS_LUMINA_SWING_SOUND = 0x455,
       BFM_WOODS_PICKUP_SOUND = 0x452, BFM_WOODS_RING_SOUND = 0x522,
       BFM_WOODS_PINK_BITE_SOUND = 0x524, BFM_WOODS_LENO_BARK_SOUND = 0x525,
       BFM_WOODS_LENO_YELP_SOUND = 0x526, BFM_WOODS_SPLASH_SOUND = 0x531,
       BFM_WOODS_SATIATE_RECOVERY_SOUND = 0x5fb, BFM_WOODS_CHARGED_SLASH_SOUND = 0x9da };
enum { BFM_WOODS_CRYSTAL_HIT_SOUND = 1581, BFM_WOODS_CRYSTAL_BREAK_SOUND = 1582,
       BFM_WOODS_RESCUE_DEPART_SOUND = 1637, BFM_WOODS_RESCUE_VANISH_SOUND = 1638,
       BFM_WOODS_RESCUE_BP_SOUND = 3070 };
/* The room's ordered sound stream carries native key-off requests above the
 * sixteen-bit disc selectors. They release only matching active sounds. */
enum { BFM_WOODS_CHARGE_SOUND = 1100, BFM_WOODS_FULL_CHARGE_SOUND = 1101,
       BFM_WOODS_STOP_CHARGE_SOUND = 0x10000 + BFM_WOODS_CHARGE_SOUND,
       BFM_EARTH_SCROLL_RINGS_SOUND = 0x639,
       BFM_EARTH_SCROLL_STOP_RINGS_SOUND = 0x10000 + BFM_EARTH_SCROLL_RINGS_SOUND };
enum { BFM_NIGHTMARE_WAKE_SOUND = 0x7e7, BFM_NIGHTMARE_BED_LANDING_SOUND = 0x434,
       BFM_NIGHTMARE_FLOOR_LANDING_SOUND = 0x42e };
/* Prepare the wake cue and the bed and floor contact sounds from SC03 member 3. */
BfmBedroomEffects *bfm_nightmare_sound_load(const MusashiDiscMedia *disc);
enum { BFM_MAZE_GUIDE_SNIFF_SOUND = 0x527, BFM_MAZE_GUIDE_ABSENT_SOUND = 0xc4d };
BfmBedroomEffects *bfm_forest_bank_effects_load(const MusashiDiscMedia *disc);
BfmBedroomEffects *bfm_maze_guide_effects_load(const MusashiDiscMedia *disc);
enum { BFM_VALLEY_EARTH_LANDING_SOUND = 0x47d, BFM_VALLEY_EARTH_RING_SOUND = 0x47e,
       BFM_VALLEY_BOULDER_START_SOUND = 0x5c2, BFM_VALLEY_BOULDER_IMPACT_SOUND = 0x5c3,
       BFM_VALLEY_HINGE_IMPACT_SOUND = 0x651, BFM_VALLEY_GATE_SLIDE_SOUND = 0x5c4,
       BFM_VALLEY_SOLDIER_LAND_SOUND = 0x63f, BFM_VALLEY_KNITTER_DEPART_SOUND = 0xac3,
       BFM_VALLEY_KNITTER_VANISH_SOUND = 0xac4, BFM_VALLEY_AWARD_RAISE_SOUND = 0x843 };
BfmBedroomEffects *bfm_valley_effects_load(const MusashiDiscMedia *disc);
BfmBedroomEffects *bfm_woods_shrink_sound_load(const MusashiDiscMedia *disc);
enum { BFM_WOODS_BOX_OPEN_SOUND = 1471, BFM_WOODS_BOX_CLOSE_SOUND = 2184,
       BFM_WOODS_BOX_ACCEPT_SOUND = 0xb3b, BFM_ROOTRICK_IMPACT_SOUND = 0x5ca,
       BFM_ROOTRICK_STUMBLE_LANDING_SOUND = 0xb09,
       BFM_WALL_CLIMB_FIRST_SOUND = 0x56d, BFM_WALL_CLIMB_SECOND_SOUND = 0x56e };
BfmBedroomEffects *bfm_woods_box_sound_load(const MusashiDiscMedia *disc);
enum { BFM_FIELD_MENU_ROW_SOUND = 0x45a, BFM_FIELD_MENU_HEAL_SOUND = 0x45c,
       BFM_FIELD_MENU_ERROR_SOUND = 0x45f, BFM_FIELD_MENU_COMMIT_SOUND = 0x462 };
enum { BFM_ROOM_MENU_DUCK = 20, BFM_ROOM_MENU_RESTORE = 21 };
BfmBedroomEffects *bfm_village_illness_effects_load(const MusashiDiscMedia *disc);
BfmBedroomEffects *bfm_village_knock_sound_load(const MusashiDiscMedia *disc);
BfmBedroomEffects *bfm_forest_swing_sound_load(const MusashiDiscMedia *disc);
BfmBedroomEffects *bfm_bakery_effects_load(const MusashiDiscMedia *disc);
BfmBedroomEffects *bfm_menu_effects_load(const MusashiDiscMedia *disc);
/* The castle command and rescued-person menus use the mounted common sound bank. */
BfmBedroomEffects *bfm_castle_menu_effects_load(const MusashiDiscMedia *disc);
BfmBedroomEffects *bfm_bedroom_effects_load(const MusashiDiscMedia *disc, unsigned member, uint32_t room);
void bfm_bedroom_effects_free(BfmBedroomEffects *effects);
/* Serialized with the sound callback. All source decoding occurs during load. */
void bfm_bedroom_effects_stop(BfmBedroomEffects *effects);
void bfm_bedroom_effects_start(BfmBedroomEffects *effects, unsigned command);
void bfm_bedroom_effects_mix(BfmBedroomEffects *effects, int16_t *stereo, size_t frames);
#endif
