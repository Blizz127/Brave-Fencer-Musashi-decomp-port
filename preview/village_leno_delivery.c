/* SPDX-License-Identifier: MIT */
#include "village_leno_delivery.h"
#include "forest_pose.h"
#include "village_ground.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>
#include <string.h>

enum { PARTS = 12, POSES = 20, SPEAK_POSES = 20, DOG_WAIT = 6, DOG_RUN = 80, Q = 65536 };
static const uint32_t origin = UINT32_C(0x80128158), upper = UINT32_C(0x8018c794);

typedef enum { READY, WATCH, READING, AFTER_MESSAGE, CLOSING, DONE } Stage;

struct BfmVillageLenoDelivery {
    BfmSaveRecord *save;
    BfmBedroomTravel *travel;
    BfmVillageGround *ground;
    MusashiForestPlayerMesh *mesh;
    const uint8_t *keys[POSES], *shadow, *message, *speaking[SPEAK_POSES];
    BfmBedroomRoot dog;
    int32_t velocity[3];
    unsigned fraction[3], dog_age, pose;
    unsigned watch_age, turn_delay, speak_delay, end_delay, final_yaw;
    int16_t goal_x, goal_z;
    Stage stage;
    int turn_due;
    unsigned speaking_age;
};

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t length)
{
    if (!scene || address < origin || length > bytes || address - origin > bytes - length)
        return NULL;
    return scene + (address - origin);
}

static int flag(const BfmSaveRecord *save, unsigned bit)
{
    return !!(save->flags[bit / 8] & (1u << (bit % 8)));
}

static int bringing_dog(const BfmSaveRecord *save, int carrying)
{
    return carrying && u32le(save->progress + 8) < 20 && flag(save, 0xdf) &&
           (u16le(save->variables + 10) & 0x40);
}

int bfm_village_leno_delivery_pending(const BfmSaveRecord *save, int carrying)
{
    return save && (bringing_dog(save, carrying) || (u32le(save->progress + 8) == 20 && !flag(save, 0x10b)));
}

static int read_speech(BfmVillageLenoDelivery *d, const uint8_t *scene, size_t bytes)
{
    const uint8_t *script = mounted(scene, bytes, UINT32_C(0x80189d4c), 88);
    if (!script || u32le(script) != 0x4a || u32le(script+8) != 3 ||
        u32le(script+16) != 0x3f || u32le(script+24) != 8 || u32le(script+32) != 0x3f ||
        u32le(script+40) != 0x39 || u32le(script+48) != 0x3a || u32le(script+56) != 0x0b ||
        u32le(script+64) != 1 || u32le(script+72) != 0x3f || u32le(script+80) != 2)
        return 0;
    d->goal_x = s16le(script+12);
    d->goal_z = s16le(script+14);
    d->turn_delay = u32le(script+20);
    d->final_yaw = u32le(script+28);
    d->speak_delay = u32le(script+36);
    d->end_delay = u32le(script+76);
    d->message = mounted(scene, bytes, u32le(script+68), 132);
    return d->final_yaw < 4096 && d->turn_delay && d->turn_delay <= 255 &&
           d->speak_delay && d->speak_delay <= 255 && d->end_delay && d->end_delay <= 255 &&
           d->message && d->message[0] == 5 && d->message[130] == 3 && d->message[131] == 10;
}

/* The field script requests the resident talking loop as the window opens.
 * The program selects twenty successive body keys before jumping back to
 * the face selection. Keep every 256-byte pose in the mounted player bank. */
static int read_speaking_pose(BfmVillageLenoDelivery *d, const uint8_t *main, size_t size)
{
    const uint32_t base = UINT32_C(0x800cedf8);
    size_t program_offset = UINT32_C(0x800d5a34) - base;
    if (!main || program_offset > size || size - program_offset < (5+SPEAK_POSES+2)*4) return 0;
    const uint8_t *program = main + program_offset;
    if (u32le(program) != 4 || u32le(program+8) != 19 || u32le(program+12) != 15 ||
        u32le(program+16) != 0 || u32le(program+(5+SPEAK_POSES)*4) != 6 ||
        u32le(program+(6+SPEAK_POSES)*4) != 3) return 0;
    for (unsigned i = 0; i < SPEAK_POSES; ++i) {
        uint32_t key = u32le(program+(5+i)*4);
        if (key < base || key-base > size || size-(key-base) < 256) return 0;
        d->speaking[i] = main + (key-base);
    }
    return 1;
}

/* Integrate the field's fixed-point velocity while keeping the whole part
 * available to the normal village ground and wall sweep. */
static void move_dog(BfmVillageLenoDelivery *d)
{
    int16_t to[3];
    unsigned fraction[3];
    for (unsigned i = 0; i < 3; ++i) {
        int32_t sum = (int32_t)d->fraction[i] + d->velocity[i];
        int step = sum / Q - (sum % Q < 0);
        to[i] = (int16_t)(d->dog.xyz[i] + step);
        fraction[i] = (unsigned)(sum - step * Q);
    }
    BfmForestContact footing;
    BfmVillageGroundResult result = bfm_village_ground_step(d->ground, d->dog.xyz, to, &footing);
    if (result == BFM_VILLAGE_GROUND_BLOCKED) return;
    memcpy(d->fraction, fraction, sizeof fraction);
    memcpy(d->dog.xyz, to, sizeof to);
    d->dog.shadow_visible = result == BFM_VILLAGE_GROUND_FOOTING;
    if (d->dog.shadow_visible) {
        memcpy(d->dog.shadow_xyz, to, sizeof to);
        memcpy(d->dog.shadow_normal, footing.normal, sizeof footing.normal);
    }
}

static int load_dog(BfmVillageLenoDelivery *d, const uint8_t *scene, size_t bytes,
                    const BfmBedroomRoot *arrival)
{
    const uint8_t *profile = mounted(scene, bytes, UINT32_C(0x801892c8), 52);
    const uint8_t *stride = mounted(scene, bytes, UINT32_C(0x801892fc), 12);
    const uint8_t *loop = mounted(scene, bytes, UINT32_C(0x801a7f4c), (POSES+1)*8);
    if (!profile || !stride || !loop || u32le(loop+POSES*8) || u32le(loop+POSES*8+4) != 0xffff)
        return 0;
    unsigned parts;
    d->mesh = musashi_forest_player_mesh_from_table(scene, bytes, origin, u32le(profile+4), &parts);
    d->ground = bfm_village_ground_open(scene, bytes, upper);
    d->shadow = mounted(scene, bytes, UINT32_C(0x80186e14), 12);
    if (!d->mesh || parts != PARTS || !d->ground || !d->shadow) return 0;
    for (unsigned i = 0; i < POSES; ++i) {
        uint32_t key = u32le(loop+i*8);
        if ((key >> 24) != 0x81 || u32le(loop+i*8+4) != 1) return 0;
        d->keys[i] = mounted(scene, bytes, key & ~UINT32_C(0x01000000), PARTS*8);
        if (!d->keys[i]) return 0;
    }
    if (u32le(stride) || u32le(stride+4) || (int32_t)u32le(stride+8) != -16*Q) return 0;
    d->dog = *arrival;
    d->dog.yaw = 0xf00;
    BfmForestTransform heading;
    bfm_forest_actor_root(d->dog.yaw, arrival->xyz, &heading);
    for (unsigned i = 0; i < 3; ++i)
        d->velocity[i] = (int32_t)((int64_t)heading.rotation[i][2] * (int32_t)u32le(stride+8) / 4096);
    move_dog(d);
    return 1;
}

BfmVillageLenoDelivery *bfm_village_leno_delivery_open(const uint8_t *scene, size_t bytes,
    const uint8_t *main, size_t main_bytes, BfmSaveRecord *save, int carrying,
    const BfmBedroomRoot *arrival)
{
    if (!arrival || !bfm_village_leno_delivery_pending(save, carrying)) return NULL;
    const uint8_t *room = mounted(scene, bytes, upper, 0xcc);
    if (!room) return NULL;
    const uint8_t *actors = mounted(scene, bytes, u32le(room+0xb4), 18*20);
    if (!actors || u16le(actors+4*20+6) != 318 || u16le(actors+17*20+6) != 631) return NULL;
    BfmVillageLenoDelivery *d = calloc(1, sizeof *d);
    if (!d) return NULL;
    d->save = save;
    if (flag(save, 0x10b)) d->stage = DONE;
    else {
        d->travel = bfm_bedroom_travel_from_root(scene, bytes, upper, arrival);
        if (!d->travel || !read_speech(d, scene, bytes) || !read_speaking_pose(d, main, main_bytes)) goto fail;
    }
    if (bringing_dog(save, carrying)) {
        if (!load_dog(d, scene, bytes, arrival)) goto fail;
        save->variables[10] = 0x11;
        save->variables[11] = 0x10;
        save->progress[8] = 20;
        save->flags[0xe1/8] |= 1u << (0xe1%8);
    }
    return d;
fail:
    bfm_village_leno_delivery_free(d);
    return NULL;
}

void bfm_village_leno_delivery_free(BfmVillageLenoDelivery *d)
{
    if (!d) return;
    musashi_forest_player_mesh_free(d->mesh);
    bfm_village_ground_free(d->ground);
    bfm_bedroom_travel_free(d->travel);
    free(d);
}

int bfm_village_leno_delivery_active(const BfmVillageLenoDelivery *d)
{
    return d && d->stage != DONE && d->stage != READY;
}

int bfm_village_leno_delivery_message_closed(BfmVillageLenoDelivery *d)
{
    if (!d || d->stage != READING) return 0;
    d->stage = AFTER_MESSAGE;
    return 1;
}

int bfm_village_leno_delivery_step(BfmVillageLenoDelivery *d, BfmVillageMotion *motion,
    const BfmVillageCamera *camera, int player_ready, BfmForestMessage **opened)
{
    if (!d) return 1;
    if (d->mesh && d->dog_age < DOG_WAIT + DOG_RUN) {
        ++d->dog_age;
        d->pose = d->dog_age % POSES;
        if (d->dog_age > DOG_WAIT) move_dog(d);
    }
    if (d->stage == DONE || (d->stage == READY && !player_ready)) return 1;
    if (!motion || !camera) return 0;
    if (d->stage == READING || d->stage == AFTER_MESSAGE || d->stage == CLOSING)
        d->speaking_age = (d->speaking_age + 1) % SPEAK_POSES;
    if (d->stage == READY) {
        if (!bfm_bedroom_travel_walk(d->travel, d->goal_x, d->goal_z, 0)) return 0;
        d->stage = WATCH;
    } else if (d->stage == WATCH) {
        ++d->watch_age;
        if (d->watch_age == d->turn_delay) d->turn_due = 1;
        if (d->watch_age == d->turn_delay + d->speak_delay) {
            if (!opened || *opened) return 0;
            *opened = bfm_field_message_borrow(d->message, 132);
            if (!*opened) return 0;
            /* The actor submits the selected key, then its first ordinary animation tick. */
            d->speaking_age = 1;
            d->stage = READING;
        }
    } else if (d->stage == AFTER_MESSAGE) {
        d->stage = CLOSING;
    } else if (d->stage == CLOSING && !--d->end_delay) {
        if (!bfm_village_motion_place(motion, bfm_bedroom_travel_root(d->travel))) return 0;
        d->save->flags[0x10b/8] |= 1u << (0x10b%8);
        d->stage = DONE;
        return 2;
    }
    if (d->turn_due && !bfm_bedroom_travel_root(d->travel)->busy) {
        if (!bfm_bedroom_travel_turn(d->travel, d->final_yaw, 0)) return 0;
        d->turn_due = 0;
    }
    if (!bfm_bedroom_travel_step(d->travel, camera->eye, camera->target) ||
        !bfm_village_motion_staged(motion, bfm_bedroom_travel_root(d->travel))) return 0;
    return 2;
}

void bfm_village_leno_delivery_pose(const BfmVillageLenoDelivery *d, BfmBedroomPose *frame)
{
    if (!d || !frame || (d->stage != READING && d->stage != AFTER_MESSAGE && d->stage != CLOSING)) return;
    frame->pose = d->speaking[d->speaking_age];
    frame->show_lumina = 0;
    frame->draw_current = 1;
}

int bfm_village_leno_delivery_add(const BfmVillageLenoDelivery *d, BfmBedroomFigure *figure,
                                  const BfmBedroomView *view)
{
    if (!d || !d->mesh || d->dog_age >= DOG_WAIT+DOG_RUN) return 1;
    BfmForestTransform placement;
    BfmForestPose pose;
    bfm_forest_actor_root(d->dog.yaw, d->dog.xyz, &placement);
    if (!bfm_forest_actor_pose_decode(d->keys[d->pose], PARTS*8, PARTS, &pose)) return 0;
    BfmForestActor actor = {.mesh=d->mesh, .pose=&pose, .root=&placement, .visible_parts=(1u<<PARTS)-1};
    return bfm_bedroom_figure_add_actor(figure, view, &actor,
                                        &d->dog, d->shadow, 8192);
}
