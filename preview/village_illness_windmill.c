/* SPDX-License-Identifier: MIT */
#include "village_illness_windmill.h"
#include "forest_pose.h"
#include "../pc_port/forest_player_bytes.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

enum { IDLE, WALK, SPEAK, CLIPS };
typedef enum { CLOSED, OPENING, OPEN, CLOSING } Door;
typedef struct {
    uint32_t program;
    const uint8_t *keys;
    unsigned count;
} Clip;

struct BfmVillageIllnessWindmill {
    const uint8_t *scene, *home, *hinge, *shadow;
    size_t bytes;
    MusashiForestPlayerMesh *mesh;
    Clip clips[CLIPS];
    BfmBedroomRoot root;
    int fraction[2];
    unsigned clip, key, turn_goal, turn_left, walk_left;
    int speed, visible, active, answers;
    Door door;
    unsigned door_age, sound;
    int angle;
};

static const uint32_t base = UINT32_C(0x80128158);
static const uint32_t room = UINT32_C(0x80190780);

static const uint8_t *mounted(const BfmVillageIllnessWindmill *w, uint32_t address, size_t span)
{
    if (address < base || span > w->bytes || address-base > w->bytes-span) return NULL;
    return w->scene + (address-base);
}

static int load_clip(const BfmVillageIllnessWindmill *w, Clip *clip, uint32_t address)
{
    const uint8_t *keys = mounted(w,address,8);
    if (!keys) return 0;
    size_t entries = (w->bytes - (size_t)(keys-w->scene)) / 8;
    for (size_t i = 0; i < entries; ++i) {
        uint32_t pose = u32le(keys+8*i), duration = u32le(keys+8*i+4);
        if (!pose) {
            if (!i || duration != UINT16_MAX) return 0;
            *clip = (Clip){address,keys,(unsigned)i};
            return 1;
        }
        if ((pose >> 24) != 0x81 || (pose & 3u) || duration != 1 ||
            !mounted(w,pose & ~UINT32_C(0x01000000),10*8)) return 0;
    }
    return 0;
}

static int at_home(const BfmSaveRecord *save)
{
    unsigned hour = save->progress[0x39];
    return hour < 11 || hour >= 23;
}

static void indoors(BfmVillageIllnessWindmill *w)
{
    for (unsigned k = 0; k < 3; ++k) w->root.xyz[k] = s16le(w->home+2*k);
    w->root.yaw = u16le(w->home+6);
    w->visible = 0;
    w->clip = IDLE;
    w->key = w->walk_left = w->turn_left = 0;
    w->fraction[0] = w->fraction[1] = 0;
}

BfmVillageIllnessWindmill *bfm_village_illness_windmill_open(const uint8_t *scene,
    size_t bytes, const BfmSaveRecord *save)
{
    if (!scene || bytes > BFM_FIELD_SCENE_CAPACITY || !save) return NULL;
    BfmVillageIllnessWindmill *w = calloc(1,sizeof *w);
    if (!w) return NULL;
    w->scene = scene;
    w->bytes = bytes;
    w->home = mounted(w,UINT32_C(0x8018f038),8);
    const uint8_t *kit = mounted(w,UINT32_C(0x8018f040),20);
    w->shadow = mounted(w,UINT32_C(0x8018d748),12);
    const uint8_t *desc = mounted(w,room,0xbc);
    if (!w->home || !kit || !w->shadow || !desc || u16le(kit+16) != 23 ||
        u16le(kit+18) != 16 || u16le(w->home+6) >= 4096) goto fail;
    uint32_t first = u32le(desc+0xb4), end = u32le(desc+0xb8);
    if (end < first || (end-first)%20 || !mounted(w,first,end-first)) goto fail;
    unsigned residents = 0;
    for (uint32_t ptr = first; ptr < end; ptr += 20) {
        const uint8_t *row = mounted(w,ptr,20);
        unsigned kind = u16le(row+6);
        if (!kind) break;
        if (kind == 380) ++residents;
        if (kind == 97 && u16le(row+8) == 16) {
            if (w->hinge) goto fail;
            w->hinge = row;
        }
    }
    if (residents != 1 || !w->hinge || u16le(w->hinge+12) != 0) goto fail;
    unsigned parts;
    w->mesh = musashi_forest_player_mesh_from_table(scene,bytes,base,u32le(kit),&parts);
    if (!w->mesh || parts != 10) goto fail;
    for (unsigned i = 0; i < CLIPS; ++i)
        if (!load_clip(w,&w->clips[i],u32le(kit+4+4*i))) goto fail;
    w->answers = at_home(save);
    indoors(w);
    return w;
fail:
    bfm_village_illness_windmill_free(w);
    return NULL;
}

void bfm_village_illness_windmill_free(BfmVillageIllnessWindmill *w)
{
    if (w) {
        musashi_forest_player_mesh_free(w->mesh);
        free(w);
    }
}

int bfm_village_illness_windmill_ready(const BfmVillageIllnessWindmill *w,
    const BfmSaveRecord *save, const int16_t xyz[3])
{
    if (!w || !save || !xyz || !w->answers || w->active || w->door != CLOSED) return 0;
    unsigned story = u32le(save->progress+8);
    int x = xyz[0]-s16le(w->hinge), z = xyz[2]-s16le(w->hinge+4);
    return story >= 200 && story < 230 && x >= -192 && x < 64 && z >= -96 && z < 0;
}

/* Building 16 uses a high actor slot. It runs after Wid and sees the
 * request he posts this field. The closing leaf first holds at the open
 * stop during the eight-field close timer. */
static void advance_door(BfmVillageIllnessWindmill *w, unsigned request)
{
    w->sound = 0;
    if (request == 1 && w->door != OPENING && w->door != OPEN) {
        w->door = OPENING;
        w->door_age = 0;
    } else if (request == 2 && w->door != CLOSING && w->door != CLOSED) {
        w->door = CLOSING;
        w->door_age = 0;
    }
    if (w->door != OPENING && w->door != CLOSING) return;
    unsigned age = ++w->door_age;
    if (w->door == CLOSING && age <= 7) return;
    unsigned steps = w->door == OPENING ? age : age-7;
    int distance = steps < 39 ? (int)(steps*(steps+1)) : 1536;
    if (w->door == OPENING) {
        w->angle = -distance;
        if (age == 8) w->sound = 1300;
        if (distance == 1536) w->door = OPEN;
    } else {
        w->angle = distance - 1536;
        if (distance == 1536) {
            w->door = CLOSED;
            w->sound = 1301;
        }
    }
}

static void turn_to_goal(BfmVillageIllnessWindmill *w)
{
    if (!--w->turn_left) {
        w->root.yaw = w->turn_goal;
        return;
    }
    int delta = (int)w->turn_goal - (int)w->root.yaw;
    if (delta > 2048) delta -= 4096;
    if (delta < -2048) delta += 4096;
    int step = delta/6;
    if (!step) step = (delta>0) - (delta<0);
    w->root.yaw = (w->root.yaw+step) & 4095u;
}

static int advance_motion(BfmVillageIllnessWindmill *w)
{
    if (!w->turn_left && !w->walk_left) return 1;
    if (w->turn_left) turn_to_goal(w);
    else {
        BfmForestTransform matrix;
        bfm_forest_actor_root(w->root.yaw,w->root.xyz,&matrix);
        for (unsigned j = 0; j < 2; ++j) {
            unsigned axis = j*2;
            int scaled = w->speed*matrix.rotation[axis][2];
            int delta = w->fraction[j] + scaled/16 - (scaled%16 < 0);
            int units = delta/65536 - (delta%65536 < 0);
            int next = w->root.xyz[axis] + units;
            if (next < INT16_MIN || next > INT16_MAX) return 0;
            w->root.xyz[axis] = (int16_t)next;
            w->fraction[j] = delta - units*65536;
        }
        --w->walk_left;
    }
    /* The resident lifts the foot eight units before its downward probe.
     * The shared shadow probe starts twenty units above its input instead
     * of four, so supply the equivalent starting point. */
    BfmBedroomRoot probe = w->root;
    if (probe.xyz[1] > INT16_MAX-8 || probe.xyz[1] < INT16_MIN+8) return 0;
    probe.xyz[1] += 8;
    if (!bfm_bedroom_travel_air_shadow(w->scene,w->bytes,room,&probe)) return 0;
    w->root.shadow_visible = probe.shadow_visible;
    if (probe.shadow_visible) {
        memcpy(w->root.shadow_xyz,probe.shadow_xyz,sizeof probe.shadow_xyz);
        memcpy(w->root.shadow_normal,probe.shadow_normal,sizeof probe.shadow_normal);
        w->root.xyz[1] = probe.shadow_xyz[1];
        w->fraction[0] = w->fraction[1] = 0;
    } else w->root.xyz[1] -= 8;
    return 1;
}

int bfm_village_illness_windmill_step(BfmVillageIllnessWindmill *w,
    const BfmSaveRecord *save, int event_active, const BfmVillageWindmillCue *cue)
{
    if (!w || !save) return 0;
    advance_door(w,cue ? cue->door : 0);
    w->key = (w->key+1) % w->clips[w->clip].count;
    if (!advance_motion(w)) return 0;
    if (!event_active) {
        if (w->active) {
            indoors(w);
            w->answers = 1;
        } else w->answers = at_home(save);
    }
    w->active = event_active;
    if (!cue) return 1;
    if (cue->door > 2 || cue->show < -1 || cue->show > 1 ||
        cue->walk_q8 < INT16_MIN || cue->walk_q8 > INT16_MAX ||
        cue->walk_fields > UINT16_MAX || cue->turn_fields > UINT16_MAX ||
        (cue->walk_fields && cue->turn_fields)) return 0;
    if (cue->show) {
        w->visible = cue->show > 0;
        if (w->visible && !bfm_bedroom_travel_air_shadow(w->scene,w->bytes,room,&w->root)) return 0;
    }
    if (cue->program) {
        unsigned clip = 0;
        while (clip < CLIPS && w->clips[clip].program != cue->program) ++clip;
        if (clip == CLIPS) return 0;
        w->clip = clip;
        w->key = 0;
    }
    if (cue->turn_fields) {
        w->turn_goal = (w->root.yaw+(unsigned)cue->turn) & 4095u;
        w->turn_left = 30;
        w->walk_left = 0;
    }
    if (cue->walk_fields) {
        w->speed = cue->walk_q8;
        w->walk_left = cue->walk_fields;
        w->turn_left = 0;
    }
    return 1;
}

const int16_t *bfm_village_illness_windmill_position(const BfmVillageIllnessWindmill *w)
{
    return w ? w->root.xyz : NULL;
}

int bfm_village_illness_windmill_add(const BfmVillageIllnessWindmill *w,
    BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    if (!w || !figure || !view) return 0;
    if (!w->visible) return 1;
    uint32_t pose_addr = u32le(w->clips[w->clip].keys+8*w->key) & ~UINT32_C(0x01000000);
    BfmForestPose pose;
    if (!bfm_forest_actor_pose_decode(mounted(w,pose_addr,80),80,10,&pose)) return 0;
    BfmForestTransform root;
    bfm_forest_actor_root(w->root.yaw,w->root.xyz,&root);
    const BfmForestActor actor = {.root=&root,.pose=&pose,.mesh=w->mesh,.visible_parts=(1u<<10)-1};
    return bfm_bedroom_figure_add_actor(figure,view,&actor,&w->root,w->shadow,0x3000);
}

unsigned bfm_village_illness_windmill_door_sound(const BfmVillageIllnessWindmill *w)
{
    return w ? w->sound : 0;
}

int bfm_village_illness_windmill_closed(const BfmVillageIllnessWindmill *w)
{
    return w && w->door == CLOSED;
}

int bfm_village_illness_windmill_hinged(const BfmVillageIllnessWindmill *w)
{
    return w && w->door != CLOSED;
}

int bfm_village_illness_windmill_door_angle(const BfmVillageIllnessWindmill *w)
{
    return w ? w->angle : 0;
}
