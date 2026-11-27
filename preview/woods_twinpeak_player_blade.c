/* SPDX-License-Identifier: MIT */
#include "woods_twinpeak_player_blade.h"
#include "field_rest.h"
#include "woods_fusion_art.h"

int bfm_twinpeak_player_field_ready(const BfmVillageMotion *motion, const BfmGameplayRun *run)
{
    const BfmVillagePlayer *player = bfm_village_motion_player(motion);
    return run && run->has_player && player && player->controlled && !player->root.busy &&
           bfm_village_motion_grounded(motion) && !bfm_village_motion_box_pole(motion) &&
           !bfm_village_motion_woods_departing(motion) && !bfm_village_motion_woods_sliding(motion) &&
           bfm_forest_status_values(&run->player).hp > 0;
}

int bfm_twinpeak_player_blade_step(BfmWoodsCasterMelee *blade, const BfmVillageMotion *motion,
    BfmSaveRecord *save, BfmGameplayRun *run, BfmFieldInput input, int field_free,
    BfmWoodsCasterMeleeFrame *swing)
{
    if (!blade || !motion || !save || !run || !swing) return 0;
    const BfmVillagePlayer *player = bfm_village_motion_player(motion);
    if (!player) return 0;
    int available = field_free && bfm_twinpeak_player_field_ready(motion, run);
    unsigned actor_flags = (save->progress[0x23] & 0x80) ? 0x400u : 0u;
    if (available && !actor_flags && !swing->action && input.select_pressed) {
        /* The unmodified low bits identify the equipped orb. Select changes
         * the HUD mode before the next action samples the player's equipment. */
        BfmFieldRest free_stance = {0};
        bfm_field_rest_input(&free_stance, save, (BfmFieldInput){.select_pressed=1});
        run->player.record[0x48] = save->progress[0x48];
    }

    BfmWoodsCasterMeleeInput attack = {
        .permitted=available,
        .square_edge=input.square_pressed, .triangle_edge=input.triangle_pressed,
        .moving=input.x || input.y, .jump_edge=input.cross_pressed,
        .actor_flags=actor_flags
    };
    return bfm_woods_caster_melee_step(blade, run, &player->root, &attack, swing);
}

int bfm_twinpeak_player_blade_pose(const BfmWoodsCasterMeleeFrame *swing,
    const uint8_t *main, size_t bytes, BfmBedroomPose *body)
{
    if (!swing || !body) return 0;
    if (!swing->pose) return 1;
    if (!bfm_woods_fusion_art_pose(main, bytes, swing->pose,
                                   (uint32_t)swing->show_fusion << 22, 0, body)) return 0;
    body->show_lumina = swing->show_lumina;
    return 1;
}
