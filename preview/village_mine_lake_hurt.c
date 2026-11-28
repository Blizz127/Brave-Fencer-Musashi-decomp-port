/* SPDX-License-Identifier: MIT */
#include "village_mine_lake_hurt.h"
#include "village_mine_pit.h"
#include "woods_player_hit.h"
#include "woods_revive.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>
#include <string.h>

enum {
    RETRY_TABLE = 0x801840dc - 0x80128158,
    SAVED_EARLY = 0x801842c8 - 0x80128158,
    SAVED_ILLNESS = 0x801842ec - 0x80128158
};

struct BfmMineLakeHurt {
    const MusashiDiscMedia *disc;
    const uint8_t *scene;
    size_t bytes;
    BfmVillageMotion *motion;
    BfmSaveRecord *save;
    BfmSwitchbackHit *hit;
    BfmMineLungeHit incoming;
    BfmVillageMinePit pit;
    BfmForestBankThornFrame floor;
    int dying;
    BfmRetryScreen *retry;
};

BfmMineLakeHurt *bfm_mine_lake_hurt_open(const MusashiDiscMedia *disc, const uint8_t *scene,
    size_t bytes, BfmVillageMotion *motion, BfmSaveRecord *save)
{
    const size_t binding = 0x80181d20u - 0x80128158u;
    const BfmVillagePlayer *p = bfm_village_motion_player(motion);
    if (!disc || !scene || !p || !save || bytes < SAVED_ILLNESS + sizeof save->destination ||
        u32le(scene + binding) != 0x80185a50u || u32le(scene + SAVED_EARLY) != 0x3012 ||
        u32le(scene + SAVED_ILLNESS) != 0x3054) return NULL;
    BfmMineLakeHurt *h = calloc(1, sizeof *h);
    if (!h) return NULL;
    h->hit = bfm_switchback_hit_open_lake(disc, scene, bytes, p->root.xyz);
    if (!h->hit) { free(h); return NULL; }
    h->disc = disc;
    h->scene = scene;
    h->bytes = bytes;
    h->motion = motion;
    h->save = save;
    return h;
}

void bfm_mine_lake_hurt_free(BfmMineLakeHurt *h)
{
    if (!h) return;
    bfm_switchback_hit_free(h->hit);
    bfm_retry_screen_free(h->retry);
    free(h);
}

void bfm_mine_lake_hurt_queue(BfmMineLakeHurt *h, const BfmMineLungeHit *incoming)
{
    if (h && incoming) h->incoming = *incoming;
}

int bfm_mine_lake_hurt_step(BfmMineLakeHurt *h, BfmGameplayRun *run, BfmFieldInput input,
    unsigned camera_yaw, unsigned field)
{
    if (!h) return -1;
    if (!run || !run->has_player) return 0;
    if (h->retry) return 1;
    const BfmVillagePlayer *p = bfm_village_motion_player(h->motion);
    int busy = !p->controlled || bfm_village_motion_woods_departing(h->motion);
    BfmMineLungeHit incoming = h->incoming;
    h->incoming = (BfmMineLungeHit){0};
    memcpy(run->player.record, h->save->progress, sizeof h->save->progress);
    const uint8_t *status = run->player.record;
    unsigned flags = status[0x23] & 128u ? 0x400u : 0u;
    if (status[0x49] == 16 && u32le(status + 0x50)) flags |= 0x800u;
    BfmWoodsPlayerHitInput check = {.root = &p->root, .player_flags = flags,
        .protected_action = busy, .player_action = h->floor.action};
    h->dying = bfm_woods_player_hit_can_check_defeat(run, &check);
    if (h->dying) return 1;

    unsigned material = bfm_village_motion_mine_pit_material(h->motion);
    /* Environmental damage occupies the pending hit slot before the player
       dispatches the prior creature hit, including when the fall is rejected. */
    if (!busy && (material & 255) == 28) incoming = (BfmMineLungeHit){0};
    BfmForestBankThornInput floor_input = {.disc = h->disc, .field = field,
        .scene_state = busy, .protected_action = busy, .player_flags = flags,
        .player_action = h->floor.action};
    if (!bfm_village_motion_mine_pit(h->motion, &h->pit, run, floor_input,
                                     bfm_switchback_hit_can_fall(h->hit), &h->floor)) return -1;
    if (!h->floor.movement_locked)
        return bfm_switchback_hit_step(h->hit, h->motion, run, h->save, input, camera_yaw, &incoming);
    if (!bfm_switchback_hit_environment(h->hit, run, &p->root, &h->floor)) return -1;
    memcpy(h->save->progress, run->player.record, sizeof h->save->progress);
    return 1;
}

const int16_t *bfm_mine_lake_hurt_target(BfmMineLakeHurt *h)
{
    return h ? bfm_switchback_hit_target(h->hit, h->motion) : NULL;
}

int bfm_mine_lake_hurt_body(const BfmMineLakeHurt *h, const uint8_t *main, size_t bytes,
    BfmBedroomPose *pose, unsigned *visible)
{
    if (!h || !main || !pose || !visible || !bfm_switchback_hit_body(h->hit, main, bytes, pose, visible))
        return 0;
    if (h->floor.hide_body || h->retry) *visible = 0;
    return 1;
}

int bfm_mine_lake_hurt_draw(const BfmMineLakeHurt *h, BfmBedroomFigure *figure,
    const BfmBedroomView *view)
{
    return h && bfm_switchback_hit_draw(h->hit, figure, view);
}

unsigned bfm_mine_lake_hurt_take_sound(BfmMineLakeHurt *h)
{
    return h ? bfm_switchback_hit_take_sound(h->hit) : 0;
}

int bfm_mine_lake_hurt_defeated(const BfmMineLakeHurt *h)
{
    return h && h->dying;
}

BfmMineLakeLife bfm_mine_lake_hurt_resolve(BfmMineLakeHurt *h, BfmGameplayRun *run,
    BfmVillageFieldDefeat *actor, const BfmVillageCamera *presented,
    int item_blocked, BfmForestMessage **notice)
{
    if (!h || !h->dying || !run || !run->has_player || !actor || !presented || !notice || *notice)
        return BFM_MINE_LAKE_LIFE_ERROR;
    if (h->retry) return BFM_MINE_LAKE_RETRY;
    unsigned slot;
    BfmWoodsReviveResult item = bfm_woods_revive_prepare(run, h->save, item_blocked, actor->main,
        actor->main_bytes, &slot, notice);
    if (item == BFM_WOODS_REVIVE_ERROR) return BFM_MINE_LAKE_LIFE_ERROR;
    if (item == BFM_WOODS_REVIVE_READY) {
        uint8_t *status = run->player.record;
        if (!bfm_village_motion_woods_revive(h->motion, bfm_field_tired_stance(status))) {
            bfm_forest_message_free(*notice);
            *notice = NULL;
            return BFM_MINE_LAKE_LIFE_ERROR;
        }
        if (status[0x49] == 3 || status[0x49] == 12 || status[0x49] == 24) {
            memset(status + 0x49, 0, 4);
            memset(h->save->progress + 0x49, 0, 4);
        }
        bfm_woods_revive_restore(run, h->save, slot);
        bfm_village_motion_woods_condition(h->motion, bfm_field_tired_stance(status));
        h->dying = 0;
        return BFM_MINE_LAKE_REVIVED;
    }
    if (!bfm_village_motion_mine_release_rider(h->motion)) return BFM_MINE_LAKE_LIFE_ERROR;
    h->retry = bfm_village_field_defeat_open(actor, h->disc, h->scene, h->bytes, RETRY_TABLE,
                                             presented, run);
    return h->retry ? BFM_MINE_LAKE_RETRY : BFM_MINE_LAKE_LIFE_ERROR;
}

BfmRetryScreen *bfm_mine_lake_hurt_retry(const BfmMineLakeHurt *h)
{
    return h ? h->retry : NULL;
}

int bfm_mine_lake_hurt_restore(const BfmMineLakeHurt *h, const BfmGameplayRun *run,
    BfmSaveRecord *out)
{
    if (!h || !h->retry || !run || !out) return 0;
    BfmWoodsRetryPage choice = bfm_retry_screen_woods_destination(h->retry);
    if (choice == BFM_WOODS_RESTORE_BOX)
        return bfm_gameplay_field_recovery(run, BFM_CHECKPOINT_MEMORY_BOX, NULL, 0, 0, out);
    if (choice != BFM_WOODS_RESTORE_SAVED || !run->saved_data.present) return 0;
    const BfmSaveRecord *saved = &run->saved_data.state;
    unsigned story = u32le(saved->progress + 8);
    if (story < 10 || story >= 600) return 0;
    /* Saved Data restores its own banks, then takes the numbered entrance
       from the source room's table for the restored story. */
    const uint8_t *destination = h->scene + (story < 200 ? SAVED_EARLY : SAVED_ILLNESS);
    *out = *saved;
    memcpy(out->destination, destination, sizeof out->destination);
    return 1;
}
