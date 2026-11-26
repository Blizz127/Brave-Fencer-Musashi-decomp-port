/* SPDX-License-Identifier: MIT */
#include "forest_walk.h"
#include "forest_opening_data.h"
#include "forest_motion.h"
#include "forest_manual.h"
#include "forest_swing.h"
#include "forest_fusion_power.h"
#include "forest_hud.h"
#include "forest_message.h"
#include "forest_message_art.h"
#include "forest_speech_actor.h"
#include "forest_voice.h"
#include "forest_action.h"
#include "../pc_port/forest_player_archive.h"
#include "../pc_port/forest_player_bytes.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

enum { IDLE = 4, WALK = 7, RUN = 8, LAST_WALK_TICK = 31 };
static const double radians = 6.28318530717958647693 / 4096;

typedef struct {
    unsigned count;
    uint32_t frames[32];
    uint32_t visible;
} Motion;

struct BfmForestWalk {
    BfmForestField *field;
    BfmGameplayRun standalone_gameplay;
    BfmGameplayRun *gameplay;
    const MusashiDiscMedia *disc;
    ForestPlayerState actor;
    BfmForestOpeningData data;
    BfmForestCamera camera;
    BfmForestTransform root;
    BfmForestPose pose;
    int8_t blade_anchors[2][3];
    Motion idle, walk, run;
    BfmForestJumpPoses jump_poses;
    BfmForestJumpPoses stride_poses;
    int stride_x, stride_y, stride_jump;
    BfmForestJump jump;
    BfmForestSpeechActor speaker;
    BfmForestManual manual;
    BfmForestStatus status;
    BfmForestHud *hud;
    int player_control, input_x, input_y;
    int jump_pressed, jump_held, control_polled;
    int attack_pressed, swing_sound, hit_sound, swing_connected;
    unsigned swing_age;
    BfmForestSwing swing;
    BfmForestFusionPower fusion_power;
    int at_boundary, blocked_x, blocked_y;
    BfmForestMessage *message;
    BfmForestMessageArt *message_art;
    ForestVoice *voice;
    unsigned voice_generation;
    int32_t eye[3], target[3], follow_offset[3];
    int look_x, look_z;
    int support_y, support_z;
    int16_t entrance_yaw, yaw, cached[3];
    int follow_projection;
    unsigned tick, elapsed, motion;
    int entry_world_tick;
    uint32_t frame, visible;
};

/* The two registered scene-event camera setups operate on the field thread.
 * Keep the transient callback record and its owner separate from the public
 * frame data, which is copied to each individual walk. */
s32 D_80126950, D_80126954, D_8012695C;
s16 D_80126968, D_8012696A, D_8012696C, D_80126976, D_80126978, D_8012697A;
u32 D_8012697C;
u16 D_80126980, D_80126B5E, D_80126B62, D_80126B66, D_80126940;
s16 D_80126944, D_80126942;
struct FacingActor { u8 bytes[0xa4]; } D_80126948;
extern void func_8017D900(void), func_8017D98C(void);
extern s32 func_801473EC(ForestActionPlayer *actor);

static int32_t floor_shift(int64_t n, unsigned shift)
{
    int64_t d = (int64_t)1 << shift;
    return (int32_t)(n/d - (n%d < 0));
}

static void normalize_axis(int out[3])
{
    unsigned mag=(unsigned)(out[0]*out[0]+out[1]*out[1]+out[2]*out[2]);
    unsigned shift=3;
    while (mag>255) { mag>>=2; ++shift; }
    int gain=(int)sqrt((double)((UINT32_C(1)<<30)/mag));
    for (unsigned i=0;i<3;i++) out[i]=floor_shift((int64_t)out[i]*gain,shift);
}

static int setup_cameras(BfmForestWalk *w)
{
    D_80126B5E=(u16)w->cached[0]; D_80126B62=(u16)w->cached[1]; D_80126B66=(u16)w->cached[2];
    func_8017D900();
    if (D_8012697C!=w->data.first_track || u32le(D_80126948.bytes)!=UINT32_C(0x80174f28)) return 0;
    for (unsigned a=0;a<3;++a) {
        w->eye[a]=w->data.key[0][a]; w->target[a]=w->data.key[1][a];
    }
    /* The initial long-range camera key is completely obscured by the
     * room-change fade; the next update activates the near field camera. */
    w->camera.projection=(uint16_t)D_80126950;
    func_8017D98C();
    if (u32le(D_80126948.bytes)!=UINT32_C(0x8017da08) || D_8012696C || !D_8012695C) return 0;
    w->follow_projection=D_80126954;
    int vx=(int)lround(sin(D_8012696A*radians)*4096), vz=(int)lround(cos(D_8012696A*radians)*4096);
    int axis[]={vx,0,vz}; normalize_axis(axis);
    int c=(int)lround(cos(D_80126968*radians)*4096);
    axis[0]=floor_shift((int64_t)axis[0]*c,12); axis[1]=-(int)lround(sin(D_80126968*radians)*4096);
    axis[2]=floor_shift((int64_t)axis[2]*c,12); normalize_axis(axis);
    for (unsigned a=0;a<3;++a) w->follow_offset[a]=floor_shift((int64_t)axis[a]*D_8012695C,12);
    return 1;
}

static int approach(int value, int desired, int divisor)
{
    int delta = desired - value;
    int step = delta / divisor;
    if (!step && delta) step = delta < 0 ? -1 : 1;
    return value + step;
}

/* The camera leads the current heading and keeps settling after a stop. */
static int follow(BfmForestWalk *w)
{
    int dz = (int)lround(cos(w->yaw * radians) * 4096);
    int dx = (int)lround(sin(w->yaw * radians) * 4096);
    w->look_x = approach(w->look_x, -dx / 64, 16);
    w->look_z = approach(w->look_z, -dz / 32, 16);
    w->target[0] = w->cached[0] + w->look_x;
    w->target[1] = w->cached[1];
    w->target[2] = w->cached[2] + w->look_z;
    for (unsigned i=0;i<3;i++) w->eye[i]=w->target[i]+w->follow_offset[i];
    int h=w->camera.projection, d=w->follow_projection-h;
    for (int div=4; d && div; div/=2) if (d/div) {h+=d/div;break;}
    return bfm_forest_camera_look_at(w->eye,w->target,(uint16_t)h,&w->camera);
}

static int word(const ForestPlayerArchive *archive,uint32_t position,uint32_t *out)
{
    uint8_t bytes[4];
    if ((position&3) || !forest_player_archive_read(archive,position,bytes,4)) return 0;
    *out=u32le(bytes);return 1;
}

static int load_motion(const ForestPlayerArchive *a, uint32_t table,unsigned id, Motion *out)
{
    uint32_t pos,list,cmd;
    if(table>UINT32_MAX-4*id || !word(a,table+4*id,&pos) || pos>UINT32_MAX-1024 ||
       !word(a,pos,&cmd) || cmd!=4 || !word(a,pos+4,&list) || list>UINT32_MAX-128 ||
       !word(a,pos+8,&cmd) || cmd!=19) return 0;
    uint32_t visible=0;
    for(unsigned i=0;i<MUSASHI_FOREST_PLAYER_PARTS;i++) {
        if(!word(a,list+i*4,&cmd) || (cmd>>24)!=0x80) return 0;
        visible|=UINT32_C(1)<<i;
    }
    if(!word(a,list+4*MUSASHI_FOREST_PLAYER_PARTS,&cmd) || cmd) return 0;
    /* Musashi enters with Fusion, the alternate blade and two Lumina parts
     * hidden. These are part indices in the mounted player mesh. */
    out->visible=visible&~((UINT32_C(1)<<21)|(UINT32_C(1)<<24)|(UINT32_C(1)<<25));
    for(unsigned i=3;i<256;i++) {
        if(!word(a,pos+i*4,&cmd)) return 0;
        if(cmd<=1) return out->count>0 && cmd==(id!=IDLE);
        if(cmd==8) continue;  /* footfall event, independent of the pose */
        if((cmd&3) || cmd<32 || cmd>UINT32_MAX-16 || out->count>=32) return 0;
        out->frames[out->count++]=cmd+16;
    }
    return 0;
}

/* The doorway and its first uphill face share a horizontal X axis. Ease
 * their mounted normal in world space, independently of the actor's facing. */
static void settle_ground(BfmForestWalk *w, const int16_t normal[3])
{
    if (!normal[1]) return;
    if (!w->support_y) w->support_y = normal[1];
    w->support_y = approach(w->support_y, normal[1], 8);
    w->support_z = approach(w->support_z, normal[2], 8);
}

static void update_root(BfmForestWalk *w)
{
    int s = (int)lround(sin(w->yaw * radians) * 4096);
    int c = (int)lround(cos(w->yaw * radians) * 4096);
    w->root = (BfmForestTransform){.rotation = {{c,0,s},{0,4096,0},{-s,0,c}},
        .translation = {w->cached[0], w->cached[1], w->cached[2]}};
    if (!w->support_z) return;

    /* The inclination samples the same 10-bit angular intervals as the
     * bedroom walking direction. Quantize the support basis before applying
     * the player's yaw, so the two signed sine coefficients round separately. */
    int ratio = abs(w->support_z) * 1024 / -w->support_y;
    int pitch = (int)lround(atan((ratio + 0.375) / 1024.0) / radians);
    if (w->support_z > 0) pitch = -pitch;
    int sp = (int)lround(sin(pitch * radians) * 4096);
    int cp = (int)lround(cos(pitch * radians) * 4096);
    int up[3] = {0, cp, sp}, forward[3] = {0, -sp, cp};
    normalize_axis(up);
    normalize_axis(forward);
    for (unsigned axis = 1; axis < 3; ++axis) {
        w->root.rotation[axis][0] = (int16_t)floor_shift(-s * forward[axis], 12);
        w->root.rotation[axis][1] = (int16_t)up[axis];
        w->root.rotation[axis][2] = (int16_t)floor_shift(c * forward[axis], 12);
    }
}

static int select_pose(BfmForestWalk *w, uint32_t frame, uint32_t visible)
{
    if (frame != w->frame) {
        ForestPlayerArchive archive;
        if (frame < 16 || !bfm_forest_pose_load(w->disc, frame, &w->pose) ||
            !forest_player_archive_open(w->disc, &archive) ||
            !forest_player_archive_read(&archive, frame - 16, (uint8_t *)w->blade_anchors,
                                        sizeof(w->blade_anchors))) return 0;
    }
    w->frame=frame; w->visible=visible;
    update_root(w);
    return 1;
}

static int update_pose(BfmForestWalk *w)
{
    const Motion *m=w->motion==IDLE?&w->idle:(w->motion==WALK?&w->walk:&w->run);
    return select_pose(w, m->frames[w->elapsed % m->count], m->visible);
}

BfmForestWalk *bfm_forest_walk_open(const MusashiDiscMedia *disc,const RoomTransferDescriptor *entry)
{
    if (!entry || entry->parameter!=0) return NULL;
    BfmForestWalk *w=calloc(1,sizeof *w); if(!w) return NULL;
    w->gameplay = &w->standalone_gameplay;
    w->entry_world_tick = -1;
    w->disc=disc; w->field=bfm_forest_enter(disc,entry,&w->actor,&w->yaw,w->cached);
    w->entrance_yaw=w->yaw;
    w->look_x = -(int)lround(sin(w->yaw * radians) * 4096) / 64;
    w->look_z = -(int)lround(cos(w->yaw * radians) * 4096) / 32;
    ForestPlayerArchive archive;
    w->hud = bfm_forest_open_hud(w->field, disc);
    if (!w->hud || !bfm_forest_status_load(disc, &w->status) || !bfm_forest_opening_data(w->field,&w->data) ||
        !forest_player_archive_open(disc,&archive) || !setup_cameras(w) ||
        !load_motion(&archive,w->data.animation_table,IDLE,&w->idle) ||
        !load_motion(&archive,w->data.animation_table,WALK,&w->walk) ||
        !load_motion(&archive,w->data.animation_table,RUN,&w->run)) goto fail;
    w->motion=IDLE; w->elapsed=1;
    if(update_pose(w)) return w;
fail:
    bfm_forest_walk_free(w); return NULL;
}

void bfm_forest_walk_bind_gameplay(BfmForestWalk *w, BfmGameplayRun *run)
{
    if (w) w->gameplay = run ? run : &w->standalone_gameplay;
}

void bfm_forest_walk_inherit_clock(BfmForestWalk *w, uint16_t first_tick)
{
    if (w && !w->tick)
        w->entry_world_tick = first_tick;
}

void bfm_forest_walk_free(BfmForestWalk *w)
{
    if (w) {
        bfm_forest_hud_free(w->hud);
        bfm_forest_message_free(w->message);
        bfm_forest_message_art_free(w->message_art);
        bfm_forest_voice_free(w->voice);
        bfm_forest_field_free(w->field);
        free(w);
    }
}

static int begin_message(BfmForestWalk *w)
{
    enum { DOOR_MESSAGE_BYTES = 160 };
    const uint8_t *program = bfm_forest_room_bytes(w->field, w->data.message, DOOR_MESSAGE_BYTES);
    if (!program) return 0;
    w->message = bfm_forest_message_open(program, DOOR_MESSAGE_BYTES);
    w->message_art = bfm_forest_open_message_art(w->field, w->disc);
    w->voice = bfm_forest_voice_load(w->disc);
    if (!w->message || !w->message_art || !w->voice ||
        !bfm_forest_speech_actor_begin(&w->speaker, w->field, w->disc, w->visible, &w->pose))
        return 0;
    memset(w->actor.velocity, 0, sizeof w->actor.velocity);
    w->actor.field_2c.word = 0;
    return 1;
}

static int tick_message(BfmForestWalk *w, int confirm_edge)
{
    if (!w->message) {
        if (!begin_message(w)) return -1;
    } else {
        const BfmForestMessageFrame *prior = bfm_forest_message_frame(w->message);
        bfm_forest_voice_step(w->voice);
        if (!prior || !bfm_forest_speech_actor_tick(&w->speaker,
                prior->events & BFM_FOREST_FIRST_GESTURE,
                prior->events & BFM_FOREST_SECOND_GESTURE,
                prior->events & BFM_FOREST_MESSAGE_CLOSED, &w->pose))
            return -1;
        if (w->speaker.phase != BFM_FOREST_TALK_RELEASED) {
            const BfmForestMessageFrame *next = bfm_forest_message_tick(
                w->message, bfm_forest_voice_status(w->voice), confirm_edge);
            if (!next) return -1;
            if (next->events & BFM_FOREST_VOICE_REQUEST) {
                if (!bfm_forest_voice_request(w->voice, next->voice_bank, next->voice_cue))
                    return -1;
                ++w->voice_generation;
            }
        }
    }
    if (w->speaker.phase == BFM_FOREST_TALK_RELEASED) {
        w->actor.state = 0;
        w->actor.phase = 1;
        w->player_control = 1;
        bfm_forest_hud_show(w->hud);
    }
    w->frame = w->speaker.frame;
    w->visible = w->speaker.visible;
    memcpy(w->blade_anchors, w->speaker.blade_anchors, sizeof w->blade_anchors);
    ++w->tick;
    return 1;
}

/* Normal controller axes are in screen space. The mounted room handles
 * velocity and contact in world space. */
static int tick_ground(BfmForestWalk *w, int x, int y, int enter_stance,
                       BfmForestDustEmission *dust)
{
    if (w->at_boundary && (x != w->blocked_x || y != w->blocked_y))
        w->at_boundary = 0;
    int previous_x = w->cached[0], previous_z = w->cached[2];
    BfmForestManualResult result = bfm_forest_manual_tick(&w->manual, w->field, &w->actor,
        &w->camera, w->at_boundary ? 0 : x, w->at_boundary ? 0 : y, &w->yaw, w->cached);
    if (result == BFM_FOREST_MANUAL_FALL) return 0;
    if (result == BFM_FOREST_MANUAL_BOUNDARY) {
        w->at_boundary = 1;
        w->blocked_x = x;
        w->blocked_y = y;
        result = bfm_forest_manual_tick(&w->manual, w->field, &w->actor, &w->camera,
                                        0, 0, &w->yaw, w->cached);
    }
    if (result <= BFM_FOREST_MANUAL_BOUNDARY)
        return -1;
    /* A submitted direction becomes a stride only after a ground step. */
    int advanced = result == BFM_FOREST_MANUAL_MOVING &&
                   (w->cached[0] != previous_x || w->cached[2] != previous_z);
    w->stride_x = advanced ? x : 0;
    w->stride_y = advanced ? y : 0;
    settle_ground(w, w->manual.ground_normal);
    /* The active world counter advances after the actors run. The initial
     * forest pose already completed its first update at local tick zero. */
    unsigned world_tick = (unsigned)w->entry_world_tick + w->tick;
    if (result == BFM_FOREST_MANUAL_STARTED || result == BFM_FOREST_MANUAL_MOVING) {
        if (result == BFM_FOREST_MANUAL_STARTED ||
            (w->entry_world_tick >= 0 && w->actor.field_2c.word == -11 * 65536 &&
             world_tick % 8 == 0))
            *dust = BFM_FOREST_DUST_TRAIL;
        w->motion = w->manual.motion;
        w->elapsed = w->manual.elapsed;
        if (!update_pose(w)) return -1;
    } else {
        BfmForestStandingEvent event = enter_stance ?
            BFM_FOREST_STANDING_STOP : BFM_FOREST_STANDING_CONTINUE;
        if (result == BFM_FOREST_MANUAL_STOPPED || result == BFM_FOREST_MANUAL_RUN_STOPPED)
            event = result == BFM_FOREST_MANUAL_RUN_STOPPED && !w->at_boundary ?
                    BFM_FOREST_STANDING_RUN_STOP : BFM_FOREST_STANDING_STOP;
        if (!bfm_forest_standing_tick(&w->speaker, event, &w->actor, &w->pose))
            return -1;
        w->actor.field_2c.word = bfm_forest_standing_forward_q16(&w->speaker);
        if (w->actor.field_2c.word && event == BFM_FOREST_STANDING_CONTINUE &&
            !w->at_boundary && !bfm_forest_manual_coast(w->field, &w->actor, w->yaw, w->cached)) {
            w->at_boundary = 1;
            w->blocked_x = x;
            w->blocked_y = y;
        }
        if (!w->at_boundary && !enter_stance && !x && !y &&
            (result == BFM_FOREST_MANUAL_STOPPED || result == BFM_FOREST_MANUAL_RUN_STOPPED ||
             (w->entry_world_tick >= 0 && w->actor.field_2c.word != 0 && world_tick % 4 == 0)))
            *dust = BFM_FOREST_DUST_BRAKE;
        update_root(w);
        w->frame = w->speaker.frame;
        w->visible = w->speaker.visible;
        memcpy(w->blade_anchors, w->speaker.blade_anchors, sizeof w->blade_anchors);
    }
    return 1;
}

static int select_jump_pose(BfmForestWalk *w)
{
    const BfmForestJumpPoses *poses = w->jump.walk_off || w->stride_jump ?
                                      &w->stride_poses : &w->jump_poses;
    uint32_t frame;
    if (w->jump.walk_off && !w->jump.landed) {
        if (!w->jump.elapsed) return 0;
        frame = bfm_forest_walkoff_pose_frame(poses, w->jump.elapsed - 1);
    } else {
        enum BfmForestJumpPhase phase = BFM_FOREST_JUMP_RISING;
        if (w->jump.landed) phase = BFM_FOREST_JUMP_LANDING;
        else if (w->jump.descending) phase = BFM_FOREST_JUMP_FALLING;
        frame = bfm_forest_jump_pose_frame(poses, phase, w->jump.phase_elapsed);
    }
    return frame && select_pose(w, frame, poses->visible);
}

/* The grounded attack stops movement. The end of the mounted strike window
 * allows a new direction to take over while the last keys are playing. */
static int tick_swing(BfmForestWalk *w, int x, int y, int attack_pressed,
                      BfmForestDustEmission *dust, BfmForestHit *hit)
{
    /* A new Square after four active updates can restart an ordinary slash
     * that missed. Connected combo follow-ups have their own actions. */
    int repeat_miss = attack_pressed && !w->swing_connected && w->actor.strike_updates >= 4 &&
                      w->swing_age + 1 < w->swing.count;
    int starting = w->actor.state != 5 || repeat_miss;
    if (starting) {
        if (!w->swing.count &&
            (!bfm_forest_fusion_power_load(w->disc, w->field, &w->fusion_power) ||
             !bfm_forest_swing_load(w->disc, &w->swing))) return 0;
        w->swing_age = 0;
        w->actor.field_1a8 = 0;
        w->actor.strike_updates = 0;
        w->actor.ordinary_strike_toggle ^= 1;
        w->actor.strike_index = w->actor.ordinary_strike_toggle ? 0 : 1;
        w->swing_connected = 0;
        w->swing_sound = 1;
        w->actor.state = 5;
        w->actor.phase = 1;
        w->manual.moving = w->manual.after_jump = 0;
        w->actor.field_24.word = w->actor.field_2c.word = w->actor.local_y.word = 0;
        memset(w->actor.velocity, 0, sizeof w->actor.velocity);
    } else {
        ++w->swing_age;
    }
    const BfmForestSwingFrame *key = bfm_forest_swing_frame(&w->swing, w->swing_age);
    w->actor.field_1a8 = key && key->window;
    if (!key || ((x || y) && !w->actor.field_1a8 && w->actor.strike_updates)) {
        w->actor.state = 0;
        return tick_ground(w, x, y, 1, dust) > 0;
    }
    /* Initializing the action installs its motion and clears contact state.
     * Strength belongs to the subsequent active updates. */
    if (starting) return 1;
    /* The grounded action rolls strength before testing the blade. A miss
     * or a recovering target still advances the shared gameplay stream. */
    uint16_t strength;
    if (!bfm_forest_fusion_strength(&w->fusion_power, &w->status, w->actor.strike_index,
                                    bfm_gameplay_story_flag(w->gameplay, 0x81),
                                    bfm_gameplay_random(w->gameplay), &strength)) return 0;
    if (w->actor.field_1a8) ++w->actor.strike_updates;
    if (w->actor.field_1a8 && !w->swing_connected) {
        const BfmForestSwingFrame *next = bfm_forest_swing_frame(&w->swing, w->swing_age + 1);
        if (!next) return 0;
        hit->connected = bfm_forest_ground_strike(w->field, &w->root, key->tip, next->tip,
                                                  strength, &w->yaw, hit->position, hit->normal);
        if (hit->connected < 0) return 0;
        w->swing_connected = hit->connected;
    }
    /* The field display still shows the previous motion during the first two
     * updates after the Square edge. */
    if (w->swing_age < 2) return 1;
    const BfmForestSwingFrame *visible = bfm_forest_swing_frame(&w->swing, w->swing_age - 2);
    return visible && select_pose(w, visible->pose, w->swing.visible);
}

static int tick_player(BfmForestWalk *w, int x, int y, int jump_pressed, int jump_held,
                       int attack_pressed)
{
    BfmForestDustEmission dust = BFM_FOREST_DUST_NONE;
    BfmForestHit hit = {0};
    BfmForestTransform previous_root = w->root;
    uint32_t sweep_pose = 0;
    int continuing_stride = (x || y) && x == w->stride_x && y == w->stride_y;
    w->stride_x = w->stride_y = 0;
    bfm_forest_hud_tick(w->hud);
    bfm_forest_fall_begin(&w->jump, &w->manual, &w->actor);
    int swinging = w->actor.state == 5 ||
                   (!w->jump.active && !jump_pressed && attack_pressed && w->actor.state <= 1);
    int launched = !swinging && !w->jump.active && jump_pressed &&
                   bfm_forest_jump_begin(&w->jump, w->field, &w->actor, w->cached);
    if (!w->jump.active && !swinging) {
        int grounded = tick_ground(w, x, y, 0, &dust);
        if (grounded < 0) return -1;
        if (!grounded && !bfm_forest_fall_begin(&w->jump, &w->manual, &w->actor)) return -1;
    }
    if (w->jump.active && w->jump.walk_off && !w->stride_poses.visible &&
        !bfm_forest_walkoff_poses_load(w->disc, w->data.animation_table, &w->stride_poses))
        return -1;
    if (swinging) {
        if (!tick_swing(w, x, y, attack_pressed, &dust, &hit)) return -1;
        if (w->actor.state == 5) sweep_pose = w->swing.frames[w->swing_age].pose;
    } else if (launched) {
        w->stride_jump = continuing_stride;
        if (w->stride_jump) {
            if (!w->stride_poses.visible &&
                !bfm_forest_walkoff_poses_load(w->disc, w->data.animation_table, &w->stride_poses))
                return -1;
        } else if (!w->jump_poses.visible &&
                   !bfm_forest_jump_poses_load(w->disc, w->data.animation_table, &w->jump_poses)) {
            return -1;
        }
        w->at_boundary = 0;
        dust = BFM_FOREST_DUST_TRAIL;
        if (!select_jump_pose(w)) return -1;
    } else if (w->jump.active) {
        int previously_landed = w->jump.landed;
        BfmForestJumpResult result = bfm_forest_jump_tick(&w->jump, w->field,
            &w->actor, &w->camera, x, y, jump_held, &w->yaw, w->cached);
        w->at_boundary = result == BFM_FOREST_JUMP_BOUNDARY;
        if (w->at_boundary) {
            w->blocked_x = x;
            w->blocked_y = y;
        } else {
            if (result == BFM_FOREST_JUMP_LANDED && !previously_landed && !w->jump.walk_off)
                dust = BFM_FOREST_DUST_BRAKE;
            if (result == BFM_FOREST_JUMP_FINISHED && !w->jump.walk_off && (x || y))
                dust = BFM_FOREST_DUST_TRAIL;
            if (result == BFM_FOREST_JUMP_FINISHED)
                bfm_forest_jump_resume(&w->manual, &w->jump, &w->actor);
            if (result == BFM_FOREST_JUMP_FINISHED && !w->manual.moving) {
                if (tick_ground(w, 0, 0, 1, &dust) < 0) return -1;
            } else if (result == BFM_FOREST_JUMP_FINISHED && w->stride_jump && !w->jump.walk_off) {
                /* Ground control installs the walking entry pose. The next
                 * ground step selects the clip for its speed. */
                w->motion = WALK;
                w->elapsed = 0;
                if (!update_pose(w)) return -1;
            } else {
                if (!select_jump_pose(w)) return -1;
            }
        }
    }
    bfm_forest_voice_step(w->voice);
    ++w->tick;
    if (!bfm_forest_tick_sweep(w->field, w->disc, w->swing_sound, sweep_pose, &previous_root) ||
        !follow(w) || !bfm_forest_tick_shore_actor_with_run(w->field, w->cached, w->gameplay) ||
        !bfm_forest_submit_guard_and_impact(w->field, &hit, w->eye, &w->camera))
        return -1;
    w->hit_sound = hit.connected;
    bfm_forest_tick_dust(w->field, dust, w->cached, w->eye, w->yaw);
    return 1;
}

int bfm_forest_walk_hit_sound(const BfmForestWalk *w)
{
    return w && w->hit_sound;
}

int bfm_forest_walk_swing_sound(const BfmForestWalk *w)
{
    return w && w->swing_sound;
}

int bfm_forest_walk_control(const BfmForestWalk *w)
{
    if (!w || !w->player_control) return 0;
    return w->at_boundary ? 2 : 1;
}

ForestVoice *bfm_forest_walk_voice(BfmForestWalk *w, unsigned *generation)
{
    if (!w || !generation) return NULL;
    *generation = w->voice_generation;
    return w->voice;
}

int bfm_forest_walk_tick(BfmForestWalk *w)
{
    return bfm_forest_walk_tick_input(w, 0);
}

int bfm_forest_walk_tick_input(BfmForestWalk *w, int confirm_edge)
{
    return bfm_forest_walk_tick_axes(w, 0, 0, confirm_edge);
}

int bfm_forest_walk_tick_axes(BfmForestWalk *w, int x, int y, int confirm_edge)
{
    return bfm_forest_walk_tick_controls(w, x, y, confirm_edge, confirm_edge);
}

int bfm_forest_walk_tick_controls(BfmForestWalk *w, int x, int y,
                                  int cross_pressed, int cross_held)
{
    return bfm_forest_walk_tick_pad(w, x, y, cross_pressed ? BFM_FOREST_CROSS : 0,
                                   cross_held ? BFM_FOREST_CROSS : 0);
}

int bfm_forest_walk_tick_pad(BfmForestWalk *w, int x, int y,
                             uint16_t pressed, uint16_t held)
{
    int cross_pressed = (pressed & BFM_FOREST_CROSS) != 0;
    int cross_held = (held & BFM_FOREST_CROSS) != 0;
    if(!w || x < -1 || x > 1 || y < -1 || y > 1) return -1;
    /* The actor uses the previous completed controller poll. */
    int read_x = w->input_x, read_y = w->input_y;
    int jump_pressed = w->jump_pressed, jump_held = w->jump_held;
    w->input_x = x; w->input_y = y;
    /* The first poll returning control can carry the introduction's closing
     * Cross edge. Directions are live there, but a jump needs a fresh press. */
    w->jump_pressed = w->control_polled && cross_pressed;
    w->jump_held = cross_held;
    int attack_pressed = w->attack_pressed;
    w->attack_pressed = w->control_polled && (pressed & BFM_FOREST_SQUARE);
    w->swing_sound = 0;
    w->hit_sound = 0;
    w->control_polled = w->player_control;
    if (w->player_control) return tick_player(w, read_x, read_y, jump_pressed, jump_held, attack_pressed);
    if (w->tick >= LAST_WALK_TICK) return tick_message(w, cross_pressed);
    ++w->tick;
    if(w->tick==1) {++w->elapsed;}
    else if(w->tick==2) {w->elapsed=0; w->motion=WALK; w->actor.field_2c.word=w->data.initial_velocity;}
    else {
        /* The doorway steers toward the closest eighth-turn to its authored
         * facing in the current camera plane. No player input is consumed. */
        double bearing=atan2(w->target[0]-w->eye[0],w->target[2]-w->eye[2])/radians;
        int plane=(int)lround(bearing);
        w->yaw=(int16_t)((plane+512*(int)lround((w->entrance_yaw-plane)/512.0))&4095);
        int32_t speed=w->actor.field_2c.word-0x1000;
        if(speed< -0xb0000) speed=-0xb0000;
        w->actor.field_2c.word=speed;
        if(!bfm_forest_level_velocity(&w->actor,0,w->yaw,0)) return -1;
        func_801473EC(&w->actor);
        int16_t x=(int16_t)(w->actor.position[0]>>16),z=(int16_t)(w->actor.position[2]>>16);
        BfmForestContact hit;
        if(!bfm_forest_ground(w->field,x,w->cached[1],z,&hit) || hit.normal[0] || hit.normal[2]) return -1;
        w->cached[0]=x;w->cached[1]=hit.height;w->cached[2]=z;
        w->actor.position[1]=(uint32_t)(uint16_t)hit.height<<16;
        unsigned next=speed< -0x94000 ? RUN : WALK;
        w->elapsed=next==w->motion?w->elapsed+1:0;w->motion=next;
    }
    return follow(w) && update_pose(w) ? 1 : -1;
}

int bfm_forest_walk_draw(BfmForestWalk *w,uint32_t pixels[240][320])
{
    if(!w || !pixels) return 0;
    for(unsigned y=0;y<240;y++) for(unsigned x=0;x<320;x++) pixels[y][x]=UINT32_C(0xff000000);
    if(!w->tick) return 1;
    BfmForestBladeFrame blade = {.tick = w->tick};
    int has_blade = bfm_forest_blade_transform(w->blade_anchors, &w->root, w->eye, &blade.world);
    if (!bfm_forest_draw_entrance_player(w->field, &w->camera, &w->pose, &w->root,
                                         w->visible, w->tick % 2, has_blade ? &blade : NULL, pixels)) return 0;
    unsigned shade=w->tick<16?(255-w->tick*16)/8:0;
    for(unsigned y=0;y<240;y++) for(unsigned x=0;x<320;x++) {
        unsigned result=UINT32_C(0xff000000), rgb=pixels[y][x];
        for(unsigned shift=0;shift<=16;shift+=8){
            unsigned v=(rgb>>shift&255)>>3;
            v=v>shade?v-shade:0;
            result|=(v*255/31)<<shift;
        }
        pixels[y][x]=result;
    }
    if (w->player_control) bfm_forest_hud_draw(w->hud, &w->status, pixels);
    if (w->message && !w->player_control)
        bfm_forest_message_art_draw(w->message_art, bfm_forest_message_frame(w->message), pixels);
    return 1;
}
