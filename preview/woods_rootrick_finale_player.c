/* SPDX-License-Identifier: MIT */
#include "woods_rootrick_finale_player.h"
#include "bedroom_figure.h"
#include "field_waypoint.h"
#include "village_ground.h"
#include "woods_charged_hit.h"
#include "../pc_port/forest_player_bytes.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

typedef enum { WALK, TURN, RELATIVE_TURN, FACE, LIFT, ATTACK, CONTROL } Order;
typedef struct { Order order; int x, z; uint32_t value; } Request;
enum { ORDERS = 12 };

struct BfmRootrickFinalePlayer {
    BfmVillageMotion *motion;
    BfmWoodsCarry carry;
    BfmWoodsCasterMelee *melee;
    unsigned outcome, count, entered, walking;
    Request orders[ORDERS];
    unsigned socket_yaw;
    uint32_t granted_control_bits;
    int16_t socket[3];
};

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t addr, size_t n)
{
    const uint32_t base = UINT32_C(0x80128158);
    if (!scene || addr < base || addr-base > bytes || n > bytes-(addr-base)) return NULL;
    return scene+(addr-base);
}

BfmRootrickFinalePlayer *bfm_woods_rootrick_player_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, const uint8_t *main, size_t main_bytes,
    BfmVillageMotion *motion)
{
    const uint8_t *room = mounted(scene,bytes,UINT32_C(0x80190ce4),4);
    if (!room || !motion || !disc) return NULL;
    const uint8_t *bank = mounted(scene,bytes,u32le(room),0x50);
    const uint8_t *v = mounted(scene,bytes,UINT32_C(0x80193390),12);
    const uint8_t *grab = mounted(scene,bytes,UINT32_C(0x80192e0c),4);
    const uint8_t *grip = mounted(scene,bytes,UINT32_C(0x8019454c),6);
    if (!bank || !v || !grab || !grip || u32le(v) || u32le(v+4) || (int32_t)u32le(v+8) >= 0 ||
        (int32_t)u32le(v+8) < -11*65536)
        return NULL;
    BfmRootrickFinalePlayer *p = calloc(1,sizeof *p);
    if (!p) return NULL;
    p->motion = motion;
    BfmForestLiftData *data = &p->carry.data;
    if (!bfm_forest_lift_motions(disc,u32le(bank+0x4c),data)) goto fail;
    data->raise_offsets = mounted(scene,bytes,UINT32_C(0x80193610),
                                  3*(data->motion[BFM_LIFT_RAISE].count-1));
    data->toss_offsets = mounted(scene,bytes,UINT32_C(0x8019364c),
                                 3*data->motion[BFM_LIFT_THROW].event);
    if (!data->raise_offsets || !data->toss_offsets) goto fail;
    data->walking_speed = (int32_t)u32le(v+8);
    data->grab_base = u16le(grab) & 4095;
    data->grab_spread = u16le(grab+2);
    for (unsigned i = 0; i < 3; ++i) p->carry.grip[i] = s16le(grip+2*i);
    p->melee = bfm_woods_twinpeak_melee_open(disc,scene,bytes,main,main_bytes);
    if (p->melee) return p;
fail:
    bfm_woods_rootrick_player_free(p);
    return NULL;
}

void bfm_woods_rootrick_player_free(BfmRootrickFinalePlayer *p)
{
    if (p) bfm_woods_caster_melee_free(p->melee);
    free(p);
}

int bfm_woods_rootrick_player_begin(BfmRootrickFinalePlayer *p, unsigned outcome)
{
    if (!p || !outcome || outcome > 3 || p->outcome) return 0;
    p->outcome = outcome;
    return 1;
}

int bfm_woods_rootrick_player_align_x(BfmRootrickFinalePlayer *p, int32_t x_q16)
{
    if (!p || p->outcome != 2) return 0;
    int32_t position[3];
    if (!bfm_village_motion_position_q16(p->motion,position)) return 0;
    position[0] = x_q16;
    return bfm_village_motion_staged_position(p->motion,position);
}

int bfm_woods_rootrick_player_queue(BfmRootrickFinalePlayer *p,
    const BfmRootrickCommand *command, const BfmForestTransform *rival)
{
    if (!p || !p->outcome || !command) return -1;
    const BfmBedroomRoot *hero = &bfm_village_motion_player(p->motion)->root;
    uint32_t value = command->operand;
    Request r = {.x=(int16_t)value,.z=(int16_t)(value>>16),.value=value};
    switch (command->event) {
    case BFM_ROOTRICK_WALK_PLAYER: r.order = WALK; break;
    case BFM_ROOTRICK_WALK_FROM_PLAYER:
        r.order = WALK;
        r.x = (int16_t)(r.x+hero->xyz[0]);
        r.z = (int16_t)(r.z+hero->xyz[2]);
        break;
    case BFM_ROOTRICK_WALK_FROM_ACTOR: {
        int16_t local[3] = {(int16_t)r.x,0,(int16_t)r.z}, world[3];
        if (!rival) return -1;
        bfm_forest_lift_world(rival,local,world);
        r.order = WALK;
        r.x = world[0]; r.z = world[2];
        break;
    }
    case BFM_ROOTRICK_FACE_POINT: r.order = FACE; break;
    case BFM_ROOTRICK_FACE_ACTOR:
        if (!rival) return -1;
        r.order = FACE;
        r.x = (int16_t)rival->translation[0]; r.z = (int16_t)rival->translation[2];
        break;
    case BFM_ROOTRICK_PLAYER_YAW: r.order = TURN; break;
    case BFM_ROOTRICK_PLAYER_RELATIVE_YAW: r.order = RELATIVE_TURN; break;
    case BFM_ROOTRICK_PLAYER_CONTROL_BITS: r.order = CONTROL; break;
    case BFM_ROOTRICK_PLAYER_TARGET:
        if (value != (1u<<16 | 186u)) return -1;
        r.order = LIFT;
        break;
    case BFM_ROOTRICK_PLAYER_ATTACK:
        if (value > 1) return -1;
        r.order = ATTACK;
        break;
    default: return 0;
    }
    if (p->count == ORDERS) return -1;
    p->orders[p->count++] = r;
    return 1;
}

static void complete(BfmRootrickFinalePlayer *p)
{
    --p->count;
    memmove(p->orders,p->orders+1,p->count*sizeof p->orders[0]);
    p->entered = 0;
}

/* The turned model and the walk stick both use local -Z as forward. */
static int turn(BfmBedroomRoot *r, unsigned goal)
{
    int delta = (int)(goal & 4095) - (int)r->yaw;
    if (delta > 2048) delta -= 4096;
    if (delta < -2048) delta += 4096;
    int step = delta/4;
    if (!step) step = delta/2;
    if (!step) step = delta;
    r->yaw = (unsigned)((int)r->yaw+step) & 4095;
    return !delta;
}

static int footing(BfmRootrickFinalePlayer *p, BfmBedroomRoot *r, int16_t to[3])
{
    BfmForestContact contact;
    if (bfm_village_ground_step(bfm_village_motion_ground(p->motion),r->xyz,to,&contact)
        != BFM_VILLAGE_GROUND_FOOTING) return 0;
    memcpy(r->xyz,to,sizeof r->xyz);
    memcpy(r->normal,contact.normal,sizeof r->normal);
    return 1;
}

static int stride(BfmRootrickFinalePlayer *p, BfmBedroomRoot *r, int32_t position[3],
                  int32_t speed, unsigned yaw)
{
    BfmForestTransform rotation;
    int16_t to[3];
    memcpy(to,r->xyz,sizeof to);
    bfm_forest_actor_root(yaw,r->xyz,&rotation);
    for (unsigned i = 0; i < 2; ++i) {
        unsigned axis = 2*i;
        int64_t v = (int64_t)speed * -rotation.rotation[axis][2];
        int64_t q = position[axis] + v/4096 - (v%4096 < 0);
        int64_t whole = q/65536 - (q%65536 < 0);
        if (whole < INT16_MIN || whole > INT16_MAX) return 0;
        to[axis] = (int16_t)whole;
        position[axis] = (int32_t)q;
    }
    return footing(p,r,to);
}

int bfm_woods_rootrick_player_step(BfmRootrickFinalePlayer *p, BfmGameplayRun *run,
    int camera_yaw, uint32_t actor_flags, int winning_rock_present, BfmRootrickPlayerFrame *frame)
{
    if (!p || !p->outcome || !run || !frame) return 0;
    *frame = (BfmRootrickPlayerFrame){.granted_control_bits=p->granted_control_bits};
    BfmBedroomRoot r = bfm_village_motion_player(p->motion)->root;
    int32_t position[3];
    if (!bfm_village_motion_position_q16(p->motion,position)) return 0;
    r.locomotion_edge = BFM_TRAVEL_NONE;
    Request *cmd = p->count ? p->orders : NULL;
    int done = 0, directed = 0, pickup = 0, square = 0, triangle = 0;
    BfmWaypointInput stick = {0};
    if (cmd) {
        switch (cmd->order) {
        case WALK: {
            int dx = cmd->x-r.xyz[0], dz = cmd->z-r.xyz[2];
            if ((int64_t)dx*dx+(int64_t)dz*dz < 16*16) {
                int16_t to[3] = {(int16_t)cmd->x,r.xyz[1],(int16_t)cmd->z};
                if (!footing(p,&r,to)) return 0;
                done = 1;
            } else {
                stick = bfm_waypoint_input(dx,dz,0,camera_yaw);
                directed = 1;
            }
            break;
        }
        case RELATIVE_TURN:
            cmd->value += r.yaw;
            cmd->order = TURN;
            /* fall through */
        case TURN: done = turn(&r,cmd->value); break;
        case FACE: done = turn(&r,(unsigned)bfm_waypoint_heading(r.xyz[0]-cmd->x,
                                                                  r.xyz[2]-cmd->z)); break;
        case CONTROL:
            frame->control_bits = cmd->value & ~p->granted_control_bits;
            p->granted_control_bits |= cmd->value;
            frame->granted_control_bits = p->granted_control_bits;
            done = 1;
            break;
        case LIFT:
            if (!winning_rock_present || bfm_woods_carry_active(&p->carry)) return 0;
            /* The script has already resolved a live target, bypassing brace/reach. */
            p->carry.braced = 1;
            if (!bfm_woods_carry_begin(&p->carry,run,&frame->carry)) return 0;
            pickup = done = 1;
            break;
        case ATTACK:
            if (!p->entered) {
                triangle = cmd->value != 0;
                square = !triangle;
            }
            break;
        }
    }
    if (p->outcome == 1) {
        if (done) complete(p);
        return 1;
    }
    int carrying = bfm_woods_carry_active(&p->carry);
    int moving = directed;
    BfmBedroomAction action = BFM_BEDROOM_NONE;
    uint32_t pose = 0;
    unsigned face = 0;
    int fusion = 0, lumina = 0;
    if (carrying) {
        BfmForestTransform previous;
        if (!bfm_bedroom_figure_actor_root(&bfm_village_motion_player(p->motion)->root,0,&previous))
            return 0;
        if (!pickup && !bfm_woods_carry_step(&p->carry,run,&previous,directed,square,triangle,
                                             bfm_gameplay_story_flag(run,128),&frame->carry)) return 0;
        const BfmWoodsCarryFrame *held = &frame->carry;
        moving = held->speed_q16 != 0;
        if (held->turn) r.yaw = (unsigned)stick.yaw;
        if (moving && !stride(p,&r,position,held->speed_q16,r.yaw)) return 0;
        if (held->attached) {
            memcpy(p->socket,held->socket,sizeof p->socket);
            if (p->carry.lift.clip == BFM_LIFT_IDLE || p->carry.lift.clip == BFM_LIFT_WALK) {
                p->socket_yaw = (bfm_village_motion_player(p->motion)->root.yaw+2048)&4095;
                frame->socket_turn = 1;
            }
        }
        if (held->event == BFM_CARRY_THROW) {
            unsigned rank = u32le(run->player.record+0x6c);
            const BfmWoodsChargedWeapon *weapon = bfm_woods_caster_melee_weapon(p->melee);
            if (!weapon || rank >= 30) return 0;
            frame->carry.interaction_power = 75u*weapon->ranks[rank].body/100;
            memcpy(frame->carry.socket,p->socket,sizeof p->socket);
            uint16_t material = 0;
            bfm_village_ground_support_probe(bfm_village_motion_ground(p->motion),1,
                                             r.xyz,frame->carry.socket,&material);
        }
        frame->socket_yaw = p->socket_yaw;
        pose = held->pose; face = held->face_slot;
        if (held->finished) {
            action = BFM_BEDROOM_REST;
            if (cmd && cmd->order == ATTACK) done = 1;
        }
    } else if (cmd && cmd->order == ATTACK) {
        BfmWoodsCasterMeleeInput input = {.permitted=1,.actor_flags=actor_flags,
                                          .square_edge=square,.triangle_edge=triangle};
        if (!bfm_woods_caster_melee_step(p->melee,run,&r,&input,&frame->blade)) return 0;
        done = p->entered && !frame->blade.action;
        pose = frame->blade.pose; face = frame->blade.face_slot;
        fusion = frame->blade.show_fusion; lumina = frame->blade.show_lumina;
        if (done) action = BFM_BEDROOM_REST;
    } else if (directed) {
        r.yaw = (unsigned)stick.yaw;
        if (!stride(p,&r,position,stick.speed_q16,r.yaw)) return 0;
    }
    if (moving != (int)p->walking) {
        r.locomotion_edge = moving ? BFM_TRAVEL_WALK : BFM_TRAVEL_STAND;
        p->walking = (unsigned)moving;
    }
    if (pose) {
        if (!bfm_village_motion_staged_model(p->motion,&r,pose,face,fusion,lumina)) return 0;
    } else if (!bfm_village_motion_staged_action(p->motion,&r,action)) return 0;
    for (unsigned i = 0; i < 3; ++i)
        position[i] = (int32_t)r.xyz[i]*65536 + (uint16_t)position[i];
    if (!bfm_village_motion_staged_position(p->motion,position)) return 0;
    frame->ordinary_standing = !directed && !carrying && !frame->blade.action &&
                               bfm_village_motion_grounded(p->motion);
    if (cmd) {
        p->entered = 1;
        if (done) complete(p);
    }
    return 1;
}

int bfm_woods_rootrick_player_rival_hit(const BfmRootrickFinalePlayer *p,
    const BfmWoodsCasterMeleeFrame *blade, const BfmForestGuardBox *body,
    const BfmForestTransform *rival, int16_t hit[3], int16_t normal[3])
{
    if (!p || !blade || !body || !rival || blade->rays > BFM_FOREST_STRIKE_RAYS) return 0;
    for (unsigned i = 0; i < blade->rays; ++i) {
        int16_t end[3], terrain_normal[3], center[3];
        uint16_t material;
        memcpy(end,blade->to[i],sizeof end);
        bfm_village_ground_blade(bfm_village_motion_ground(p->motion),blade->from,end,
                                 terrain_normal,&material);
        if (bfm_forest_guard_contact_point(body,rival,blade->from,end,center,hit,normal)) return 1;
    }
    return 0;
}

int bfm_woods_rootrick_player_finish(BfmRootrickFinalePlayer *p, int defeating)
{
    if (!p || !p->outcome || p->count || bfm_woods_carry_active(&p->carry)) return 0;
    if (!defeating) {
        int32_t position[3];
        if (!bfm_village_motion_position_q16(p->motion,position) ||
            !bfm_village_motion_place(p->motion,&bfm_village_motion_player(p->motion)->root) ||
            !bfm_village_motion_staged_position(p->motion,position)) return 0;
    }
    p->outcome = p->entered = p->walking = p->granted_control_bits = 0;
    return 1;
}
