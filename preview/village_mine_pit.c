/* SPDX-License-Identifier: MIT */
#include "village_mine_pit.h"

enum { DESCENDING = 2, RETURNING = 3, Q16 = 65536 };

void bfm_village_mine_pit_remember(BfmVillageMinePit *pit, const BfmVillageGround *ground,
                                   const int16_t foot[3])
{
    if (pit) bfm_forest_bank_thorn_remember(&pit->recovery, ground, foot);
}

int bfm_village_mine_pit_step(BfmVillageMinePit *pit, BfmGameplayRun *run, uint16_t material,
    int32_t vertical_velocity_q16, const BfmForestBankThornInput *in, BfmForestBankThornFrame *frame)
{
    if (!pit || !run || !in || !in->position || !in->ground || !in->entrance || !frame) return 0;
    BfmForestBankThornHurt *rescue = &pit->recovery;
    /* Both hazards rejoin the same return and safe-floor search states. */
    if (rescue->phase >= RETURNING) {
        if (!bfm_forest_bank_thorn_hurt_step(rescue, run, in, frame)) return 0;
        if (frame->recovered) {
            rescue->vertical_speed = 0;
            frame->phase = 1;
        }
        return 1;
    }

    *frame = (BfmForestBankThornFrame){.yaw = (unsigned)in->yaw & 4095u, .action = in->player_action};
    if (!rescue->phase) {
        if (material != 0x021c || (in->scene_state & UINT32_C(0x80ffffff)) ||
            in->player_action == 25 || in->player_action == 26 ||
            (in->protected_action && !(in->protected_action & UINT32_C(0x02000000)))) return 1;
        BfmForestStatusValues status = bfm_forest_status_values(&run->player);
        if (status.hp <= 0) return 1;
        if (!bfm_forest_hit_damage(in->disc, &run->player, in->player_flags, 24,
                                   &rescue->damage)) return 0;
        if (run->player.record[0x49] == 20) {
            int bp = status.bp + 10;
            if (bp > status.max_bp) bp = status.max_bp;
            run->player.record[0x40] = (uint8_t)bp;
            run->player.record[0x41] = (uint8_t)((unsigned)bp >> 8);
            frame->bp_credit = 10;
            for (int axis = 0; axis < 3; ++axis)
                frame->effect_root[axis] = (int16_t)(in->position[axis] >> 16);
        }
        rescue->phase = DESCENDING;
        rescue->ticks = 64;
        rescue->vertical_speed = vertical_velocity_q16;
        frame->started = 1;
        frame->rumble_program = UINT32_C(0x80062c04);
    }

    frame->action = 25;
    frame->movement_locked = 1;
    if (rescue->vertical_speed < 31 * Q16) rescue->vertical_speed += Q16 + Q16 / 2;
    in->position[1] += rescue->vertical_speed;
    if ((in->position[1] >> 16) > 512 || !rescue->ticks) {
        rescue->phase = RETURNING;
        rescue->ticks = 90;
        frame->hide_body = 1;
    } else {
        --rescue->ticks;
    }
    frame->phase = rescue->phase;
    return 1;
}
