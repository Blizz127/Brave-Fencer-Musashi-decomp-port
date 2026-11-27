/* SPDX-License-Identifier: MIT */
#include "woods_carry.h"
#include "../pc_port/forest_player_bytes.h"

static const uint8_t *mounted(const uint8_t *scene, size_t size, uint32_t address, size_t length)
{
    const uint32_t base = UINT32_C(0x80128158);
    if (!scene || address < base || address-base > size || length > size-(address-base)) return NULL;
    return scene+(address-base);
}

int bfm_woods_carry_load(BfmWoodsCarry *carry, const MusashiDiscMedia *disc,
                         const uint8_t *scene, size_t size)
{
    BfmWoodsCarry result = {0};
    BfmForestLiftData *data = &result.data;
    const uint8_t *room = mounted(scene,size,UINT32_C(0x8018ea38),0x50);
    const uint8_t *velocity = mounted(scene,size,UINT32_C(0x80189e70),12);
    const uint8_t *grab = mounted(scene,size,UINT32_C(0x80189874)+10*12,4);
    const uint8_t *grip = mounted(scene,size,UINT32_C(0x8018b7e4)+12,6);
    if (!carry || !room || !velocity || !grab || !grip ||
        !bfm_forest_lift_motions(disc,u32le(room+0x4c),data)) return 0;
    data->raise_offsets = mounted(scene,size,UINT32_C(0x8018a0f0),
                                  3*(data->motion[BFM_LIFT_RAISE].count-1));
    data->toss_offsets = mounted(scene,size,UINT32_C(0x8018a12c),
                                 3*data->motion[BFM_LIFT_THROW].event);
    uint32_t raw = u32le(velocity+8);
    if (!data->raise_offsets || !data->toss_offsets || raw <= UINT32_C(0x80000000) ||
        u32le(velocity) || u32le(velocity+4)) return 0;
    data->walking_speed = -(int32_t)(0u-raw);
    data->grab_base = u16le(grab) & 4095;
    data->grab_spread = u16le(grab+2);
    for (unsigned i = 0; i < 3; ++i) result.grip[i] = s16le(grip+2*i);
    *carry = result;
    return 1;
}

int bfm_woods_carry_brace(BfmWoodsCarry *carry, int contacted, BfmWoodsCarryFrame *frame)
{
    if (!carry || !frame || carry->lift.target) return 0;
    *frame = (BfmWoodsCarryFrame){0};
    const BfmLiftMotion *clip = &carry->data.motion[BFM_LIFT_BRACE];
    if (!clip->count) return 0;
    if (contacted) {
        carry->brace_age = carry->braced ? (carry->brace_age+1) % clip->count : 0;
        frame->pose = clip->frames[carry->brace_age];
        frame->face_slot = 23;
    }
    carry->braced = !!contacted;
    return 1;
}

int bfm_woods_carry_reach(const BfmWoodsCarry *carry, const BfmForestTransform *hero,
                          int square, int triangle, int lumina, int16_t end[3])
{
    const int16_t reach[3] = {0,-16,-32};
    if (!carry || !hero || !end || !carry->braced || carry->lift.target ||
        !(square || (triangle && lumina))) return 0;
    bfm_forest_lift_world(hero,reach,end);
    return 1;
}

int bfm_woods_carry_begin(BfmWoodsCarry *carry, BfmGameplayRun *run, BfmWoodsCarryFrame *frame)
{
    if (!carry || !run || !frame || !carry->braced || carry->lift.target) return 0;
    carry->lift = (BfmForestLift){.target=1,.clip=BFM_LIFT_RAISE};
    for (unsigned i = 0; i < 3; ++i) carry->lift.grip[i] = carry->grip[i];
    carry->braced = carry->released = carry->triangle_throw = 0;
    *frame = (BfmWoodsCarryFrame){
        .event=BFM_CARRY_LIFT, .face_slot=17, .pose=carry->data.motion[BFM_LIFT_RAISE].frames[0],
        .interaction_power=carry->data.grab_base +
            bfm_gameplay_random(run) % (carry->data.grab_spread+1u)
    };
    return 1;
}

int bfm_woods_carry_step(BfmWoodsCarry *carry, BfmGameplayRun *run,
                         const BfmForestTransform *hero, int directed, int square,
                         int triangle, int lumina, BfmWoodsCarryFrame *frame)
{
    if (!carry || !run || !hero || !frame || !carry->lift.target) return 0;
    BfmForestLift *lift = &carry->lift;
    BfmLiftClip before = lift->clip;
    int overhead = before == BFM_LIFT_WALK || before == BFM_LIFT_IDLE;
    int throwing = overhead && (square || (triangle && lumina));
    BfmLiftStep step = bfm_forest_lift_tick(lift,&carry->data,directed,throwing);
    *frame = (BfmWoodsCarryFrame){.turn=overhead && directed};
    if (step == BFM_LIFT_FINISHED) {
        lift->target = 0;
        frame->face_slot = 17;
        frame->finished = 1;
        return 1;
    }
    if (throwing) {
        carry->triangle_throw = !square;
        frame->started_throw = 1;
        frame->face_slot = 25;
        bfm_forest_status_spend_bp(&run->player,4);
    } else if (lift->clip == BFM_LIFT_RAISE &&
               lift->age == carry->data.motion[BFM_LIFT_RAISE].event) {
        frame->face_slot = 23;
    }
    frame->pose = carry->data.motion[lift->clip].frames[lift->age];
    if (step == BFM_LIFT_RELEASE) {
        carry->released = 1;
        frame->event = carry->triangle_throw ? BFM_CARRY_DROP : BFM_CARRY_THROW;
    } else if (step == BFM_LIFT_ATTACH || step == BFM_LIFT_WALK_ATTACH) {
        if (!bfm_forest_lift_grip(&carry->data,lift,hero,frame->socket)) return 0;
        frame->attached = 1;
        if (step == BFM_LIFT_WALK_ATTACH) frame->speed_q16 = -carry->data.walking_speed;
    }
    return 1;
}

int bfm_woods_carry_active(const BfmWoodsCarry *carry)
{
    return carry && carry->lift.target;
}

BfmCarryEvent bfm_woods_carry_interrupt(BfmWoodsCarry *carry)
{
    BfmCarryEvent event = BFM_CARRY_NONE;
    if (carry) {
        if (carry->lift.target && !carry->released)
            event = carry->lift.clip == BFM_LIFT_THROW && !carry->triangle_throw ?
                    BFM_CARRY_THROW : BFM_CARRY_DROP;
        carry->lift.target = carry->braced = 0;
    }
    return event;
}
