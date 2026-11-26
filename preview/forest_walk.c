/* SPDX-License-Identifier: MIT */
#include "forest_walk.h"
#include "castle_drawbridge.h"
#include "forest_gateway_transition.h"
#include "tower_field.h"
#include "tower_ascent_door.h"
#include "tower_interior.h"
#include "tower_interior_note.h"
#include "tower_summit.h"
#include "tower_escape_camera.h"
#include "tower_summit_transfer.h"
#include "tower_summit_departure.h"
#include "forest_shore_motion.h"
#include "tower_ascent_floor.h"
#include "tower_note.h"
#include "tower_note_pose.h"
#include "forest_opening_data.h"
#include "forest_motion.h"
#include "forest_manual.h"
#include "forest_swing.h"
#include "forest_lift.h"
#include "forest_boulders.h"
#include "forest_hurt.h"
#include "forest_shore_figures.h"
#include "forest_fusion_power.h"
#include "forest_lesson.h"
#include "forest_lesson_fusion.h"
#include "forest_lesson_camera.h"
#include "forest_gunshot.h"
#include "forest_bullet.h"
#include "forest_bridge_retreat.h"
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

typedef struct { int edge, held, stick; } GunshotInput;

typedef struct {
    BfmForestPose pose;
    BfmForestCamera camera;
    BfmForestTransform root;
    uint32_t visible;
    unsigned tick;
    BfmForestBladeFrame blade;
    int has_blade;
} TowerSubmission;

struct BfmForestWalk {
    BfmForestField *field;
    BfmTowerField *tower;
    BfmTowerAscentDoor summit_door;
    BfmTowerSummitTransfer summit_transfer;
    BfmTowerSummit *summit;
    BfmTowerEscape escape;
    BfmTowerEscapeCamera escape_camera;
    BfmCastleDrawbridge *drawbridge;
    int summit_placed, summit_free;
    BfmTowerInterior *inner;
    const RoomTransferDescriptor *inner_exit;
    BfmTowerInteriorNote inner_note;
    unsigned inner_age, inner_shot;
    int inner_free, inner_entry_foot;
    BfmForestGatewayTransition gateway;
    int tower_placed;
    BfmForestMessage *tower_note;
    BfmTowerNoteKind tower_note_kind;
    BfmTowerNotePose tower_note_pose;
    unsigned tower_tick, submit_slot, tower_stroke;
    TowerSubmission submitted_tower[3];
    uint32_t (*held_forest)[320];
    unsigned last_forest_tick;
    BfmGameplayRun standalone_gameplay;
    BfmGameplayRun *gameplay;
    int ambient_status;
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
    BfmForestHud *hud;
    int player_control, input_x, input_y;
    int jump_pressed, jump_held, control_polled;
    int square_held;
    int attack_pressed, triangle_pressed, swing_sound, hit_sound, swing_connected;
    unsigned swing_age;
    BfmForestSwing swing;
    BfmForestHurt hurt;
    BfmForestHurtClip hurt_clip;
    uint16_t pending_damage;
    unsigned hurt_tail;
    int hide_recovering_body;
    BfmForestHurtDisplay hurt_display;
    BfmForestBodySample displayed_body;
    BfmForestPose displayed_pose;
    int8_t displayed_anchors[2][3];
    BfmForestLiftData lift_data;
    BfmForestLift lift;
    unsigned brace_age;
    unsigned push_body, push_clock;
    int braced;
    BfmForestFusionPower fusion_power;
    BfmForestLesson lesson;
    BfmForestLessonFusion fusion;
    BfmForestLessonCamera lens;
    BfmForestGunshot gun;
    BfmForestBullets bullets;
    BfmForestBridgeRetreat retreat;
    GunshotInput gun_input;
    unsigned bridge_phase;
    int gunshot_fired, retreat_control;
    int shoulder_held, orbit[3];
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
    memcpy(w->orbit, axis, sizeof w->orbit);
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

static int read_body(const BfmForestWalk *w, uint32_t frame, BfmForestPose *pose,
                     int8_t anchors[2][3])
{
    enum { ANCHOR_BYTES = 16, POSE_BYTES = 240 };
    if (frame < ANCHOR_BYTES || (frame & 3)) return 0;
    uint8_t resident[ANCHOR_BYTES + POSE_BYTES];
    uint32_t start = frame - ANCHOR_BYTES;
    const uint8_t *key = bfm_forest_gunshot_bytes(&w->fusion.award, start, sizeof resident);
    if (!key) {
        ForestPlayerArchive archive;
        if (!forest_player_archive_open(w->disc, &archive) ||
            !forest_player_archive_read(&archive, start, resident, sizeof resident)) return 0;
        key = resident;
    }
    if (!bfm_forest_pose_decode(key + ANCHOR_BYTES, POSE_BYTES, pose)) return 0;
    memcpy(anchors, key, sizeof w->blade_anchors);
    return 1;
}

static int select_pose(BfmForestWalk *w, uint32_t frame, uint32_t visible)
{
    if (frame != w->frame && !read_body(w, frame, &w->pose, w->blade_anchors)) return 0;
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
    if (!w->hud || !bfm_forest_status_load(disc, &w->gameplay->player) || !bfm_forest_opening_data(w->field,&w->data) ||
        !forest_player_archive_open(disc,&archive) || !setup_cameras(w) ||
        !load_motion(&archive,w->data.animation_table,IDLE,&w->idle) ||
        !load_motion(&archive,w->data.animation_table,WALK,&w->walk) ||
        !load_motion(&archive,w->data.animation_table,RUN,&w->run)) goto fail;
    w->motion=IDLE; w->elapsed=1;
    w->gameplay->has_player = 1;
    if(update_pose(w)) return w;
fail:
    bfm_forest_walk_free(w); return NULL;
}

static int start_ambient(BfmForestWalk *w)
{
    if (w->ambient_status) return w->ambient_status > 0;
    /* Room placement precedes the player's first idle pose by one field
     * update. The scenery advances while the player is taking that pose. */
    bfm_forest_start_ambient(w->field, w->gameplay);
    bfm_forest_submit_atmosphere(w->field, &w->camera, w->eye);
    w->ambient_status = -1;
    if (!bfm_forest_tick_shore_actor_with_run(w->field, w->cached, w->gameplay)) return 0;
    bfm_forest_submit_atmosphere(w->field, &w->camera, w->eye);
    w->ambient_status = 1;
    return 1;
}

int bfm_forest_walk_bind_gameplay(BfmForestWalk *w, BfmGameplayRun *run)
{
    if (!w) return 0;
    if (!run) run = &w->standalone_gameplay;
    if (!run->has_player || run == &w->standalone_gameplay) {
        run->player = w->gameplay->player;
        run->has_player = 1;
    }
    w->gameplay = run;
    return start_ambient(w);
}

void bfm_forest_walk_inherit_clock(BfmForestWalk *w, uint16_t first_tick)
{
    if (w && !w->tick)
        w->entry_world_tick = first_tick;
}

void bfm_forest_walk_free(BfmForestWalk *w)
{
    if (w) {
        bfm_castle_drawbridge_free(w->drawbridge);
        bfm_tower_summit_free(w->summit);
        bfm_tower_interior_note_close(&w->inner_note);
        bfm_tower_interior_free(w->inner);
        bfm_forest_message_free(w->tower_note);
        bfm_forest_message_free(w->lesson.page);
        bfm_forest_hud_free(w->hud);
        bfm_forest_message_free(w->message);
        bfm_forest_message_art_free(w->message_art);
        bfm_forest_voice_free(w->voice);
        free(w->held_forest);
        bfm_tower_field_free(w->tower);
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

/* The short body sweep uses the moving root. The following action update
 * uses the separate, forward-facing reach segment only from the brace clip. */
static int lift_target(BfmForestWalk *w, unsigned mask, const int16_t from[3],
                       const int16_t to[3], int16_t point[3])
{
    BfmForestShoreFigures *figures = bfm_forest_lift_figures(w->field);
    for (unsigned i = 0; i < BFM_FOREST_SHORE_FIGURES; ++i)
        if (bfm_forest_shore_figure_probe(figures, i, mask, from, to, point))
            return (int)i + 1;
    return 0;
}

static int brace_against_body(BfmForestWalk *w, const int16_t previous[3])
{
    int16_t from[3] = {previous[0], previous[1] - 16, previous[2]};
    int16_t to[3] = {w->cached[0], w->cached[1] - 16, w->cached[2]}, hit[3];
    int stone = bfm_forest_boulder_contact(bfm_forest_room_boulders(w->field), from, to, hit);
    int target = stone ? 0 : lift_target(w, 0x1800, from, to, hit);
    int passing_soldier = !target && !stone && (from[0] != to[0] || from[2] != to[2]) &&
                          bfm_forest_melee_brace(w->field, from);
    if (!target && !stone && !passing_soldier) {
        w->braced = 0;
        return 0;
    }
    if (!w->lift_data.motion[BFM_LIFT_BRACE].count &&
        !bfm_forest_lift_load(w->field, w->disc, w->data.animation_table, &w->lift_data)) return -1;
    const BfmLiftMotion *motion = &w->lift_data.motion[BFM_LIFT_BRACE];
    w->brace_age = w->braced ? (w->brace_age + 1) % motion->count : 0;
    w->braced = !stone;
    if (stone) {
        w->push_body = (unsigned)stone;
        w->push_clock = 10;
        w->actor.state = 9;
        w->actor.phase = 1;
    }
    if (passing_soldier) return select_pose(w, motion->frames[w->brace_age], w->walk.visible) ? 1 : -1;
    w->cached[0] = hit[0];
    w->cached[2] = hit[2];
    for (unsigned axis = 0; axis < 3; axis += 2)
        w->actor.position[axis] = ((uint32_t)(uint16_t)hit[axis] << 16) |
                                  (w->actor.position[axis] & 0xffffu);
    return select_pose(w, motion->frames[w->brace_age], w->walk.visible) ? 1 : -1;
}

static void restore_whole_position(BfmForestWalk *w, const int16_t at[3])
{
    for (unsigned axis = 0; axis < 3; ++axis) {
        w->cached[axis] = at[axis];
        w->actor.position[axis] = ((uint32_t)(uint16_t)at[axis] << 16) |
                                  (w->actor.position[axis] & 0xffffu);
    }
}

static int finish_boulder_push(BfmForestWalk *w)
{
    w->push_body = 0;
    w->actor.state = 0;
    w->actor.phase = 1;
    w->manual = (BfmForestManual){0};
    w->actor.field_2c.word = 0;
    memset(w->actor.velocity, 0, sizeof w->actor.velocity);
    w->motion = IDLE;
    w->elapsed = 0;
    return update_pose(w);
}

static int tick_boulder_push(BfmForestWalk *w, int x, int y, int square)
{
    const BfmLiftMotion *clip = &w->lift_data.motion[BFM_LIFT_BRACE];
    int16_t heading = w->yaw;
    int directed = (x || y) && bfm_forest_manual_heading(&w->camera, x, y, &heading);
    if (w->actor.phase == 1) {
        if (!directed && !square) return finish_boulder_push(w);
        memset(w->actor.velocity, 0, sizeof w->actor.velocity);
        if (!--w->push_clock) {
            if (directed) w->yaw = (int16_t)(((unsigned)heading + 250) & 0xff8u);
            w->push_clock = 20;
            w->actor.phase = 2;
        }
    } else {
        if (w->actor.phase == 3 && !directed && !square) return finish_boulder_push(w);
        if (w->actor.phase == 3 && directed)
            w->yaw = (int16_t)(((unsigned)heading + 250) & 0xff8u);
        int16_t before[3];
        memcpy(before, w->cached, sizeof before);
        int32_t speed = bfm_forest_room_boulders(w->field)->forward_speed;
        if (!bfm_forest_carry_step(w->field, &w->actor, w->yaw, speed, w->cached))
            return finish_boulder_push(w);
        int16_t from[3] = {before[0], (int16_t)(before[1] - 16), before[2]};
        int16_t to[3] = {w->cached[0], (int16_t)(w->cached[1] - 16), w->cached[2]};
        int result = bfm_forest_boulder_move(w->field, w->push_body - 1, from, to);
        if (result < 0) return finish_boulder_push(w);
        if (!result) restore_whole_position(w, before);
        if (w->actor.phase == 3) {
            w->push_clock = 20;
            w->actor.phase = 2;
        } else if (!--w->push_clock) {
            w->actor.phase = 3;
        }
    }
    w->brace_age = (w->brace_age + 1) % clip->count;
    return select_pose(w, clip->frames[w->brace_age], w->walk.visible);
}

static int start_lift(BfmForestWalk *w)
{
    const int16_t reach[3] = {0, -16, -32};
    int16_t end[3];
    bfm_forest_lift_world(&w->root, reach, end);
    int target = lift_target(w, 0x0200, w->cached, end, NULL);
    if (!target) return 0;
    BfmForestShoreFigures *figures = bfm_forest_lift_figures(w->field);
    BfmForestLift lift = {.clip = BFM_LIFT_RAISE, .target = target};
    if (!bfm_forest_shore_figure_grip_offset(figures, lift.grip) ||
        !bfm_forest_fusion_power_load(w->disc, w->field, &w->fusion_power)) return -1;
    unsigned power = w->lift_data.grab_base +
        bfm_gameplay_random(w->gameplay) % ((unsigned)w->lift_data.grab_spread + 1);
    if (!bfm_forest_shore_figure_grab(figures, target - 1, (uint16_t)power, w->yaw)) return 0;
    w->lift = lift;
    w->braced = 0;
    w->actor.state = 13;
    w->actor.phase = 1;
    memset(&w->manual, 0, sizeof w->manual);
    memset(w->actor.velocity, 0, sizeof w->actor.velocity);
    w->actor.field_24.word = w->actor.local_y.word = w->actor.field_2c.word = 0;
    return select_pose(w, w->lift_data.motion[BFM_LIFT_RAISE].frames[0], w->walk.visible) ? 1 : -1;
}

/* Static room faces have already advanced the standing player. Clip that
 * stride against any intact pylon at the same body height as a forest brace. */
static int slide_tower_stride(BfmForestWalk *w, const int16_t previous[3])
{
    int16_t from[3] = {previous[0], previous[1] - 16, previous[2]};
    int16_t to[3] = {w->cached[0], w->cached[1] - 16, w->cached[2]}, contact[3];
    if (!bfm_tower_field_body_slide(w->tower, from, to, contact)) return 1;
    BfmForestContact floor;
    if (!bfm_forest_ground(bfm_tower_field_room(w->tower), contact[0], w->cached[1], contact[2], &floor))
        return 0;
    contact[1] = floor.height;
    memcpy(w->cached, contact, sizeof contact);
    memcpy(w->manual.ground_normal, floor.normal, sizeof floor.normal);
    for (unsigned axis = 0; axis < 3; ++axis)
        w->actor.position[axis] = ((uint32_t)(uint16_t)contact[axis] << 16) |
                                  (w->actor.position[axis] & 0xffffu);
    return 1;
}

/* Normal controller axes are in screen space. The mounted room handles
 * velocity and contact in world space. */
static int tick_ground(BfmForestWalk *w, int x, int y, int enter_stance,
                       BfmForestDustEmission *dust)
{
    if (w->at_boundary && (x != w->blocked_x || y != w->blocked_y))
        w->at_boundary = 0;
    int16_t before[3];
    memcpy(before, w->cached, sizeof before);
    int previous_x = before[0], previous_z = before[2];
    BfmForestField *room = w->tower_placed ? bfm_tower_field_room(w->tower) : w->field;
    if (w->summit_free) room = bfm_tower_summit_room(w->summit);
    else if (w->inner_free) room = bfm_tower_interior_room(w->inner);
    BfmForestManualResult result = bfm_forest_manual_tick(&w->manual, room, &w->actor,
        &w->camera, w->at_boundary ? 0 : x, w->at_boundary ? 0 : y, &w->yaw, w->cached);
    if (result == BFM_FOREST_MANUAL_FALL) return 0;
    if (result == BFM_FOREST_MANUAL_BOUNDARY) {
        w->at_boundary = 1;
        w->blocked_x = x;
        w->blocked_y = y;
        result = bfm_forest_manual_tick(&w->manual, room, &w->actor, &w->camera,
                                        0, 0, &w->yaw, w->cached);
    }
    if (result <= BFM_FOREST_MANUAL_BOUNDARY)
        return -1;
    if (w->tower_placed && !w->inner_free && result == BFM_FOREST_MANUAL_MOVING && !slide_tower_stride(w, before))
        return -1;
    /* A submitted direction becomes a stride only after a ground step. */
    int advanced = result == BFM_FOREST_MANUAL_MOVING &&
                   (w->cached[0] != previous_x || w->cached[2] != previous_z);
    w->stride_x = advanced ? x : 0;
    w->stride_y = advanced ? y : 0;
    int16_t support[3];
    /* Near the rim, the forward foot sweep and a vertical sample can
     * land on different facets of the same cell. */
    settle_ground(w, !w->tower_placed && bfm_forest_bank_body_normal(room, before, w->cached, support)
                        ? support : w->manual.ground_normal);
    /* The active world counter advances after the actors run. The initial
     * forest pose already completed its first update at local tick zero. */
    unsigned world_tick = (unsigned)w->entry_world_tick + w->tick;
    if (result == BFM_FOREST_MANUAL_STARTED || result == BFM_FOREST_MANUAL_MOVING) {
        if (result == BFM_FOREST_MANUAL_STARTED ||
            (w->entry_world_tick >= 0 && w->actor.field_2c.word == -11 * 65536 &&
             world_tick % 8 == 0))
            *dust = BFM_FOREST_DUST_TRAIL;
        int brace = w->tower_placed ? 0 : brace_against_body(w, before);
        if (brace < 0) return -1;
        if (brace) return 1;
        w->motion = w->manual.motion;
        w->elapsed = w->manual.elapsed;
        if (!update_pose(w)) return -1;
    } else if (w->tower_placed) {
        w->elapsed = w->motion == IDLE ? w->elapsed + 1 : 0;
        w->motion = IDLE;
        if (!update_pose(w)) return -1;
    } else {
        w->braced = 0;
        BfmForestStandingEvent event = enter_stance ?
            BFM_FOREST_STANDING_STOP : BFM_FOREST_STANDING_CONTINUE;
        if (result == BFM_FOREST_MANUAL_STOPPED || result == BFM_FOREST_MANUAL_RUN_STOPPED)
            event = result == BFM_FOREST_MANUAL_RUN_STOPPED && !w->at_boundary ?
                    BFM_FOREST_STANDING_RUN_STOP : BFM_FOREST_STANDING_STOP;
        if (!bfm_forest_standing_tick(&w->speaker, event, &w->actor, &w->pose))
            return -1;
        w->actor.field_2c.word = bfm_forest_standing_forward_q16(&w->speaker);
        if (w->actor.field_2c.word && event == BFM_FOREST_STANDING_CONTINUE &&
            !w->at_boundary && !bfm_forest_manual_coast(room, &w->actor, w->yaw, w->cached)) {
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
                      BfmForestDustEmission *dust, BfmForestHit hits[BFM_FOREST_STRIKE_RAYS],
                      size_t *count)
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
        if (w->tower_placed && !w->inner_free && ++w->tower_stroke == 0) ++w->tower_stroke;
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
    int16_t support[3];
    /* Blade rays use the body's updated basis even while its feet are still. */
    if (!w->tower_placed && bfm_forest_bank_body_normal(w->field, w->cached, w->cached, support)) {
        settle_ground(w, support);
        update_root(w);
    }
    if (starting) return 1;
    /* The grounded action rolls strength before testing the blade. A miss
     * or a recovering target still advances the shared gameplay stream. */
    uint16_t strength;
    if (!bfm_forest_fusion_strength(&w->fusion_power, &w->gameplay->player, w->actor.strike_index,
                                    bfm_gameplay_story_flag(w->gameplay, 0x81),
                                    bfm_gameplay_random(w->gameplay), &strength)) return 0;
    if (w->actor.field_1a8) ++w->actor.strike_updates;
    if (w->actor.field_1a8 && !w->inner_free) {
        const BfmForestSwingFrame *next = bfm_forest_swing_frame(&w->swing, w->swing_age + 1);
        if (!next) return 0;
        if (w->tower_placed) {
            int16_t start[3], ends[BFM_FOREST_STRIKE_RAYS][3];
            size_t rays = bfm_forest_blade_rays(&w->root, key->tip, next->tip, start, ends);
            for (size_t ray = 0; ray < rays; ++ray) {
                BfmForestHit *hit = &hits[*count];
                if (bfm_tower_field_blade_contact(w->tower, w->tower_stroke, start, ends[ray],
                                                  hit->position, hit->normal)) {
                    hit->connected = 1;
                    ++*count;
                }
            }
        } else {
            *count = bfm_forest_ground_strike_rays(w->field, &w->root, key->tip, next->tip,
                                                  strength, w->actor.strike_index + 1, &w->yaw, hits);
        }
        if (*count) w->swing_connected = 1;
    }
    /* The field display still shows the previous motion during the first two
     * updates after the Square edge. */
    if (w->swing_age < 2) return 1;
    const BfmForestSwingFrame *visible = bfm_forest_swing_frame(&w->swing, w->swing_age - 2);
    return visible && select_pose(w, visible->pose, w->swing.visible);
}

/* The scene's mover aligns the actor to its mark within sixteen world units.
 * The approach uses ordinary swept floor contact for each stride. */
static int tick_lesson_guide(BfmForestWalk *w)
{
    int dx = (int)w->lesson.mark[0] - w->cached[0];
    int dz = (int)w->lesson.mark[2] - w->cached[2];
    double distance = hypot((double)dx, (double)dz);
    int16_t to[3] = {w->cached[0], w->cached[1], w->cached[2]};
    int arrived = distance < 16;
    if (distance > 0) {
        w->yaw = (int16_t)((unsigned)lround(atan2(-dx, -dz) / radians) & 4095);
        if (arrived) {
            to[0] = w->lesson.mark[0];
            to[2] = w->lesson.mark[2];
        } else {
            to[0] = (int16_t)(to[0] + lround(11 * dx / distance));
            to[2] = (int16_t)(to[2] + lround(11 * dz / distance));
        }
        BfmForestContact floor;
        if (!bfm_forest_ground(w->field, w->cached[0], w->cached[1], w->cached[2], &floor) ||
            !bfm_forest_corridor_step(w->field, &floor, w->cached, to)) return 0;
        memcpy(w->cached, to, sizeof to);
        for (unsigned i = 0; i < 3; ++i)
            w->actor.position[i] = (uint32_t)(uint16_t)to[i] << 16;
    }
    w->motion = arrived ? IDLE : RUN;
    ++w->elapsed;
    if (!update_pose(w)) return 0;
    return arrived ? 2 : 1;
}

static int update_lesson_view(BfmForestWalk *w, BfmForestLessonStage before, int incoming_yaw)
{
    BfmForestLessonStage stage = w->lesson.stage;
    if (stage == BFM_LESSON_LOOK && before == BFM_LESSON_TURN)
        bfm_forest_lesson_camera_begin(&w->lens, D_8012695C, w->look_x, w->look_z, 0, 128);
    if (stage >= BFM_LESSON_LOOK && stage <= BFM_LESSON_TEACH) {
        BfmForestLensCue cue = BFM_FOREST_LENS_HOLD;
        if (before == BFM_LESSON_LOOK) cue = BFM_FOREST_LENS_CLIFF;
        if (before == BFM_LESSON_CLIFF_TALK && stage == BFM_LESSON_WAIT_SOLDIER)
            cue = BFM_FOREST_LENS_RETURN;
        if (before == BFM_LESSON_DISCOVERY_TALK) cue = BFM_FOREST_LENS_SOLDIER;
        bfm_forest_lesson_camera_tick(&w->lens, cue, incoming_yaw);
        if (stage >= BFM_LESSON_CLIFF_TALK) {
            BfmForestClearingEvent event = BFM_FOREST_CLEARING_CONTINUE;
            if (stage == BFM_LESSON_CLIFF_TALK && before != stage) event = BFM_FOREST_CLEARING_CLIFF;
            if (stage == BFM_LESSON_WAIT_SOLDIER && before != stage) event = BFM_FOREST_CLEARING_RECOVER;
            if (stage == BFM_LESSON_DISCOVERY_TALK && before != stage) event = BFM_FOREST_CLEARING_FACE_SOLDIER;
            if (!bfm_forest_clearing_pose_tick(&w->speaker, event, &w->pose)) return 0;
            w->frame = w->speaker.frame;
            w->visible = w->speaker.visible;
            memcpy(w->blade_anchors, w->speaker.blade_anchors, sizeof w->blade_anchors);
        }
        return bfm_forest_lesson_camera_view(&w->lens, w->cached, w->orbit,
            (uint16_t)w->follow_projection, w->eye, w->target, &w->camera);
    }
    if (before == BFM_LESSON_TEACH && stage == BFM_LESSON_PRACTICE) {
        w->look_x = w->lens.lead[0];
        w->look_z = w->lens.lead[1];
    }
    return follow(w);
}

static int bridge_control(BfmForestWalk *w, BfmForestRetreatPad *pad)
{
    unsigned prior = w->bridge_phase;
    if (prior < 2 && w->lesson.stage >= BFM_LESSON_DISCOVERY_TALK) prior = 1;
    if (prior < 2 && bfm_forest_status_values(&w->gameplay->player).ability == 8) prior = 2;
    w->bridge_phase = bfm_forest_field_bridge_step(w->field, prior);
    bfm_forest_field_advance_gunshots(w->field, &w->bullets, w->eye, w->gameplay,
                                      bfm_forest_bridge_impact, w->field);
    if (prior == 2 && w->bridge_phase == 3) {
        bfm_forest_bridge_retreat_contact(&w->retreat, prior, 1);
        *pad = (BfmForestRetreatPad){0};
        return 1;
    }
    return bfm_forest_bridge_retreat_controls(&w->retreat, w->actor.state,
        w->jump.active && !w->jump.landed, w->target[0] - w->eye[0],
        w->target[2] - w->eye[2], &w->yaw, pad);
}

/* The release consumes the previous held root. Move its whole coordinates
 * off any solid face before the creature applies its mounted launch profile. */
static int release_lift(BfmForestWalk *w, int upright)
{
    if (!w->lift.target) return 1;
    unsigned i = w->lift.target - 1;
    BfmForestShoreFigures *figures = bfm_forest_lift_figures(w->field);
    BfmForestShoreFigureState body;
    if (bfm_forest_shore_figure_state(figures, i, &body)) {
        uint32_t rank = u32le(w->gameplay->player.record + 0x6c);
        if (rank >= 30) return 0;
        int16_t hit[3];
        if (bfm_forest_resolve_solid_segment(w->field, w->cached, body.position, hit) &&
            !bfm_forest_shore_figure_hold(figures, i, hit, w->yaw)) return 0;
        uint16_t power = (uint16_t)(w->fusion_power.body[rank] * 75u / 100u);
        int released = upright ? bfm_forest_shore_figure_drop(figures, i, power, w->yaw) :
                                 bfm_forest_shore_figure_throw(figures, i, power, w->yaw);
        if (!released) return 0;
    }
    w->lift.target = 0;
    return 1;
}

static int tick_lift(BfmForestWalk *w, int x, int y, int square, BfmForestDustEmission *dust)
{
    BfmForestShoreFigures *figures = bfm_forest_lift_figures(w->field);
    BfmForestLift *lift = &w->lift;
    BfmForestShoreFigureState body;
    int active = lift->target && bfm_forest_shore_figure_state(figures, lift->target - 1, &body);
    BfmLiftStep step = lift->target && !active ? BFM_LIFT_FINISHED :
        bfm_forest_lift_tick(lift, &w->lift_data, x || y, square);
    if (step == BFM_LIFT_FINISHED) {
        *lift = (BfmForestLift){0};
        w->actor.state = 0;
        return tick_ground(w, 0, 0, 1, dust) > 0;
    }
    if (step == BFM_LIFT_ATTACH || step == BFM_LIFT_WALK_ATTACH) {
        int16_t socket[3];
        if (!bfm_forest_lift_grip(&w->lift_data, lift, &w->root, socket)) return 0;
        if (step == BFM_LIFT_WALK_ATTACH) {
            int16_t heading;
            if (!bfm_forest_manual_heading(&w->camera, x, y, &heading)) return 0;
            if (bfm_forest_carry_step(w->field, &w->actor, heading,
                                      w->lift_data.walking_speed, w->cached)) w->yaw = heading;
        }
        if (!bfm_forest_shore_figure_hold(figures, lift->target - 1, socket, w->yaw))
            return 0;
    } else if (step == BFM_LIFT_RELEASE && !release_lift(w, 0)) return 0;
    /* Starting a throw spends four BP even if the held body is interrupted
     * before it leaves Musashi's hands. */
    if (lift->clip == BFM_LIFT_THROW && w->actor.state != 14)
        bfm_forest_status_spend_bp(&w->gameplay->player, 4);
    w->actor.state = lift->clip == BFM_LIFT_THROW ? 14 : 13;
    w->actor.phase = lift->clip == BFM_LIFT_IDLE || lift->clip == BFM_LIFT_WALK ? 2 : 1;
    return select_pose(w, w->lift_data.motion[lift->clip].frames[lift->age], w->walk.visible);
}

/* The field retires a round on contact. The player receives that delivery
 * before processing the next pad sample, even while a previous hit protects him. */
static int tick_hurt(BfmForestWalk *w, BfmForestDustEmission *dust)
{
    int16_t shot_yaw;
    unsigned received = bfm_forest_take_player_strike(w->field, &shot_yaw);
    if (w->actor.field_a8) {
        --w->actor.field_a8;
        received = 0;
    }
    if (received && !w->hurt.active) {
        if (w->jump.active) return 0;
        if ((!w->hurt_clip.visible && !bfm_forest_hurt_load(w->disc, &w->hurt_clip)) ||
            !bfm_forest_hit_damage(w->disc, &w->gameplay->player, w->actor.flags, received, &w->pending_damage) ||
            !bfm_forest_hurt_begin(&w->hurt, w->yaw, shot_yaw)) return -1;
        if (!release_lift(w, w->actor.state == 13)) return -1;
        w->lift = (BfmForestLift){0};
        w->braced = 0;
        w->push_body = 0;
        if (!bfm_forest_queue_hurt_number(w->field, w->cached, w->pending_damage)) return -1;
        w->at_boundary = 0;
    }
    uint32_t frame;
    switch (bfm_forest_hurt_step(&w->hurt, &w->hurt_clip, &w->actor, &w->manual, &frame)) {
    case BFM_FOREST_HURT_FREE:
        return 1;
    case BFM_FOREST_HURT_LOCKED:
        return select_pose(w, frame, w->hurt_clip.visible) ? 2 : -1;
    case BFM_FOREST_HURT_RECOVERED: {
        unsigned debit = w->pending_damage;
        if (w->actor.flags & UINT32_C(0x02000000))
            debit = bfm_forest_status_values(&w->gameplay->player).max_hp;
        bfm_forest_status_debit(&w->gameplay->player, debit);
        w->pending_damage = 0;
        w->actor.field_a8 = 32;
        w->hurt_tail = 3;
        if (bfm_forest_status_values(&w->gameplay->player).hp <= 0) return 0;
        return tick_ground(w, 0, 0, 1, dust) == 1 ? 2 : -1;
    }
    default:
        return -1;
    }
}

static int tick_gunshot_action(BfmForestWalk *w, GunshotInput circle, int interrupted,
                               int *x, int *y, BfmForestGunshotEvent *shot)
{
    if (!interrupted && circle.edge && !w->gun.cost &&
        bfm_forest_status_values(&w->gameplay->player).ability == 8 &&
        (!bfm_forest_prepare_equipped_gunshot(&w->fusion.award, w->disc, &w->gameplay->player) ||
         !bfm_forest_gunshot_load(&w->gun, w->field, &w->fusion.award, w->data.animation_table)))
        return 0;
    *shot = bfm_forest_gunshot_tick(&w->gun, &w->gameplay->player,
                                   circle.edge, circle.held, interrupted);
    if (w->actor.state == 23) w->actor.state = 0;
    if (*shot != BFM_GUNSHOT_GROUND) {
        w->yaw = bfm_forest_gunshot_aim(&w->gun, &w->camera, *x, *y, circle.stick, w->yaw);
        *x = *y = 0;
        w->actor.field_2c.word = 0;
        w->actor.velocity[0] = w->actor.velocity[2] = 0;
        w->manual = (BfmForestManual){.entry_velocity = w->inner_free ? w->data.initial_velocity : 0};
    }
    return 1;
}

static int submit_gunshot_action(BfmForestWalk *w, BfmForestGunshotEvent shot)
{
    if (shot != BFM_GUNSHOT_GROUND) {
        w->actor.state = 23;
        if (!select_pose(w, w->gun.pose, w->visible & ~((UINT32_C(1) << 24) | (UINT32_C(1) << 25))))
            return 0;
        if (shot == BFM_GUNSHOT_FIRED) {
            if (!bfm_forest_bullet_launch(&w->bullets, &w->root, 0, w->yaw, 0)) return 0;
            w->gunshot_fired = 1;
        }
    }
    return 1;
}

static int step_jump(BfmForestWalk *w, const BfmForestField *room, int launched, int continuing_stride,
                     int x, int y, int jump_held, BfmForestDustEmission *dust)
{
    if (w->jump.active && w->jump.walk_off && !w->stride_poses.visible &&
        !bfm_forest_walkoff_poses_load(w->disc, w->data.animation_table, &w->stride_poses))
        return 0;
    if (launched) {
        w->braced = 0;
        w->stride_jump = continuing_stride;
        if (w->stride_jump) {
            if (!w->stride_poses.visible &&
                !bfm_forest_walkoff_poses_load(w->disc, w->data.animation_table, &w->stride_poses))
                return 0;
        } else if (!w->jump_poses.visible &&
                   !bfm_forest_jump_poses_load(w->disc, w->data.animation_table, &w->jump_poses)) {
            return 0;
        }
        w->at_boundary = 0;
        *dust = BFM_FOREST_DUST_TRAIL;
        if (!select_jump_pose(w)) return 0;
    } else if (w->jump.active) {
        int previously_landed = w->jump.landed;
        BfmForestJumpResult result = bfm_forest_jump_tick(&w->jump, room,
            &w->actor, &w->camera, x, y, jump_held, &w->yaw, w->cached);
        w->at_boundary = result == BFM_FOREST_JUMP_BOUNDARY;
        if (w->at_boundary) {
            w->blocked_x = x;
            w->blocked_y = y;
        } else {
            if (result == BFM_FOREST_JUMP_LANDED && !previously_landed && !w->jump.walk_off)
                *dust = BFM_FOREST_DUST_BRAKE;
            if (result == BFM_FOREST_JUMP_FINISHED && !w->jump.walk_off && (x || y))
                *dust = BFM_FOREST_DUST_TRAIL;
            if (result == BFM_FOREST_JUMP_FINISHED) {
                bfm_forest_jump_resume(&w->manual, &w->jump, &w->actor);
                if (w->inner_free) w->manual.entry_velocity = w->data.initial_velocity;
            }
            if (result == BFM_FOREST_JUMP_FINISHED && !w->manual.moving) {
                if (tick_ground(w, 0, 0, 1, dust) < 0) return 0;
            } else if (result == BFM_FOREST_JUMP_FINISHED && w->stride_jump && !w->jump.walk_off) {
                /* Ground control installs the walking entry pose. The next
                 * ground step selects the clip for its speed. */
                w->motion = WALK;
                w->elapsed = 0;
                if (!update_pose(w)) return 0;
            } else {
                if (!select_jump_pose(w)) return 0;
            }
        }
    }
    return 1;
}

static int tick_player(BfmForestWalk *w, int x, int y, int jump_pressed, int jump_held,
                       int attack_pressed, int square_held, int triangle_pressed, int shoulder,
                       GunshotInput circle, int confirm_edge, int reveal_edge)
{
    BfmForestDustEmission dust = BFM_FOREST_DUST_NONE;
    BfmForestHit hits[BFM_FOREST_STRIKE_RAYS] = {0};
    size_t hit_count = 0;
    BfmForestTransform previous_root = w->root;
    uint32_t sweep_pose = 0;
    BfmForestLessonStage before = w->lesson.stage;
    int incoming_yaw = w->yaw, square_edge = attack_pressed;
    int owned = bfm_forest_lesson_owns_player(&w->lesson) ||
        (w->fusion.award.phase > BFM_AWARD_IDLE && w->fusion.award.phase < BFM_AWARD_COMPLETE);
    int at_mark = 0;
    BfmForestRetreatPad script_pad;
    w->retreat_control = bridge_control(w, &script_pad);
    if (w->retreat_control) {
        x = script_pad.x;
        y = script_pad.y;
        jump_pressed = script_pad.cross_pressed;
        jump_held = script_pad.cross_held;
        attack_pressed = square_held = square_edge = triangle_pressed = shoulder = confirm_edge = reveal_edge = 0;
        circle = (GunshotInput){0};
        if (script_pad.cross_pressed) {
            /* The room ends its scripted stride before the directed jump. */
            w->actor.state = 0;
            w->actor.field_24.word = w->actor.local_y.word = w->actor.field_2c.word = 0;
            memset(w->actor.velocity, 0, sizeof w->actor.velocity);
            w->stride_x = w->stride_y = w->manual.moving = 0;
        }
    }
    BfmForestCast *cast = &w->fusion.cast;
    BfmForestCastEvent casting = BFM_CAST_GROUND;
    bfm_forest_hud_tick(w->hud);
    int result = tick_hurt(w, &dust);
    if (result <= 0) return result;
    if (result == 1) {
        if (owned) {
            x = y = jump_pressed = jump_held = attack_pressed = square_held = 0;
            if (!w->jump.active && w->actor.state != 5) {
                if (w->lesson.stage == BFM_LESSON_APPROACH) {
                    int guide = tick_lesson_guide(w);
                    if (!guide) return -1;
                    at_mark = guide == 2;
                } else {
                    w->motion = IDLE;
                    ++w->elapsed;
                    if (!update_pose(w)) return -1;
                }
                w->actor.state = 0;
                w->actor.phase = 1;
                w->actor.field_2c.word = 0;
                memset(w->actor.velocity, 0, sizeof w->actor.velocity);
                memset(&w->manual, 0, sizeof w->manual);
            }
        }
        int away = bfm_forest_lesson_blade_away(&w->fusion);
        if (!owned && before == BFM_LESSON_PRACTICE && !w->jump.active && !jump_pressed &&
            (w->actor.state <= 1 || cast->action == BFM_CAST_STANCE || cast->action == BFM_CAST_THROW)) {
            if (shoulder && !cast->stance.count &&
                !bfm_forest_cast_load(cast, w->disc, w->data.animation_table)) return -1;
            if (cast->action == BFM_CAST_GROUND) cast->visible = w->visible;
            casting = bfm_forest_cast_tick(cast, &w->gameplay->player, shoulder, square_edge, x || y, away);
            if (casting != BFM_CAST_GROUND && casting != BFM_CAST_SWING) attack_pressed = 0;
            if (w->actor.state == 17 || w->actor.state == 21) w->actor.state = 0;
            if (casting == BFM_CAST_THROW || casting == BFM_CAST_LAUNCH) x = y = 0;
        }
        if (away) attack_pressed = 0;
        int interrupted = owned || w->retreat_control || w->jump.active || jump_pressed ||
            (w->actor.state > 1 && w->actor.state != 23);
        BfmForestGunshotEvent shot;
        if (!tick_gunshot_action(w, circle, interrupted, &x, &y, &shot)) return -1;
        if (shot != BFM_GUNSHOT_GROUND) attack_pressed = 0;
        int continuing_stride = (x || y) && x == w->stride_x && y == w->stride_y;
        w->stride_x = w->stride_y = 0;
        bfm_forest_fall_begin(&w->jump, &w->manual, &w->actor);
        int pushing = w->actor.state == 9 && w->push_body;
        int lifting = w->actor.state == 13 || w->actor.state == 14;
        int new_lift = 0;
        int grab_edge = attack_pressed || (triangle_pressed && bfm_gameplay_story_flag(w->gameplay, 0x80));
        if (!owned && !away && shot == BFM_GUNSHOT_GROUND && casting == BFM_CAST_GROUND && !lifting &&
            !w->jump.active && !jump_pressed && grab_edge &&
            w->actor.state == 1 && w->braced && !(w->actor.flags & 0x400u)) {
            new_lift = start_lift(w);
            if (new_lift < 0) return -1;
            lifting = new_lift;
        }
        int swinging = !lifting && (w->actor.state == 5 ||
                       (!w->jump.active && !jump_pressed && attack_pressed && w->actor.state <= 1));
        int launched = !pushing && !lifting && !swinging && !w->jump.active && jump_pressed &&
                       bfm_forest_jump_begin(&w->jump, w->field, &w->actor, w->cached);
        if (!owned && !pushing && !lifting && !w->jump.active && !swinging) {
            int grounded = tick_ground(w, x, y, 0, &dust);
            if (grounded < 0) return -1;
            if (!grounded && !bfm_forest_fall_begin(&w->jump, &w->manual, &w->actor)) return -1;
        }
        if (pushing) {
            if (!tick_boulder_push(w, x, y, square_held)) return -1;
        } else if (lifting) {
            if (!new_lift && !tick_lift(w, x, y, attack_pressed, &dust)) return -1;
        } else if (swinging) {
            w->braced = 0;
            if (!tick_swing(w, x, y, attack_pressed, &dust, hits, &hit_count)) return -1;
            if (w->actor.state == 5) sweep_pose = w->swing.frames[w->swing_age].pose;
        } else if (!step_jump(w, w->field, launched, continuing_stride, x, y, jump_held, &dust)) {
            return -1;
        }
        if (!submit_gunshot_action(w, shot)) return -1;
        if (casting == BFM_CAST_STANCE || casting == BFM_CAST_THROW || casting == BFM_CAST_LAUNCH) {
            w->actor.state = casting == BFM_CAST_STANCE ? 17 : 21;
            if (!select_pose(w, cast->pose, cast->visible)) return -1;
        }
    } else {
        bfm_forest_gunshot_tick(&w->gun, &w->gameplay->player, 0, 0, 1);
        square_edge = 0;
        w->stride_x = w->stride_y = 0;
    }
    int was_attached = w->lesson.tether.phase == BFM_TETHER_ATTACHED;
    int absorption_edge = square_edge && w->lesson.tether.phase == BFM_TETHER_ATTACHED;
    if (!bfm_forest_lesson_flight(&w->fusion, &w->lesson, w->field, &w->root, w->cached,
            w->yaw, w->tick, square_edge, casting == BFM_CAST_LAUNCH)) return -1;
    if (!bfm_forest_field_gunshot_art(w->field, &w->fusion.award, &w->root, w->eye,
                                     w->gameplay, w->gunshot_fired)) return -1;
    if (!bfm_forest_field_fusion_effects(w->field, w->disc, &w->fusion.flight, &w->lesson.tether,
            was_attached, absorption_edge, w->cached, w->gameplay)) return -1;
    if (bfm_forest_lesson_blade_away(&w->fusion))
        w->visible &= ~((UINT32_C(1) << 21) | (UINT32_C(1) << 22));
    BfmForestAward *award = &w->fusion.award;
    if (w->fusion.flight.phase == BFM_FUSION_DONE && w->lesson.tether.phase == BFM_TETHER_EARNED) {
        if (award->phase == BFM_AWARD_IDLE) {
            if (!bfm_forest_award_begin(award, w->disc, cast, &w->lesson.tether, &w->gameplay->player)) return -1;
        } else {
            BfmForestAwardPhase event = bfm_forest_award_tick(award, cast, &w->lesson.tether,
                                                             &w->gameplay->player, w->lesson.award_closed);
            if (event == BFM_AWARD_ERROR) return -1;
            if (event == BFM_AWARD_OPEN) w->lesson.award_ready = 1;
        }
        if (award->phase != BFM_AWARD_COMPLETE) {
            if (!select_pose(w, award->pose, award->visible)) return -1;
            w->actor.state = 22;
        }
    }
    if (cast->stance.count) {
        int gauge = -1;
        if (w->lesson.tether.phase == BFM_TETHER_ATTACHED ||
            (w->lesson.tether.phase == BFM_TETHER_EARNED && bfm_forest_lesson_blade_away(&w->fusion)))
            gauge = (int)w->lesson.tether.meter;
        bfm_forest_hud_gauges(w->hud, w->gameplay->player.record[0x47], gauge);
    }
    bfm_forest_voice_step(w->voice);
    if (!bfm_forest_lesson_enter(&w->lesson, w->field, w->cached[2]) ||
        !bfm_forest_lesson_step(&w->lesson, w->field, w->disc, &w->gameplay->player, w->voice,
                                &w->voice_generation, at_mark, &w->yaw, confirm_edge, reveal_edge))
        return -1;
    if (owned && !bfm_forest_lesson_owns_player(&w->lesson) &&
        (award->phase == BFM_AWARD_IDLE || award->phase == BFM_AWARD_COMPLETE)) {
        /* A dismissed lesson hands back a standing actor. Do not resume
         * a run-stop animation or carry the dismissing Cross into a jump. */
        if (tick_ground(w, 0, 0, 1, &dust) < 0) return -1;
        w->jump_pressed = w->attack_pressed = w->triangle_pressed = w->control_polled = 0;
    }
    if (owned) update_root(w);
    ++w->tick;
    if (!bfm_forest_tick_sweep(w->field, w->disc, w->swing_sound, sweep_pose, &previous_root) ||
        !update_lesson_view(w, before, incoming_yaw) || !bfm_forest_tick_shore_actor_with_run(w->field, w->cached, w->gameplay) ||
        !bfm_forest_submit_guard_and_impacts_with_run(w->field, hits, hit_count, w->eye, &w->camera, w->gameplay))
        return -1;
    BfmForestBodySample sample = {w->root, sweep_pose ? sweep_pose : w->frame, w->visible};
    /* During the protection countdown only the actor model blinks. The
     * independent sword halo, shot, number and terrain still submit. */
    if (w->actor.field_a8 & 1u) sample.visible = 0;
    BfmForestBodySample delayed = bfm_forest_hurt_display(&w->hurt_display, sample);
    w->hide_recovering_body = !delayed.visible;
    if (w->hurt.active || w->hurt_tail) {
        if (delayed.pose != w->displayed_body.pose &&
            !read_body(w, delayed.pose, &w->displayed_pose, w->displayed_anchors)) return -1;
        w->displayed_body = delayed;
        if (w->hurt_tail) --w->hurt_tail;
    } else {
        w->displayed_body.pose = 0;
    }
    bfm_forest_hud_submit_points(w->hud, &w->gameplay->player);
    w->hit_sound = hit_count != 0;
    bfm_forest_tick_dust(w->field, dust, w->cached, w->eye, w->yaw);
    return 1;
}

int bfm_forest_walk_hit_sound(const BfmForestWalk *w)
{
    return w && w->hit_sound;
}

const RoomTransferDescriptor *bfm_forest_walk_inner_exit(const BfmForestWalk *w)
{
    return w ? w->inner_exit : NULL;
}

BfmTowerBellSound *bfm_forest_walk_bell_sound(const BfmForestWalk *w,
                                              unsigned *command, unsigned *argument)
{
    return w ? bfm_tower_interior_sound(w->inner, command, argument) : NULL;
}

int bfm_forest_walk_gunshot_sound(const BfmForestWalk *w)
{
    return w && w->gunshot_fired;
}

int bfm_forest_walk_swing_sound(const BfmForestWalk *w)
{
    return w && w->swing_sound;
}

uint16_t bfm_forest_walk_room(const BfmForestWalk *w)
{
    if (w && w->drawbridge) return bfm_castle_drawbridge_room(w->drawbridge);
    if (w && w->summit) return 0x300b;
    if (w && w->inner) return 0x300a;
    return w ? bfm_forest_gateway_transition_frame(&w->gateway).room : 0;
}

int bfm_forest_walk_take_castle_request(BfmForestWalk *w, RoomTransferDescriptor *destination)
{
    return w && bfm_gameplay_story_flag(w->gameplay, 0x80) &&
           bfm_tower_escape_take_castle(&w->escape, destination);
}

int bfm_forest_walk_control(const BfmForestWalk *w)
{
    if (w && w->drawbridge) return 0;
    if (w && w->summit_transfer.phase != BFM_SUMMIT_TRANSFER_IDLE) {
        if (w->escape.phase) return w->escape.phase == BFM_ESCAPE_UPPER || w->escape.phase == BFM_ESCAPE_LOWER;
        if (!w->summit_free || bfm_tower_summit_pickup(w->summit) || bfm_tower_summit_active_lesson(w->summit)) return 0;
        return w->at_boundary ? 2 : 1;
    }
    if (w && w->inner_age) {
        BfmTowerBellResponse reaction;
        if (!w->inner_free || (bfm_tower_interior_response(w->inner, &reaction) && reaction.locked)) return 0;
        return w->at_boundary ? 2 : 1;
    }
    if (w && w->gateway.active) {
        BfmForestGatewayFrame frame = bfm_forest_gateway_transition_frame(&w->gateway);
        if (!frame.live || (frame.room == 0x3009 && (!frame.complete || w->tower_note ||
                                    (bfm_tower_field_plate_pressing(w->tower) || bfm_tower_field_revealing(w->tower))))) return 0;
    }
    if (!w || !w->player_control || w->retreat_control || bfm_forest_lesson_owns_player(&w->lesson) ||
        (w->fusion.award.phase > BFM_AWARD_IDLE && w->fusion.award.phase < BFM_AWARD_COMPLETE))
        return 0;
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

static int gateway_walk(BfmForestWalk *w, const BfmForestGatewayFrame *frame)
{
    if (frame->motion == BFM_GATEWAY_STOP) {
        memset(w->actor.velocity, 0, sizeof w->actor.velocity);
        w->actor.field_24.word = w->actor.local_y.word = w->actor.field_2c.word = 0;
        w->actor.state = 0;
        w->actor.phase = 1;
        w->motion = IDLE;
        w->elapsed = 0;
        return update_pose(w);
    }
    if (frame->motion == BFM_GATEWAY_KEEP) return 1;
    if (frame->motion == BFM_GATEWAY_WALK_START) {
        w->yaw = w->tower_placed ? w->entrance_yaw : (int16_t)w->gateway.destination.facing;
        w->actor.state = w->actor.phase = 1;
        w->actor.field_24.word = w->actor.local_y.word = 0;
    } else {
        BfmForestField *room = w->tower_placed ? bfm_tower_field_room(w->tower) : w->field;
        /* The departing control owner carries the one-unit turn from the
         * north steering step. The entering walk keeps the new model yaw. */
        if (!w->tower_placed)
            w->yaw = (int16_t)((w->gateway.destination.facing + 1) & 4095);
        w->actor.field_2c.word = -frame->forward_speed_q16;
        if (!bfm_forest_manual_coast(room, &w->actor, w->yaw, w->cached))
            return 0;
    }
    w->motion = frame->running ? RUN : WALK;
    w->elapsed = frame->pose_index;
    return update_pose(w);
}

static int tick_tower_player(BfmForestWalk *w, int x, int y, int square_edge)
{
    BfmForestDustEmission dust = BFM_FOREST_DUST_NONE;
    if (w->actor.state == 5 || (w->actor.state <= 1 && square_edge)) {
        BfmForestHit hits[BFM_FOREST_STRIKE_RAYS] = {0};
        size_t count = 0;
        if (!tick_swing(w, x, y, square_edge, &dust, hits, &count)) return -1;
        w->hit_sound = count != 0;
        return 1;
    }
    return tick_ground(w, x, y, 0, &dust);
}

static int open_tower_note(BfmForestWalk *w, BfmTowerNoteKind kind)
{
    BfmForestField *room = bfm_tower_field_room(w->tower);
    w->tower_note = bfm_tower_note_open(room, w->gateway.destination.parameter, kind);
    if (!w->tower_note) return 0;
    w->tower_note_kind = kind;
    if (kind == BFM_TOWER_ARRIVAL_NOTE)
        return bfm_tower_note_pose_begin(&w->tower_note_pose, room, w->disc, w->visible, &w->pose);
    return 1;
}

static int step_tower_note(BfmForestWalk *w, int confirm, int reveal)
{
    if (w->tower_note_kind == BFM_TOWER_ARRIVAL_NOTE) {
        const BfmForestMessageFrame *prior = bfm_forest_message_frame(w->tower_note);
        if (!bfm_tower_note_pose_tick(&w->tower_note_pose, prior->actor_flags, &w->pose)) return 0;
        w->frame = w->tower_note_pose.actor.frame;
        w->visible = w->tower_note_pose.actor.visible;
        memcpy(w->blade_anchors, w->tower_note_pose.actor.blade_anchors, sizeof w->blade_anchors);
    }
    bfm_forest_voice_step(w->voice);
    const BfmForestMessageFrame *message = bfm_forest_voice_page(
        w->voice, w->tower_note, &w->voice_generation, confirm, reveal);
    if (!message) return 0;
    if (message->events & BFM_FOREST_MESSAGE_CLOSED) {
        bfm_forest_message_free(w->tower_note);
        w->tower_note = NULL;
        if (w->tower_note_kind == BFM_TOWER_PLATE_NOTE)
            bfm_tower_field_close_plate_note(w->tower);
        w->actor.state = 0;
        w->actor.phase = 1;
        w->manual = (BfmForestManual){.entry_velocity = w->data.initial_velocity};
        w->motion = IDLE;
        w->elapsed = 0;
        return update_pose(w);
    }
    return 1;
}

/* Measured in room updates, each covering two display fields. The room
 * selector changes while the old scene is fading. The destination owns
 * placement and the authored camera sequence independently. */
enum { INNER_SELECT=31, INNER_WIPE=44, INNER_BLACK=125, INNER_PLACE=150, INNER_CAMERA=198, INNER_LIT=215 };

static int step_inner_player(BfmForestWalk *w, BfmForestField *room, int x, int y,
                             int square_edge, int jump_pressed, int jump_held, GunshotInput circle)
{
    /* The entrance holds the player above the floor while reading. The
     * first directional step settles the feet on the room's surface. */
    if (w->inner_entry_foot && w->manual.moving && (x || y)) {
        BfmForestContact floor;
        if (!bfm_forest_ground(room, w->cached[0], w->cached[1], w->cached[2], &floor) ||
            abs(floor.height - w->cached[1]) > 16) return -1;
        w->cached[1] = floor.height;
        w->actor.position[1] = (uint32_t)(uint16_t)floor.height << 16;
        w->inner_entry_foot = 0;
    }
    BfmForestGunshotEvent shot;
    int interrupted = w->jump.active || jump_pressed || square_edge || (w->actor.state > 1 && w->actor.state != 23);
    if (!tick_gunshot_action(w, circle, interrupted, &x, &y, &shot)) return -1;
    int continuing_stride = (x || y) && x == w->stride_x && y == w->stride_y;
    w->stride_x = w->stride_y = 0;
    bfm_forest_fall_begin(&w->jump, &w->manual, &w->actor);
    BfmForestDustEmission dust = BFM_FOREST_DUST_NONE;
    if (w->actor.state == 5 ||
        (!w->jump.active && !jump_pressed && square_edge && w->actor.state <= 1)) {
        BfmForestHit hits[BFM_FOREST_STRIKE_RAYS] = {0};
        size_t hit_count = 0;
        return tick_swing(w, x, y, square_edge, &dust, hits, &hit_count) ? 1 : -1;
    }
    int launched = !w->jump.active && jump_pressed &&
                   bfm_forest_jump_begin(&w->jump, room, &w->actor, w->cached);
    if (!w->jump.active) {
        int moved = tick_ground(w, x, y, 0, &dust);
        if (moved < 0 || (!moved && !bfm_forest_fall_begin(&w->jump, &w->manual, &w->actor)))
            return -1;
    }
    if (!step_jump(w, room, launched, continuing_stride, x, y, jump_held, &dust) ||
        !submit_gunshot_action(w, shot)) return -1;
    return 1;
}

static int inner_play(BfmForestWalk *w, int x, int y, int square_edge, int jump_pressed, int jump_held,
                        GunshotInput circle, int confirm, int reveal)
{
    BfmForestField *room = bfm_tower_interior_room(w->inner);
    unsigned count = bfm_tower_interior_intro_updates(w->inner);
    bfm_tower_interior_tick(w->inner, w->gameplay);
    if (w->inner_free) {
        bfm_forest_field_advance_gunshots(room, &w->bullets, w->eye, w->gameplay,
                                          bfm_tower_interior_gunshot_contact, w->inner);
        BfmTowerBellResponse reaction = {0};
        int reacting = bfm_tower_interior_response(w->inner, &reaction);
        if (reacting && reaction.age == 1) bfm_bedroom_hud_hide(w->hud);
        if (reacting && reaction.door_open) bfm_forest_hud_show(w->hud);
        bfm_forest_hud_tick(w->hud);
        if (reacting && reaction.locked) {
            if (!reaction.age) {
                bfm_forest_gunshot_tick(&w->gun, &w->gameplay->player, 0, 0, 1);
                w->actor.state = 0;
                w->actor.phase = 1;
                w->motion = IDLE;
                w->elapsed = 0;
                w->manual = (BfmForestManual){.entry_velocity = w->data.initial_velocity};
            } else {
                ++w->elapsed;
            }
            if (!update_pose(w)) return -1;
        } else if (step_inner_player(w, room, x, y, square_edge, jump_pressed, jump_held, circle) < 0) {
            return -1;
        }
        if (!w->inner_exit && !w->jump.active && w->actor.state <= 1)
            w->inner_exit = bfm_tower_interior_exit(w->inner, w->cached);
        if (!bfm_forest_field_gunshot_art(room, &w->fusion.award, &w->root, w->eye,
                                         w->gameplay, w->gunshot_fired)) return -1;
    } else {
        if (!w->inner_shot &&
            !bfm_tower_interior_note_begin(&w->inner_note, room, w->disc, w->visible, &w->pose))
            return -1;
        if (w->inner_shot == count &&
            !bfm_tower_interior_note_open(&w->inner_note, w->summit_door.entry)) return -1;
        bfm_forest_voice_step(w->voice);
        if (!bfm_tower_interior_note_tick(&w->inner_note, w->voice, &w->voice_generation,
                                           confirm, reveal, &w->pose)) return -1;
        w->frame = w->inner_note.actor.frame;
        w->visible = w->inner_note.actor.visible;
        memcpy(w->blade_anchors, w->inner_note.actor.blade_anchors, sizeof w->blade_anchors);
        if (w->inner_note.page &&
            (bfm_forest_message_frame(w->inner_note.page)->events & BFM_FOREST_MESSAGE_CLOSED)) {
            bfm_tower_interior_note_close(&w->inner_note);
            w->inner_free = 1;
            w->inner_entry_foot = 1;
            w->motion = IDLE;
            w->elapsed = 0;
            if (!update_pose(w)) return -1;
        }
    }
    if (w->actor.state != 2 && !w->jump.active)
        bfm_tower_interior_grounded_sound(w->inner, w->cached);
    if (!bfm_tower_interior_camera(w->inner, w->cached, w->inner_shot, w->inner_free,
                                   &w->camera, w->eye)) return -1;
    if (w->inner_shot <= count) ++w->inner_shot;
    bfm_forest_hud_submit_points(w->hud, &w->gameplay->player);
    return 1;
}

static int step_inner_entry(BfmForestWalk *w, int x, int y, int square_edge, int jump_pressed, int jump_held,
                        GunshotInput circle, int confirm, int reveal)
{
    if (w->inner_age < INNER_LIT) ++w->inner_age;
    if (w->inner_age == INNER_SELECT) {
        w->inner = bfm_tower_interior_open(w->disc, &w->summit_door.destination);
        if (!w->inner) return -1;
    }
    if (w->inner_age == INNER_PLACE) {
        BfmForestField *room = bfm_tower_interior_room(w->inner);
        BfmForestMessageArt *art = bfm_tower_interior_note_art(room, w->disc);
        if (!art) return -1;
        bfm_forest_message_art_free(w->message_art);
        w->message_art = art;
        w->actor = (ForestPlayerState){0};
        if (!bfm_tower_interior_place(w->inner, &w->actor, &w->yaw, w->cached)) return -1;
        w->at_boundary = 0;
        w->manual = (BfmForestManual){.entry_velocity=w->data.initial_velocity};
        w->motion = IDLE;
        w->elapsed = 0;
        if (!update_pose(w)) return -1;
    }
    if (w->inner_age >= INNER_CAMERA) {
        int result = inner_play(w, x, y, square_edge, jump_pressed, jump_held, circle, confirm, reveal);
        if (result != 1) return result;
    }
    ++w->tick;
    return 1;
}

static int step_summit_departure(BfmForestWalk *w, const BfmTowerSummitFrame *frame)
{
    BfmForestField *room = bfm_tower_interior_room(w->inner);
    if (w->summit) return 1;
    bfm_tower_interior_tick(w->inner, w->gameplay);
    if (frame->outgoing_walk) {
        if (bfm_tower_summit_transfer_at_lift(w->cached[0], w->cached[1], w->cached[2])) {
            BfmForestContact floor;
            if (!bfm_forest_ground(room, frame->goal_x, frame->goal_y, frame->goal_z, &floor)) return 0;
            int16_t at[3] = {(int16_t)frame->goal_x, floor.height, (int16_t)frame->goal_z};
            restore_whole_position(w, at);
            w->actor.state = 0;
            w->actor.phase = 1;
            w->actor.field_2c.word = 0;
            memset(w->actor.velocity, 0, sizeof w->actor.velocity);
            bfm_tower_summit_transfer_reach_lift(&w->summit_transfer, w->yaw);
            w->motion = IDLE;
            w->elapsed = 0;
        } else {
            /* The room supplies a full stick toward the board. Keep the
             * grounded actor's incoming speed when its exit walk starts. */
            w->yaw = (int16_t)bfm_forest_shore_bearing(w->cached[0] - frame->goal_x,
                                                      w->cached[2] - frame->goal_z);
            w->actor.state = w->actor.phase = 1;
            if (w->motion != RUN) {
                w->motion = RUN;
                w->elapsed = 0;
            } else {
                ++w->elapsed;
                int32_t speed = w->actor.field_2c.word;
                if (!speed) speed = w->data.initial_velocity;
                if (speed < -11 * 65536) speed = -11 * 65536;
                w->actor.field_2c.word = speed - 4096;
                if (!bfm_forest_manual_coast(room, &w->actor, w->yaw, w->cached)) return 0;
            }
        }
    } else {
        w->actor.state = 0;
        w->actor.phase = 1;
        w->elapsed = w->motion == IDLE ? w->elapsed + 1 : 0;
        w->motion = IDLE;
        if (frame->phase == BFM_SUMMIT_TRANSFER_ALIGN || frame->phase == BFM_SUMMIT_TRANSFER_LIFT)
            w->yaw = (int16_t)frame->actor_facing;
    }
    if (!update_pose(w)) return 0;
    if (frame->wide_camera)
        return bfm_tower_summit_departure_camera(&w->camera, w->eye);
    return bfm_tower_interior_camera(w->inner, w->cached, w->inner_shot, 1,
                                     &w->camera, w->eye);
}

static int place_on_summit(BfmForestWalk *w)
{
    w->actor = (ForestPlayerState){0};
    if (!bfm_tower_summit_place(w->summit, &w->actor, &w->yaw, w->cached)) return 0;
    w->manual = (BfmForestManual){.entry_velocity = w->data.initial_velocity};
    w->jump = (BfmForestJump){0};
    w->at_boundary = w->support_y = w->support_z = 0;
    w->motion = IDLE;
    w->elapsed = 0;
    if (!update_pose(w) || !bfm_tower_summit_begin(w->summit, w->visible, &w->pose)) return 0;
    w->summit_placed = 1;
    return bfm_tower_summit_camera(w->cached, &w->camera, w->eye);
}

/* An event-supplied mark drives the same grounded actor as ordinary input. */
static int walk_to_summit_mark(BfmForestWalk *w, const int16_t goal[3])
{
    int64_t dx = w->cached[0] - goal[0];
    int64_t dy = w->cached[1] - goal[1];
    int64_t dz = w->cached[2] - goal[2];
    if (dx * dx + dy * dy + dz * dz < 16 * 16) {
        restore_whole_position(w, goal);
        w->actor.state = 0;
        w->actor.phase = 1;
        w->actor.field_2c.word = 0;
        memset(w->actor.velocity, 0, sizeof w->actor.velocity);
        w->elapsed = w->motion == IDLE ? w->elapsed + 1 : 0;
        w->motion = IDLE;
        return 1;
    }
    w->yaw = (int16_t)bfm_forest_shore_bearing((int)dx, (int)dz);
    w->actor.state = w->actor.phase = 1;
    if (w->motion != RUN) {
        w->motion = RUN;
        w->elapsed = 0;
        return 0;
    }
    ++w->elapsed;
    int32_t speed = w->actor.field_2c.word;
    if (!speed) speed = w->data.initial_velocity;
    if (speed < -11 * 65536) speed = -11 * 65536;
    w->actor.field_2c.word = speed - 4096;
    return bfm_forest_manual_coast(bfm_tower_summit_room(w->summit), &w->actor,
                                   w->yaw, w->cached) ? 0 : -1;
}

static int step_summit_lesson(BfmForestWalk *w, BfmTowerSummitLesson *lesson, int confirm, int reveal)
{
    int settled = 0;
    if (lesson->age >= 2) {
        BfmForestContact floor;
        if (!bfm_forest_ground(bfm_tower_summit_room(w->summit), lesson->walk_x,
                               w->cached[1], lesson->walk_z, &floor)) return -1;
        int16_t goal[3] = {lesson->walk_x, floor.height, lesson->walk_z};
        int already_at_mark = w->cached[0] == goal[0] && w->cached[2] == goal[2];
        settled = walk_to_summit_mark(w, goal);
        if (settled < 0) return -1;
        if (settled) {
            int facing = w->yaw;
            if (already_at_mark) w->yaw = (int16_t)bfm_tower_step_facing(facing, lesson->turn_yaw);
            settled = already_at_mark && facing == lesson->turn_yaw;
        }
    }
    if (!update_pose(w)) return -1;
    bfm_forest_voice_step(w->voice);
    int result = bfm_tower_summit_lesson_tick(lesson, w->voice, &w->voice_generation,
                                             w->cached, settled, confirm, reveal);
    if (result == BFM_SUMMIT_LESSON_ERROR) return -1;
    if (result == BFM_SUMMIT_LESSON_RELEASED) bfm_tower_summit_lesson_close(lesson);
    return 1;
}

static int step_summit_pickup(BfmForestWalk *w, BfmTowerSummitPickup *pickup)
{
    int16_t mark[3], yaw;
    BfmSummitPickupGoal goal = bfm_tower_summit_pickup_goal(pickup, mark, &yaw);
    int aligned = 0, facing = 0;
    if (goal == BFM_SUMMIT_PICKUP_ALIGN) {
        aligned = walk_to_summit_mark(w, mark);
        if (aligned < 0 || !update_pose(w)) return 0;
    } else if (goal == BFM_SUMMIT_PICKUP_TURN) {
        facing = w->yaw == yaw;
        w->yaw = (int16_t)bfm_tower_step_facing(w->yaw, yaw);
    }
    /* The original HUD starts sliding on the first update after contact. */
    bfm_bedroom_hud_hide(w->hud);
    if (!bfm_tower_summit_pickup_tick(pickup, w->gameplay, aligned, facing, &w->pose)) return 0;
    const BfmSummitPickupFrame *frame = bfm_tower_summit_pickup_frame(pickup);
    if (frame->has_pose) {
        w->visible = frame->visible_parts;
        memcpy(w->blade_anchors, frame->fusion_anchors, sizeof w->blade_anchors);
        w->frame = 0;
    }
    update_root(w);
    return bfm_tower_summit_pickup_camera(pickup, w->cached, &w->camera, w->eye);
}

static int step_summit_player(BfmForestWalk *w, int x, int y, BfmSummitSkillInput input)
{
    input.visible = w->visible;
    input.moving = x || y;
    input.grounded = w->actor.state != 5;
    const BfmSummitSkillFrame *skill = bfm_tower_summit_skill_tick(w->summit,
        w->data.animation_table, &w->root, w->yaw, w->tick, &input, w->gameplay);
    if (!skill) return -1;
    bfm_forest_hud_gauges(w->hud, w->gameplay->player.record[0x47], skill->tether_gauge);
    int16_t facing = w->yaw;
    BfmForestDustEmission dust = BFM_FOREST_DUST_NONE;
    if (!skill->suppress_square && (w->actor.state == 5 || input.square_edge)) {
        BfmForestHit hits[BFM_FOREST_STRIKE_RAYS] = {0};
        size_t count = 0;
        if (!tick_swing(w, x, y, input.square_edge, &dust, hits, &count)) return -1;
    } else if (!skill->action || skill->action == 17) {
        int moved = tick_ground(w, x, y, 0, &dust);
        if (moved != 1) return moved;
        if (skill->action == 17) w->yaw = facing;
    } else if (w->actor.state != skill->action) {
        w->manual.moving = 0;
        w->actor.field_24.word = w->actor.field_2c.word = w->actor.local_y.word = 0;
        memset(w->actor.velocity, 0, sizeof w->actor.velocity);
    }
    if (skill->action) {
        w->actor.state = (uint8_t)skill->action;
        w->actor.phase = 1;
        if (skill->skill_body) {
            w->pose = *skill->skill_body;
            w->frame = 0;
            w->visible = skill->visible;
            memcpy(w->blade_anchors, skill->skill_anchors[0], sizeof w->blade_anchors);
            update_root(w);
        } else if (!select_pose(w, skill->pose, skill->visible)) {
            return -1;
        }
    }
    if (skill->blade_away) w->visible &= ~((UINT32_C(1) << 21) | (UINT32_C(1) << 22));
    return bfm_tower_summit_camera(w->cached, &w->camera, w->eye) ? 1 : -1;
}

static int step_summit_escape(BfmForestWalk *w, BfmTowerSummitPickup *pickup,
                              int horizontal, int cross_edge, int cross_held)
{
    BfmForestField *field = bfm_tower_summit_room(w->summit);
    BfmTowerEscape *e = &w->escape;
    if (!e->phase && (!bfm_gameplay_story_flag(w->gameplay, 0x80) ||
        !bfm_tower_escape_begin(e, field, w->cached, w->yaw))) return -1;
    unsigned action = e->action;
    if (!bfm_tower_escape_tick(e, field, horizontal, cross_edge, cross_held)) return -1;
    if (e->phase == BFM_ESCAPE_CAUGHT && e->age == 7) {
        BfmForestStatusValues v = bfm_forest_status_values(&w->gameplay->player);
        if (v.hp > 0) bfm_forest_status_debit(&w->gameplay->player, (unsigned)v.hp);
    }
    restore_whole_position(w, e->position);
    w->yaw = e->yaw;
    w->actor.state = (uint8_t)e->action;
    w->actor.phase = 1;
    w->elapsed = action == e->action ? w->elapsed + 1 : 0;
    w->motion = e->action && e->action < 4 ? RUN : IDLE;
    if (e->action == 2 || e->action == 3) {
        if (!w->jump_poses.visible &&
            !bfm_forest_jump_poses_load(w->disc, w->data.animation_table, &w->jump_poses)) return -1;
        enum BfmForestJumpPhase phase = e->action == 2 ? BFM_FOREST_JUMP_RISING : BFM_FOREST_JUMP_FALLING;
        uint32_t frame = bfm_forest_jump_pose_frame(&w->jump_poses, phase, w->elapsed);
        if (!frame || !select_pose(w, frame, w->jump_poses.visible)) return -1;
    } else if (!update_pose(w)) return -1;
    /* Locomotion keeps the sword equipment selected by the pickup script. */
    const uint32_t weapons = UINT32_C(0x3f) << 21;
    w->visible = (w->visible & ~weapons) |
                 (bfm_tower_summit_pickup_frame(pickup)->visible_parts & weapons);
    if (e->phase >= BFM_ESCAPE_UPPER) bfm_forest_hud_show(w->hud);
    bfm_forest_hud_tick(w->hud);
    bfm_forest_hud_submit_points(w->hud, &w->gameplay->player);
    if (!bfm_tower_summit_road_tick(w->summit, e)) return -1;
    return bfm_tower_escape_lens(&w->escape_camera, e->phase, e->age, e->position,
                                 &w->camera, w->eye) ? 1 : -1;
}

static int summit_play(BfmForestWalk *w, int x, int y, int ready, BfmSummitSkillInput input,
                       int jump_pressed, int jump_held)
{
    int confirm = input.confirm_edge, reveal = input.reveal_edge;
    if (w->summit_free) {
        BfmTowerSummitPickup *pickup = bfm_tower_summit_pickup(w->summit);
        BfmTowerSummitLesson *lesson = bfm_tower_summit_active_lesson(w->summit);
        if (pickup && bfm_tower_summit_pickup_goal(pickup, NULL, NULL) == BFM_SUMMIT_PICKUP_FOLLOWUP)
            return step_summit_escape(w, pickup, x, jump_pressed, jump_held);
        if (pickup) {
            if (!step_summit_pickup(w, pickup)) return -1;
        } else if (lesson) {
            if (step_summit_lesson(w, lesson, confirm, reveal) != 1) return -1;
        } else {
            int moved = step_summit_player(w, x, y, input);
            if (moved != 1) return moved;
        }
        bfm_forest_hud_tick(w->hud);
        bfm_forest_hud_submit_points(w->hud, &w->gameplay->player);
        if (!bfm_tower_summit_step_fixtures(w->summit, w->cached, w->eye, w->gameplay,
                                           lesson != NULL || pickup != NULL, w->visible)) return -1;
        BfmTowerSummitLesson *current = bfm_tower_summit_active_lesson(w->summit);
        if ((!pickup && bfm_tower_summit_pickup(w->summit)) || (!lesson && current)) {
            w->actor.state = 0;
            w->actor.phase = 1;
            w->manual.moving = 0;
            w->motion = IDLE;
            w->elapsed = 0;
            /* Sword contact holds the previous pose until its next update. */
            if (current && !update_pose(w)) return -1;
        }
        if (current) return bfm_tower_summit_lesson_camera(current, &w->camera, w->eye) ? 1 : -1;
        if (pickup) return 1;
        return bfm_tower_summit_camera(w->cached, &w->camera, w->eye) ? 1 : -1;
    } else {
        bfm_forest_voice_step(w->voice);
        int result = bfm_tower_summit_tick_arrival(w->summit, w->voice, &w->voice_generation,
                                                   ready, confirm, reveal, &w->pose);
        if (result == BFM_SUMMIT_ARRIVAL_ERROR) return -1;
        if (!bfm_tower_summit_step_fixtures(w->summit, w->cached, w->eye, w->gameplay, 1, w->visible)) return -1;
        if (result == BFM_SUMMIT_ARRIVAL_RELEASED) {
            w->summit_free = 1;
            w->motion = IDLE;
            w->elapsed = 0;
            w->frame = 0;
            if (!update_pose(w)) return -1;
        } else {
            const BfmTowerSummitArrival *arrival = bfm_tower_summit_arriving(w->summit);
            w->frame = arrival->actor.frame;
            w->visible = arrival->parts;
            memcpy(w->blade_anchors, arrival->actor.blade_anchors, sizeof w->blade_anchors);
        }
    }
    bfm_forest_hud_submit_points(w->hud, &w->gameplay->player);
    return bfm_tower_summit_camera(w->cached, &w->camera, w->eye) ? 1 : -1;
}

static int step_summit_transfer(BfmForestWalk *w, int x, int y, BfmSummitSkillInput input,
                                int jump_pressed, int jump_held)
{
    bfm_tower_summit_transfer_advance(&w->summit_transfer, w->summit != NULL);
    BfmTowerSummitFrame frame = bfm_tower_summit_transfer_frame(&w->summit_transfer);
    if (frame.select_destination && !w->summit) {
        w->summit = bfm_tower_summit_open(w->disc, &w->summit_transfer.destination);
        if (!w->summit) return -1;
    }
    if (frame.place_destination && !w->summit_placed && !place_on_summit(w)) return -1;
    if (w->summit_placed) {
        int result = summit_play(w, x, y, frame.entrance_ready, input, jump_pressed, jump_held);
        if (result != 1) return result;
    } else if (!step_summit_departure(w, &frame)) {
        return -1;
    }
    ++w->tick;
    return 1;
}

/* Advance the selected room without interpreting a guest callback. The
 * outgoing transfer and the destination's own entrance are distinct records. */
static int tick_gateway(BfmForestWalk *w, int x, int y, int square_edge, int jump_pressed,
                        int jump_held, GunshotInput circle, int confirm, int reveal)
{
    if (w->inner_age) return step_inner_entry(w, x, y, square_edge, jump_pressed, jump_held, circle, confirm, reveal);
    BfmForestGatewayFrame prior = bfm_forest_gateway_transition_frame(&w->gateway);
    bfm_forest_gateway_transition_advance(&w->gateway, w->tower != NULL);
    BfmForestGatewayFrame frame = bfm_forest_gateway_transition_frame(&w->gateway);
    if (frame.select_destination) {
        w->last_forest_tick = w->tick;
        w->tower = bfm_tower_field_open(w->disc, &w->gateway.destination);
        if (!w->tower || !bfm_tower_ascent_door_load(&w->summit_door,
                                                   bfm_tower_field_room(w->tower))) return -1;
        BfmForestMessageArt *art = bfm_tower_note_art(bfm_tower_field_room(w->tower), w->disc);
        if (!art) return -1;
        bfm_forest_message_art_free(w->message_art);
        w->message_art = art;
    }
    if (frame.view == BFM_GATEWAY_BLACK && w->held_forest) {
        free(w->held_forest);
        w->held_forest = NULL;
    }
    if (frame.place_player) {
        /* Room creation starts a fresh position accumulator. Inventory and
         * player status stay with the application-owned gameplay record. */
        w->actor = (ForestPlayerState){0};
        if (!bfm_tower_field_place(w->tower, &w->actor, &w->yaw, w->cached)) return -1;
        /* Projectiles belong to the room that registered them. */
        w->bullets = (BfmForestBullets){0};
        bfm_forest_gunshot_tick(&w->gun, &w->gameplay->player, 0, 0, 1);
        w->entrance_yaw = w->yaw;
        w->tower_placed = 1;
        w->at_boundary = w->support_y = w->support_z = 0;
        w->manual = (BfmForestManual){.entry_velocity = w->data.initial_velocity};
        w->motion = IDLE;
        w->elapsed = 0;
        if (!update_pose(w)) return -1;
    }
    if (w->tower_note) {
        if (!step_tower_note(w, confirm, reveal)) return -1;
    } else if (prior.complete && bfm_tower_field_plate_note(w->tower)) {
        if (!open_tower_note(w, BFM_TOWER_PLATE_NOTE)) return -1;
    } else if (prior.complete && !bfm_tower_field_plate_pressing(w->tower) && !bfm_tower_field_revealing(w->tower)) {
        int advanced = tick_tower_player(w, x, y, square_edge);
        if (advanced != 1) return advanced;
        if (bfm_tower_field_raised(w->tower) && w->actor.state <= 1) {
            BfmForestContact floor;
            if (bfm_forest_ground(bfm_tower_field_room(w->tower), w->cached[0], w->cached[1],
                                  w->cached[2], &floor) && floor.height == w->cached[1] &&
                bfm_tower_ascent_door_contact(&w->summit_door, &floor, w->cached))
                w->inner_age = 1;
        }
    } else if (frame.live && frame.room == 0x3008) {
        BfmForestDustEmission dust = BFM_FOREST_DUST_NONE;
        int moved = tick_ground(w, x, y, 0, &dust);
        if (moved != 1) return moved;
    } else if (!prior.complete && !gateway_walk(w, &frame)) {
        return -1;
    }
    if (frame.complete && !prior.complete) {
        /* The room becomes visible before its automatic entrance walk ends.
         * Recognition takes focus when the thirtieth update stops the player. */
        if (!open_tower_note(w, BFM_TOWER_ARRIVAL_NOTE)) return -1;
        memset(w->actor.velocity, 0, sizeof w->actor.velocity);
        w->actor.field_24.word = w->actor.field_2c.word = w->actor.local_y.word = 0;
    }
    if (w->tower_placed) {
        if (frame.mode >= 8) {
            bfm_tower_field_tick(w->tower, w->cached, w->yaw, w->gameplay);
            if (bfm_tower_field_rise_phase(w->tower) == 3 &&
                !bfm_tower_ascent_floor_activate(bfm_tower_field_room(w->tower)))
                return -1;
            if (!bfm_tower_field_camera(w->tower, w->cached, &w->camera, w->eye)) return -1;
            TowerSubmission *submit = &w->submitted_tower[w->submit_slot];
            *submit = (TowerSubmission){.pose = w->pose, .camera = w->camera,
                .root = w->root, .visible = w->visible, .tick = w->tower_tick++};
            submit->blade.tick = w->tick;
            submit->has_blade = (w->visible & (UINT32_C(1) << 22)) &&
                bfm_forest_blade_transform(w->blade_anchors, &w->root, w->eye, &submit->blade.world);
            w->submit_slot = (w->submit_slot + 1) % 3;
        }
    } else if (frame.view == BFM_GATEWAY_FOREST && !follow(w)) {
        return -1;
    }
    ++w->tick;
    return 1;
}

static int step_controller(BfmForestWalk *w, int x, int y, uint16_t pressed,
                           uint16_t held, int stick_direction)
{
    int cross_pressed = (pressed & BFM_FOREST_CROSS) != 0;
    int cross_held = (held & BFM_FOREST_CROSS) != 0;
    if(!w || x < -1 || x > 1 || y < -1 || y > 1) return -1;
    if (!start_ambient(w)) return -1;
    /* The actor uses the previous completed controller poll. */
    int read_x = w->input_x, read_y = w->input_y;
    int jump_pressed = w->jump_pressed, jump_held = w->jump_held;
    w->input_x = x; w->input_y = y;
    /* The first poll returning control can carry the introduction's closing
     * Cross edge. Directions are live there, but a jump needs a fresh press. */
    w->jump_pressed = w->control_polled && cross_pressed;
    w->jump_held = cross_held;
    GunshotInput circle = w->gun_input;
    w->gun_input = (GunshotInput){w->control_polled && (pressed & BFM_FOREST_CIRCLE),
                                  (held & BFM_FOREST_CIRCLE) != 0, stick_direction};
    w->gunshot_fired = 0;
    int shoulder = w->shoulder_held;
    w->shoulder_held = (held & BFM_FOREST_R1) != 0;
    int triangle_pressed = w->triangle_pressed;
    w->triangle_pressed = w->control_polled && (pressed & BFM_FOREST_TRIANGLE);
    int square_held = w->square_held;
    w->square_held = w->control_polled && (held & BFM_FOREST_SQUARE);
    int attack_pressed = w->attack_pressed;
    w->attack_pressed = w->control_polled && (pressed & BFM_FOREST_SQUARE);
    w->swing_sound = 0;
    w->hit_sound = 0;
    w->control_polled = w->player_control;
    if (w->summit_transfer.phase != BFM_SUMMIT_TRANSFER_IDLE) {
        BfmSummitSkillInput input = {.shoulder = shoulder, .square_edge = attack_pressed,
            .circle_edge = circle.edge, .confirm_edge = cross_pressed,
            .reveal_edge = (pressed & BFM_FOREST_TRIANGLE) != 0};
        return step_summit_transfer(w, read_x, read_y, input, jump_pressed, jump_held);
    }
    if (w->gateway.active) return tick_gateway(w, read_x, read_y, attack_pressed, jump_pressed, jump_held, circle,
                                              cross_pressed, (pressed & BFM_FOREST_TRIANGLE) != 0);
    if (w->player_control) {
        int16_t before[3];
        memcpy(before, w->cached, sizeof before);
        int advanced = tick_player(w, read_x, read_y, jump_pressed, jump_held, attack_pressed,
                                   square_held, triangle_pressed, shoulder, circle, cross_pressed,
                                   (pressed & BFM_FOREST_TRIANGLE) != 0);
        RoomTransferDescriptor destination;
        uint16_t entry;
        if (advanced == 1 && bfm_forest_cross_gateway(w->field, before, w->cached, &destination, &entry)) {
            if (entry != 0 || !bfm_forest_gateway_transition_begin(&w->gateway, &destination))
                return -1;
        }
        return advanced;
    }
    if (w->tick >= LAST_WALK_TICK) {
        int result = tick_message(w, cross_pressed);
        if (result <= 0) return result;
        return bfm_forest_tick_shore_actor_with_run(w->field, w->cached, w->gameplay) ? result : -1;
    }
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
    return follow(w) && update_pose(w) &&
           bfm_forest_tick_shore_actor_with_run(w->field, w->cached, w->gameplay) ? 1 : -1;
}

/* The castle script owns this run and jump. Carry the same actor, equipment
 * and gameplay stream through the room's own entrance and exit. */
static int tick_drawbridge(BfmForestWalk *w)
{
    int stepped = bfm_castle_drawbridge_tick(w->drawbridge);
    if (stepped <= 0) return stepped;
    ++w->tick;
    int16_t position[3], heading;
    unsigned pose_tick;
    enum BfmDrawbridgeMotion motion;
    if (!bfm_castle_drawbridge_actor(w->drawbridge, position, &heading, &motion, &pose_tick))
        return 1;
    restore_whole_position(w, position);
    w->yaw = heading;
    w->actor.state = motion == BFM_DRAWBRIDGE_JUMP ? 2 : motion == BFM_DRAWBRIDGE_RUN;
    w->actor.phase = 1;
    w->support_y = w->support_z = 0;
    const uint32_t weapon_parts = UINT32_C(0x3f) << 21;
    uint32_t equipped = w->visible & weapon_parts;
    if (motion == BFM_DRAWBRIDGE_JUMP) {
        if (!w->jump_poses.visible &&
            !bfm_forest_jump_poses_load(w->disc, w->data.animation_table, &w->jump_poses))
            return -1;
        enum BfmForestJumpPhase phase = bfm_castle_drawbridge_descending(w->drawbridge)
            ? BFM_FOREST_JUMP_FALLING : BFM_FOREST_JUMP_RISING;
        uint32_t frame = bfm_forest_jump_pose_frame(&w->jump_poses, phase, pose_tick);
        if (!frame || !select_pose(w, frame, w->jump_poses.visible)) return -1;
    } else {
        w->motion = motion == BFM_DRAWBRIDGE_RUN ? RUN : IDLE;
        w->elapsed = pose_tick;
        if (!update_pose(w)) return -1;
    }
    w->visible = (w->visible & ~weapon_parts) | equipped;
    return 1;
}

int bfm_forest_walk_take_boss_request(BfmForestWalk *w, RoomTransferDescriptor *destination)
{
    return w && bfm_castle_drawbridge_take_boss_request(w->drawbridge, destination);
}

int bfm_forest_walk_tick_controller(BfmForestWalk *w, int x, int y, uint16_t pressed,
                                    uint16_t held, int stick_direction)
{
    if (w && w->drawbridge) return tick_drawbridge(w);
    int result = step_controller(w, x, y, pressed, held, stick_direction);
    RoomTransferDescriptor destination;
    if (result >= 0 && bfm_forest_walk_take_castle_request(w, &destination)) {
        w->drawbridge = bfm_castle_drawbridge_open(w->disc, &destination, w->gameplay);
        return w->drawbridge ? 1 : -1;
    }
    if (result == 1 && w->summit_transfer.phase == BFM_SUMMIT_TRANSFER_IDLE) {
        const RoomTransferDescriptor *exit = bfm_forest_walk_inner_exit(w);
        if (exit && !bfm_tower_summit_transfer_begin(&w->summit_transfer, exit)) return -1;
    }
    if (result > 0 && !w->tower_placed) bfm_forest_submit_atmosphere(w->field, &w->camera, w->eye);
    return result;
}

int bfm_forest_walk_tick_pad(BfmForestWalk *w, int x, int y, uint16_t pressed, uint16_t held)
{
    return bfm_forest_walk_tick_controller(w, x, y, pressed, held, 0);
}

static void field_palette(uint32_t pixels[240][320], unsigned shade)
{
    for (unsigned y = 0; y < 240; ++y) for (unsigned x = 0; x < 320; ++x) {
        uint32_t rgb = pixels[y][x], output = UINT32_C(0xff000000);
        for (unsigned shift = 0; shift <= 16; shift += 8) {
            unsigned level = (rgb >> shift & 255) >> 3;
            if (level > shade) output |= ((level - shade) * 255 / 31) << shift;
        }
        pixels[y][x] = output;
    }
}

int bfm_forest_walk_draw(BfmForestWalk *w,uint32_t pixels[240][320])
{
    if(!w || !pixels) return 0;
    for(unsigned y=0;y<240;y++) for(unsigned x=0;x<320;x++) pixels[y][x]=UINT32_C(0xff000000);
    if(!w->tick) return 1;
    if (w->drawbridge) {
        unsigned shade = bfm_castle_drawbridge_shade(w->drawbridge);
        if (shade >= 31) return 1;
        if (bfm_castle_drawbridge_uses_departure(w->drawbridge)) {
            if (!bfm_tower_summit_draw(w->summit, &w->camera, &w->pose, &w->root,
                                       w->visible, w->tick & 1u, NULL, pixels)) return 0;
        } else if (!bfm_castle_drawbridge_draw(w->drawbridge, &w->pose, &w->root,
                                               w->visible, pixels)) return 0;
        if (shade) field_palette(pixels, shade);
        return 1;
    }
    if (w->summit_transfer.phase != BFM_SUMMIT_TRANSFER_IDLE) {
        BfmTowerSummitFrame frame = bfm_tower_summit_transfer_frame(&w->summit_transfer);
        if (frame.shade >= 255) return 1;
        BfmForestBladeFrame blade = {.tick = w->tick};
        BfmForestTransform blade_root = w->root;
        if (frame.wide_camera) bfm_tower_summit_departure_hero(frame.beam_age, &blade_root);
        int has_blade = (w->visible & (UINT32_C(1) << 22)) &&
            bfm_forest_blade_transform(w->blade_anchors, &blade_root, w->eye, &blade.world);
        if (frame.show_destination) {
            if (!w->summit_placed || !bfm_tower_summit_draw(w->summit, &w->camera, &w->pose,
                    &w->root, w->visible, w->tick & 1u, has_blade ? &blade : NULL, pixels)) return 0;
        } else {
            if (!bfm_tower_interior_draw_lift(w->inner, &w->camera, &w->pose, &w->root,
                    w->visible, w->tick & 1u, has_blade ? &blade : NULL, &frame, pixels)) return 0;
        }
        if (frame.shade) field_palette(pixels, frame.shade >> 3);
        if (w->summit_free || (!frame.show_destination && frame.show_hud))
            bfm_forest_hud_draw(w->hud, &w->gameplay->player, pixels);
        return 1;
    }
    if (w->inner_age >= INNER_BLACK) {
        if (w->inner_age < INNER_CAMERA) return 1;
        BfmForestBladeFrame blade = {.tick = w->tick};
        int visible = (w->visible & (UINT32_C(1) << 22)) &&
            bfm_forest_blade_transform(w->blade_anchors, &w->root, w->eye, &blade.world);
        if (!bfm_tower_interior_draw(w->inner, &w->camera, &w->pose, &w->root, w->visible,
                                    w->tick & 1u, visible ? &blade : NULL, pixels)) return 0;
        if (w->inner_age < INNER_LIT) field_palette(pixels, (INNER_LIT - w->inner_age) * 2 - 1);
        if (w->inner_note.page) bfm_tower_interior_note_draw(&w->inner_note, w->message_art, pixels);
        if (w->inner_free) bfm_forest_hud_draw(w->hud, &w->gameplay->player, pixels);
        return 1;
    }
    BfmForestGatewayFrame gateway = bfm_forest_gateway_transition_frame(&w->gateway);
    if (gateway.view == BFM_GATEWAY_BLACK) return 1;
    if (gateway.view == BFM_GATEWAY_TOWER) {
        /* Physics owns the current root; present the previously submitted
         * camera, model and figure pose for the displayed room update. */
        const TowerSubmission *submitted = &w->submitted_tower[w->tower_tick < 3 ? 0 : w->submit_slot];
        if (!bfm_tower_field_draw(w->tower, &submitted->camera, &submitted->pose,
                &submitted->root, submitted->visible, submitted->tick,
                submitted->has_blade ? &submitted->blade : NULL, pixels)) return 0;
        if (w->inner_age > INNER_WIPE) {
            BfmForestGatewayFrame fade = {.view = BFM_GATEWAY_HELD_FOREST,
                                           .shade = (w->inner_age - INNER_WIPE) / 2};
            bfm_forest_gateway_transition_shade(&fade, pixels);
        } else {
            field_palette(pixels, gateway.shade);
        }
        if (w->tower_note)
            bfm_forest_message_art_draw(w->message_art, bfm_forest_message_frame(w->tower_note), pixels);
        return 1;
    }
    if (gateway.view == BFM_GATEWAY_HELD_FOREST && w->held_forest) {
        memcpy(pixels, w->held_forest, 240 * sizeof *pixels);
        bfm_forest_gateway_transition_shade(&gateway, pixels);
        return 1;
    }
    unsigned field_tick = gateway.view == BFM_GATEWAY_HELD_FOREST ? w->last_forest_tick : w->tick;
    BfmForestBladeFrame blade = {.tick = field_tick};
    int delayed = w->displayed_body.pose != 0;
    const BfmForestTransform *root = delayed ? &w->displayed_body.root : &w->root;
    const BfmForestPose *pose = delayed ? &w->displayed_pose : &w->pose;
    const int8_t (*anchors)[3] = delayed ? w->displayed_anchors : w->blade_anchors;
    uint32_t visible = delayed ? w->displayed_body.visible : w->visible;
    if (w->hide_recovering_body) visible = 0;
    int has_blade = !bfm_forest_lesson_blade_away(&w->fusion) &&
                    bfm_forest_blade_transform(anchors, root, w->eye, &blade.world);
    if (!bfm_forest_draw_entrance_player(w->field, &w->camera, pose, root,
                                         visible, field_tick % 2, has_blade ? &blade : NULL, pixels)) return 0;
    field_palette(pixels, w->tick < 16 ? (255 - w->tick * 16) / 8 : 0);
    if (w->player_control && gateway.live) bfm_forest_hud_draw(w->hud, &w->gameplay->player, pixels);
    if (w->lesson.page)
        bfm_forest_message_art_draw(w->message_art, bfm_forest_message_frame(w->lesson.page), pixels);
    else if (w->message && !w->player_control)
        bfm_forest_message_art_draw(w->message_art, bfm_forest_message_frame(w->message), pixels);
    if (gateway.view == BFM_GATEWAY_HELD_FOREST) {
        w->held_forest = malloc(240 * sizeof *pixels);
        if (!w->held_forest) return 0;
        memcpy(w->held_forest, pixels, 240 * sizeof *pixels);
    }
    bfm_forest_gateway_transition_shade(&gateway, pixels);
    return 1;
}
