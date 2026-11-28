/* SPDX-License-Identifier: MIT */
#include "village_illness_mine_hero.h"

#include <stdlib.h>
#include <string.h>

struct BfmVillageMineHero {
    BfmVillageMotion *motion;
    BfmBedroomTravel *travel;
    /* The longest mounted gate script issues five orders altogether. */
    BfmMineGateOrder pending[5];
    unsigned count;
    uint32_t pose;
    int pose_ended;
};

BfmVillageMineHero *bfm_village_mine_hero_open(const uint8_t *scene, size_t bytes,
                                              BfmVillageMotion *motion)
{
    const BfmVillagePlayer *player = bfm_village_motion_player(motion);
    if (!player || !player->controlled || player->phase != BFM_VILLAGE_CONTROL ||
        (player->events & BFM_VILLAGE_TRANSFER)) return NULL;
    int32_t xyz[3];
    if (!bfm_village_motion_position_q16(motion,xyz)) return NULL;
    BfmVillageMineHero *hero = calloc(1,sizeof *hero);
    if (!hero) return NULL;
    hero->motion = motion;
    hero->travel = bfm_bedroom_travel_from_root(scene,bytes,UINT32_C(0x80190780),&player->root);
    if (!bfm_bedroom_travel_seed_position(hero->travel,xyz)) {
        bfm_village_mine_hero_free(hero);
        return NULL;
    }
    return hero;
}

void bfm_village_mine_hero_free(BfmVillageMineHero *hero)
{
    if (hero) bfm_bedroom_travel_free(hero->travel);
    free(hero);
}

int bfm_village_mine_hero_step(BfmVillageMineHero *hero, const BfmVillageCamera *view)
{
    if (!hero || !view || !bfm_bedroom_travel_step(hero->travel,view->eye,view->target))
        return 0;
    const BfmBedroomRoot *root = bfm_bedroom_travel_root(hero->travel);
    const int64_t *world = bfm_bedroom_travel_position(hero->travel);
    int32_t xyz[3] = {(int32_t)world[0],(int32_t)world[1],(int32_t)world[2]};
    if (!bfm_village_motion_staged_position(hero->motion,xyz)) return 0;
    /* The player tests the previous body marker before this field's body and
     * face update. Sound and dialogue in the director run independently. */
    if (hero->pose_ended) {
        if (!bfm_village_motion_finish_staged_program(hero->motion)) return 0;
        hero->pose = 0;
        hero->pose_ended = 0;
    }
    if (hero->pose)
        return bfm_village_motion_staged_program(hero->motion,root,hero->pose,&hero->pose_ended);
    return bfm_village_motion_staged(hero->motion,root);
}

static int select_order(BfmVillageMineHero *hero, const BfmVillageCamera *view)
{
    const BfmMineGateOrder *order = hero->pending;
    switch (order->type) {
    case BFM_MINE_GATE_WALK:
        if (!bfm_bedroom_travel_walk(hero->travel,order->x,order->z,0)) return 0;
        break;
    case BFM_MINE_GATE_TURN:
        if (!bfm_bedroom_travel_turn(hero->travel,order->yaw,0)) return 0;
        break;
    case BFM_MINE_GATE_POSE:
        if (!order->body_program) return 0;
        hero->pose = order->body_program;
        return 1;
    default:
        return 0;
    }
    /* Retire only the travel dispatch. Its first physical walk/turn runs
     * on the following player visit. */
    return bfm_bedroom_travel_step(hero->travel,view->eye,view->target);
}

int bfm_village_mine_hero_orders(BfmVillageMineHero *hero, const BfmMineGateFrame *frame,
                                const BfmVillageCamera *view)
{
    if (!hero || !frame || !view || frame->order_count > sizeof frame->orders / sizeof *frame->orders)
        return 0;
    unsigned dispatch = hero->count && !hero->pose && !bfm_bedroom_travel_root(hero->travel)->busy;
    if (hero->count - dispatch + frame->order_count > sizeof hero->pending / sizeof *hero->pending)
        return 0;
    if (dispatch) {
        if (!select_order(hero,view)) return 0;
        --hero->count;
        memmove(hero->pending,hero->pending+1,hero->count*sizeof *hero->pending);
    }
    memcpy(hero->pending+hero->count,frame->orders,frame->order_count*sizeof *hero->pending);
    hero->count += frame->order_count;
    return 1;
}

int bfm_village_mine_hero_ready(const BfmVillageMineHero *hero)
{
    return hero && !hero->count && !hero->pose && !bfm_bedroom_travel_root(hero->travel)->busy;
}

int bfm_village_mine_hero_release(BfmVillageMineHero *hero)
{
    return bfm_village_mine_hero_ready(hero) && bfm_village_motion_release_staged_field(hero->motion);
}
