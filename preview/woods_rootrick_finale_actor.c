/* SPDX-License-Identifier: MIT */
#include "woods_rootrick_finale_actor.h"
#include "bedroom_travel.h"
#include "../pc_port/forest_player_bytes.h"

#include <limits.h>
#include <string.h>

enum {
    SCRIPT_ONLY, BOULDER_STRUCK_RIVAL, TUMBLING, FLASHING,
    BOULDER_STRUCK_HERO, WAIT_FOR_SWORD, RECOILING
};
enum { FALL_ACCEL = 98304 };

int bfm_woods_rootrick_finale_begin(BfmWoodsRootrickFinale *f, BfmWoodsRootrickActor *a,
    unsigned outcome, int visible, const uint8_t *scene, size_t bytes)
{
    const size_t room = 0x80190ce4u - 0x80128158u;
    const size_t recoil = 0x80195138u - 0x80128158u;
    const size_t probe = 0x8019514cu - 0x80128158u;
    if (!f || !a || !a->ground || !scene || outcome < 1 || outcome > 3 ||
        bytes < probe + 6 || u32le(scene + room) != UINT32_C(0x801961e8)) return 0;
    *f = (BfmWoodsRootrickFinale){.scene = scene, .bytes = bytes, .outcome = outcome,
                                  .visible = visible != 0};
    for (unsigned i = 0; i < 3; ++i) {
        f->recoil[i] = (int32_t)u32le(scene + recoil + 4 * i);
        f->rim_probe[i] = s16le(scene + probe + 2 * i);
    }
    a->turn_fields = 0;
    return 1;
}

int bfm_woods_rootrick_finale_boulder_hit(BfmWoodsRootrickFinale *f, unsigned variant)
{
    if (!f || f->phase != SCRIPT_ONLY || variant > 1 || f->outcome != variant + 1) return 0;
    f->phase = variant ? BOULDER_STRUCK_RIVAL : BOULDER_STRUCK_HERO;
    return 1;
}

int bfm_woods_rootrick_finale_stage(BfmWoodsRootrickFinale *f, unsigned stage)
{
    if (!f || f->outcome != 3 || f->phase != SCRIPT_ONLY || stage != 5) return 0;
    f->phase = WAIT_FOR_SWORD;
    return 1;
}

void bfm_woods_rootrick_finale_turn(BfmWoodsRootrickFinale *f,
    BfmWoodsRootrickActor *a, int16_t relative_yaw)
{
    f->drive_fields = 0;
    a->target_yaw = (unsigned)((int)a->yaw + relative_yaw) & 4095u;
    a->turn_fields = 30;
}

void bfm_woods_rootrick_finale_drive(BfmWoodsRootrickFinale *f,
    BfmWoodsRootrickActor *a, int16_t speed_q8, unsigned fields)
{
    a->turn_fields = 0;
    f->drive_speed = (int32_t)speed_q8 * 256;
    f->drive_fields = fields;
}

static int advance_drive(BfmWoodsRootrickFinale *f, BfmWoodsRootrickActor *a)
{
    if (f->drive_fields) {
        int32_t stride[3] = {0, 0, f->drive_speed};
        unsigned contact;
        bfm_woods_rootrick_actor_impulse(a, stride, a->gravity);
        if (!bfm_woods_rootrick_actor_move(a, NULL, &contact) || a->xyz[1] < INT16_MIN + 8)
            return 0;
        a->xyz[1] -= 8;
        /* Use the same 768-unit ray as the cast's shadow, with the regular
         * floor probe's starting height. A projected step discards fractions. */
        if (a->xyz[1] > INT16_MAX - 16) return 0;
        BfmBedroomRoot support = {.xyz = {a->xyz[0], a->xyz[1] + 16, a->xyz[2]}};
        if (!bfm_bedroom_travel_air_shadow(f->scene, f->bytes, UINT32_C(0x801961e8), &support))
            return 0;
        if (support.shadow_visible) {
            a->xyz[1] = support.shadow_xyz[1];
            memset(a->fraction, 0, sizeof a->fraction);
        }
        --f->drive_fields;
    } else {
        bfm_woods_rootrick_actor_step_turn(a);
    }
    return 1;
}

static void begin_fall(BfmWoodsRootrickActor *a)
{
    memset(a->velocity, 0, sizeof a->velocity);
    a->gravity = FALL_ACCEL;
}

int bfm_woods_rootrick_finale_step(BfmWoodsRootrickFinale *f, BfmWoodsRootrickActor *a,
                                   int hero_qualified, int rival_hit)
{
    static const int16_t feet[3];
    if (!f || !a || !a->ground || !f->outcome) return BFM_ROOTRICK_FINALE_ERROR;
    if (!a->pose_held && a->elapsed < UINT_MAX) ++a->elapsed;
    unsigned contact;
    int events = 0;
    switch (f->phase) {
    case SCRIPT_ONLY:
        break;
    case BOULDER_STRUCK_RIVAL:
        begin_fall(a);
        f->phase = TUMBLING;
        break;
    case BOULDER_STRUCK_HERO:
        if (hero_qualified) {
            f->phase = SCRIPT_ONLY;
            events = BFM_ROOTRICK_FINALE_RELEASE | BFM_ROOTRICK_FINALE_STOP_RUMBLE;
        }
        break;
    case WAIT_FOR_SWORD:
        if (rival_hit) {
            a->pose_held = 1;
            bfm_woods_rootrick_actor_impulse(a, f->recoil, FALL_ACCEL);
            f->phase = RECOILING;
            events = BFM_ROOTRICK_FINALE_CONSUMED_HIT | BFM_ROOTRICK_FINALE_CRY;
        }
        break;
    case RECOILING:
        if (!bfm_woods_rootrick_actor_walk(a, f->rim_probe, &contact))
            return BFM_ROOTRICK_FINALE_ERROR;
        if (!(contact & 0x2000)) {
            begin_fall(a);
            bfm_woods_rootrick_actor_bind_clip(a, UINT32_C(0x801f4654));
            f->phase = TUMBLING;
        }
        break;
    case TUMBLING: {
        int direction = f->outcome == 3 ? 1 : -1;
        a->pitch += direction * 32;
        if (a->pitch * direction > 1024) a->pitch = direction * 1024;
        if (!bfm_woods_rootrick_actor_move(a, feet, &contact))
            return BFM_ROOTRICK_FINALE_ERROR;
        if (contact & 0x2000) {
            a->pitch = direction * 1024;
            f->phase = FLASHING;
            f->flash_fields = 60;
            events = BFM_ROOTRICK_FINALE_SHAKE;
        }
        break;
    }
    case FLASHING:
        f->visible = !f->visible;
        if (!--f->flash_fields) {
            f->phase = SCRIPT_ONLY;
            events = BFM_ROOTRICK_FINALE_RELEASE | BFM_ROOTRICK_FINALE_STOP_RUMBLE |
                     BFM_ROOTRICK_FINALE_REMOVE_SHADOW;
        }
        break;
    default:
        return BFM_ROOTRICK_FINALE_ERROR;
    }
    return advance_drive(f, a) ? events : BFM_ROOTRICK_FINALE_ERROR;
}
