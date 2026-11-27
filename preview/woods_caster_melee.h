/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_CASTER_MELEE_H
#define BFM_WOODS_CASTER_MELEE_H

#include "bedroom_travel.h"
#include "gameplay_run.h"

typedef struct BfmWoodsCasterMelee BfmWoodsCasterMelee;

typedef struct {
    int permitted, square_edge, triangle_edge, moving, jump_edge;
    uint32_t actor_flags;
    /* The guard receiver alone selects the charged cut. A fresh attack in
     * ordinary stance continues to use the regular Square/Triangle actions. */
    int charged_counter, cut_support_changed;
} BfmWoodsCasterMeleeInput;

typedef struct {
    unsigned action, event, strength, critical, rays, sound;
    int16_t from[3], to[3][3];
    uint32_t pose; /* Joint data address; the pose packet starts 16 bytes earlier. */
    int show_fusion, show_lumina;
    int charge_started, charge_marker;
    uint32_t selected_pose; /* Current logical body key used by effect actors. */
    unsigned face_slot; /* Zero preserves the field motion's facial channel. */
} BfmWoodsCasterMeleeFrame;

/* The room and player archive are the already unpacked US SC03/29 and MAIN
 * player payload. Strike endpoints and power come from their mounted data. */
BfmWoodsCasterMelee *bfm_woods_caster_melee_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t scene_bytes, const uint8_t *main, size_t main_bytes);
/* Mount Hell's Valley's ordinary Fusion and Lumina attack tables. */
BfmWoodsCasterMelee *bfm_valley_melee_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t scene_bytes, const uint8_t *main, size_t main_bytes);
/* Mount Twinpeak's ordinary Fusion and Lumina slots for field-script input. */
BfmWoodsCasterMelee *bfm_woods_twinpeak_melee_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t scene_bytes, const uint8_t *main, size_t main_bytes);
void bfm_woods_caster_melee_free(BfmWoodsCasterMelee *melee);
/* Supply an ordinary grounded, empty-handed control slot after interactions,
 * carried objects and charged skills have taken input. A nonzero action holds
 * the hero's ground movement; a recovery cancel releases it on this step.
 * A single strength applies to all rays in one field update. */
int bfm_woods_caster_melee_step(BfmWoodsCasterMelee *melee, BfmGameplayRun *run,
    const BfmBedroomRoot *root, const BfmWoodsCasterMeleeInput *input,
    BfmWoodsCasterMeleeFrame *frame);
int bfm_woods_caster_melee_cut_finishing(const BfmWoodsCasterMelee *melee);
void bfm_woods_caster_melee_connected(BfmWoodsCasterMelee *melee);

#endif
