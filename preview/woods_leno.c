/* SPDX-License-Identifier: MIT */
#include "woods_leno.h"
#include "field_waypoint.h"
#include "forest_guard_contact.h"
#include "forest_lift.h"
#include "village_talk_camera.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>
#include <string.h>

enum DogState { ABSENT, SEEK, CHEW, REST, CARRIED, FLIGHT, SINK, HURT_IN_WATER };
enum GreetingShot { NO_SHOT, SAVE_ENTRY, BEGIN_SHOT, SHOW_SHOT };
enum { UNIT = 65536, PAGES = 3, DOG_VARIABLE = 10 };

struct BfmWoodsLeno {
    const BfmWoodsLenoActor *art;
    const BfmVillageGround *ground;
    BfmSaveRecord *save;
    BfmWoodsLenoView view;
    BfmForestGuardBox body;
    int16_t home[3], bone[3], launch[3];
    int home_yaw;
    enum DogState state;
    enum GreetingShot shot;
    BfmVillageTalkCamera talk_camera;
    BfmVillageCamera entry_camera;
    int16_t local_camera[2][3];
    int close_eye[3], close_target[3];
    const uint8_t *text[PAGES];
    size_t length[PAGES];
    unsigned delay, wait, page, hold, blink, age, bounce, settling, release_timer;
    unsigned recoil, hit_yaw;
    int greeting, greeted, window, eligible, chewing;
    int32_t velocity[3], fraction[3];
};

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t length)
{
    const uint32_t origin = UINT32_C(0x80128158);
    if (address < origin || address - origin > bytes || length > bytes - (address - origin))
        return NULL;
    return scene + (address - origin);
}

static int read_resources(BfmWoodsLeno *dog, const uint8_t *scene, size_t bytes)
{
    const uint8_t *entry = mounted(scene, bytes, UINT32_C(0x8018b7e4), 12);
    const uint8_t *shape = entry ? mounted(scene, bytes, u32le(entry + 8), 16) : NULL;
    const uint8_t *program = mounted(scene, bytes, UINT32_C(0x8018b878), 72);
    if (!shape || u32le(shape) || !program || u32le(program + 16) != 15) return 0;
    for (unsigned k = 0; k < 3; ++k) {
        dog->body.low[k] = s16le(shape + 4 + 4*k);
        dog->body.high[k] = s16le(shape + 6 + 4*k);
        if (dog->body.low[k] > dog->body.high[k]) return 0;
    }
    /* The conversation selects a pair of actor-local points from the
     * common framing presets. The script releases that shot after page 3. */
    unsigned preset = u32le(program + 12);
    if (u32le(program + 8) != 30 || preset >= 11 || u32le(program + 56) != 32) return 0;
    const uint8_t *pair = mounted(scene, bytes, UINT32_C(0x8018b150) + 16*preset, 16);
    if (!pair) return 0;
    for (unsigned i = 0; i < 2; ++i)
        for (unsigned axis = 0; axis < 3; ++axis)
            dog->local_camera[i][axis] = s16le(pair + 8*i + 2*axis);
    const uint8_t *throw_vector = mounted(scene, bytes, UINT32_C(0x8018b8cc), 12);
    if (!throw_vector) return 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        if (u16le(throw_vector + 4*axis)) return 0;
        dog->launch[axis] = s16le(throw_vector + 4*axis + 2);
    }
    dog->delay = u32le(program + 20);
    for (unsigned i = 0; i < PAGES; ++i) {
        const uint8_t *line = program + 32 + i*8;
        uint32_t begin = u32le(line + 4);
        uint32_t end = i+1 == PAGES ? UINT32_C(0x8018ed40) : u32le(line + 12);
        if (u32le(line) != 1 || end <= begin) return 0;
        dog->length[i] = end - begin;
        dog->text[i] = mounted(scene, bytes, begin, dog->length[i]);
        if (!dog->text[i]) return 0;
    }
    return 1;
}

static int near(const int16_t a[3], const int16_t b[3], int radius, int inclusive)
{
    int64_t squared = 0;
    for (unsigned i = 0; i < 3; ++i) {
        int64_t d = a[i] - b[i];
        squared += d*d;
    }
    return inclusive ? squared <= radius*radius : squared < radius*radius;
}

static void face(BfmWoodsLeno *dog, const int16_t point[3])
{
    int dx = dog->view.xyz[0] - point[0], dz = dog->view.xyz[2] - point[2];
    if (dx || dz) dog->view.yaw = bfm_waypoint_heading(dx, dz);
}

static void select_clip(BfmWoodsLeno *dog, BfmWoodsLenoClip clip)
{
    dog->view.clip = clip;
    dog->view.tick = 0;
}

static void write_location(BfmWoodsLeno *dog, int carried)
{
    dog->save->variables[DOG_VARIABLE] = carried ? 0x65 : 0x25;
    dog->save->variables[DOG_VARIABLE + 1] = 0x30;
}

static void find_bone(BfmWoodsLeno *dog)
{
    dog->view.pitch = 0;
    if (near(dog->view.xyz, dog->bone, 128, 0)) {
        face(dog, dog->bone);
        dog->state = SEEK;
        select_clip(dog, BFM_LENO_APPROACH);
    } else {
        dog->state = REST;
        select_clip(dog, BFM_LENO_REST);
    }
}

BfmWoodsLeno *bfm_woods_leno_open(const uint8_t *scene, size_t bytes, BfmSaveRecord *save,
                                  const BfmVillageGround *ground, const BfmWoodsLenoActor *art)
{
    if (!scene || !save || !ground || !art) return NULL;
    BfmWoodsLeno *dog = calloc(1, sizeof *dog);
    if (!dog) return NULL;
    dog->art = art;
    dog->ground = ground;
    dog->save = save;
    if (u32le(save->progress + 8) >= 20 || !(save->flags[223/8] & (1u << (223%8))))
        return dog;
    if (!bfm_woods_leno_actor_home(art, dog->home, dog->bone, &dog->home_yaw) ||
        !read_resources(dog, scene, bytes)) {
        free(dog);
        return NULL;
    }
    memcpy(dog->view.xyz, dog->home, sizeof dog->home);
    dog->view.yaw = dog->home_yaw;
    dog->view.visible = 1;
    write_location(dog, 0);
    find_bone(dog);
    return dog;
}

void bfm_woods_leno_free(BfmWoodsLeno *dog)
{
    free(dog);
}

const BfmWoodsLenoView *bfm_woods_leno_view(const BfmWoodsLeno *dog)
{
    return dog && dog->state != ABSENT ? &dog->view : NULL;
}

int bfm_woods_leno_held(const BfmWoodsLeno *dog)
{
    return dog && dog->state == CARRIED;
}

int bfm_woods_leno_in_dialogue(const BfmWoodsLeno *dog)
{
    return dog && dog->greeting;
}

int bfm_woods_leno_speaking(const BfmWoodsLeno *dog)
{
    return dog && dog->greeting && (dog->window || dog->page);
}

static void frame_greeting(BfmWoodsLeno *dog)
{
    BfmForestTransform root;
    int16_t target[3], eye[3];
    bfm_forest_actor_pitch_yaw(dog->view.pitch, dog->view.yaw, dog->view.xyz, &root);
    bfm_forest_lift_world(&root, dog->local_camera[0], target);
    bfm_forest_lift_world(&root, dog->local_camera[1], eye);
    for (unsigned axis = 0; axis < 3; ++axis) {
        dog->close_target[axis] = target[axis];
        dog->close_eye[axis] = eye[axis];
    }
    dog->shot = SAVE_ENTRY;
}

int bfm_woods_leno_camera_step(BfmWoodsLeno *dog, const BfmVillageCamera *normal,
                               BfmVillageCamera *presented)
{
    if (!normal || !presented) return 0;
    *presented = *normal;
    if (!dog) return 0;
    switch (dog->shot) {
    case SAVE_ENTRY:
        dog->entry_camera = *normal;
        dog->shot = BEGIN_SHOT;
        break;
    case BEGIN_SHOT:
        /* One ordinary camera update separates the saved return framing
         * from the start of the authored view. */
        bfm_village_talk_camera_world(&dog->talk_camera, dog->close_eye, dog->close_target,
                                      &dog->entry_camera, normal);
        dog->shot = SHOW_SHOT;
        break;
    case SHOW_SHOT:
        bfm_village_talk_camera_step(&dog->talk_camera, presented);
        if (!dog->talk_camera.phase) {
            dog->shot = NO_SHOT;
            return 1;
        }
        break;
    case NO_SHOT: break;
    }
    return 0;
}

int bfm_woods_leno_random_due(const BfmWoodsLeno *dog)
{
    return dog && !dog->recoil && dog->state == REST &&
           dog->view.tick + 1 >= bfm_woods_leno_actor_duration(dog->art, dog->view.clip);
}

static int solid(const BfmWoodsLeno *dog)
{
    return dog && (dog->state == SEEK || dog->state == CHEW || dog->state == REST);
}

int bfm_woods_leno_brace(const BfmWoodsLeno *dog, const int16_t from[3], const int16_t to[3],
                         int16_t contact[3])
{
    if (!solid(dog) || !contact) return 0;
    BfmForestTransform root;
    bfm_forest_actor_pitch_yaw(dog->view.pitch, dog->view.yaw, dog->view.xyz, &root);
    return bfm_forest_body_slide(&dog->body, &root, from, to, contact);
}

int bfm_woods_leno_target(const BfmWoodsLeno *dog, const int16_t from[3], const int16_t to[3])
{
    if (!solid(dog) || !dog->eligible) return 0;
    int16_t center[3];
    BfmForestTransform root;
    bfm_forest_actor_pitch_yaw(dog->view.pitch, dog->view.yaw, dog->view.xyz, &root);
    return bfm_forest_guard_contact(&dog->body, &root, from, to, center);
}

static void queue_charged_hit(BfmWoodsLeno *dog, unsigned yaw)
{
    dog->recoil = 16;
    dog->hit_yaw = (yaw + 2048u) & 4095u;
}

int bfm_woods_leno_charged_blade(BfmWoodsLeno *dog, const int16_t from[3], const int16_t to[3],
                                 unsigned yaw, int16_t hit[3], int16_t normal[3])
{
    if (!solid(dog) || dog->recoil || !from || !to) return 0;
    int16_t center[3];
    BfmForestTransform root;
    bfm_forest_actor_pitch_yaw(dog->view.pitch, dog->view.yaw, dog->view.xyz, &root);
    if (!bfm_forest_guard_contact_point(&dog->body, &root, from, to, center, hit, normal)) return 0;
    queue_charged_hit(dog, yaw);
    return 1;
}

int bfm_woods_leno_charged_wave(BfmWoodsLeno *dog, const int16_t spine[3], unsigned yaw,
                                int16_t hit[3])
{
    if (!solid(dog) || dog->recoil || !spine || !near(dog->view.xyz, spine, 80, 0)) return 0;
    if (hit) memcpy(hit, dog->view.xyz, sizeof dog->view.xyz);
    queue_charged_hit(dog, yaw);
    return 1;
}

int bfm_woods_leno_close_message(BfmWoodsLeno *dog)
{
    if (!dog || !dog->window) return 0;
    dog->window = 0;
    if (++dog->page == PAGES) {
        if (dog->shot == SHOW_SHOT) bfm_village_talk_camera_release(&dog->talk_camera);
        else dog->shot = NO_SHOT;
    }
    return 1;
}

static int advance_greeting(BfmWoodsLeno *dog, BfmForestMessage **opened)
{
    if (dog->page == PAGES) {
        /* The final page starts the camera return. The next actor update
         * releases the player and enables normal pickup contact. */
        dog->greeting = 0;
        dog->greeted = 1;
        dog->eligible = 1;
        return 1;
    }
    if (dog->window) return 1;
    if (dog->wait) {
        --dog->wait;
        return 1;
    }
    *opened = bfm_field_message_borrow(dog->text[dog->page], dog->length[dog->page]);
    dog->window = *opened != NULL;
    return dog->window;
}

static void seek(BfmWoodsLeno *dog)
{
    BfmForestTransform root;
    int16_t to[3];
    BfmForestContact floor;
    face(dog, dog->bone);
    bfm_forest_actor_root(dog->view.yaw, dog->view.xyz, &root);
    memcpy(to, dog->view.xyz, sizeof to);
    for (unsigned k = 0; k < 3; k += 2)
        to[k] += (int16_t)((-12 * root.rotation[k][2]) >> 12);
    if (bfm_village_ground_step(dog->ground, dog->view.xyz, to, &floor) != BFM_VILLAGE_GROUND_BLOCKED)
        memcpy(dog->view.xyz, to, sizeof to);
    if (near(dog->view.xyz, dog->bone, 48, 0)) {
        face(dog, dog->bone);
        dog->state = CHEW;
        dog->chewing = 0;
        select_clip(dog, BFM_LENO_CHEW);
    }
}

static void return_home(BfmWoodsLeno *dog)
{
    memcpy(dog->view.xyz, dog->home, sizeof dog->home);
    dog->view.yaw = dog->home_yaw;
    dog->view.pitch = 0;
    face(dog, dog->bone);
    dog->blink = 60;
    dog->chewing = 0;
    dog->state = CHEW;
    dog->release_timer = 0;
    select_clip(dog, BFM_LENO_CHEW);
    write_location(dog, 0);
}

static void release(BfmWoodsLeno *dog)
{
    /* The release packet may correct the held position against a wall. The
     * launch still uses the actor basis published by the last attachment. */
    BfmForestTransform root;
    bfm_forest_actor_pitch_yaw(dog->view.pitch, dog->view.yaw, dog->view.xyz, &root);
    for (unsigned axis = 0; axis < 3; ++axis) {
        int64_t dot = 0;
        for (unsigned k = 0; k < 3; ++k) dot += (int64_t)root.rotation[axis][k]*dog->launch[k];
        dog->velocity[axis] = (int32_t)(16*dot);
    }
    dog->view.pitch = 0;
    memset(dog->fraction, 0, sizeof dog->fraction);
    dog->age = dog->bounce = dog->settling = 0;
    dog->release_timer = 40;
    dog->state = FLIGHT;
    select_clip(dog, BFM_LENO_TOSSED);
}

static void flight(BfmWoodsLeno *dog, BfmWoodsLenoEffects *effects)
{
    int prior_fall = dog->velocity[1];
    if (dog->bounce == 2) {
        for (unsigned k = 0; k < 3; k += 2)
            dog->velocity[k] = dog->velocity[k]*15/16;
    }
    int16_t proposed[3], to[3];
    BfmForestContact floor;
    dog->velocity[1] += 3*UNIT/2;
    for (unsigned k = 0; k < 3; ++k) {
        int32_t move = dog->velocity[k] + dog->fraction[k];
        int32_t whole = move >> 16;
        dog->fraction[k] = move - whole*UNIT;
        proposed[k] = (int16_t)(dog->view.xyz[k] + whole);
    }
    memcpy(to, proposed, sizeof to);
    BfmVillageGroundResult contact = bfm_village_ground_air(dog->ground, dog->view.xyz, to, &floor);
    /* A wall slide can bring the flight back over a floor that the uncorrected
     * endpoint would miss, particularly at the island cliffs. */
    if (contact == BFM_VILLAGE_GROUND_AIR && proposed[1] >= dog->view.xyz[1] &&
        (to[0] != proposed[0] || to[2] != proposed[2]))
        contact = bfm_village_ground_air(dog->ground, dog->view.xyz, to, &floor);
    if (contact != BFM_VILLAGE_GROUND_BLOCKED) {
        for (unsigned k = 0; k < 3; ++k)
            if (to[k] != proposed[k]) dog->fraction[k] = 0;
        memcpy(dog->view.xyz, to, sizeof to);
    }
    int grounded = contact == BFM_VILLAGE_GROUND_FOOTING;
    if (grounded && floor.slope != 0x2000) {
        dog->velocity[1] = 0;
        grounded = 0;
    }
    unsigned material = grounded ? floor.flags & 0xffu : 0;
    if (grounded && (material == 2 || material == 27)) {
        dog->age = 0;
        dog->state = SINK;
        select_clip(dog, BFM_LENO_HELD);
        write_location(dog, 0);
        dog->release_timer = 0;
        effects->sound[0] = 1329;
        effects->sound[1] = 1318;
        return;
    }
    ++dog->age;
    if (dog->bounce < 2) {
        if (grounded) {
            effects->dust = 1;
            dog->velocity[1] = dog->bounce ? -prior_fall : -13*UNIT;
            ++dog->bounce;
            dog->age = 1;
            dog->settling = dog->bounce == 2 ? 1 : 0;
        } else if (dog->age >= (dog->bounce ? 40u : 120u)) {
            return_home(dog);
        }
    } else {
        if (grounded) {
            dog->velocity[1] = 0;
            dog->fraction[1] = 0;
            ++dog->settling;
            if ((dog->settling & 3u) == 3) effects->dust = 1;
            if (dog->settling >= 17) {
                dog->release_timer = 0;
                write_location(dog, 0);
                find_bone(dog);
                if (dog->state == REST) select_clip(dog, BFM_LENO_LANDED);
            }
        }
    }
}

/* A hit leaves the current animation key in place while normal gravity
 * continues. A water hit follows the shared 30-field removal path. */
static void advance_hurt(BfmWoodsLeno *dog, BfmWoodsLenoEffects *effects)
{
    if (dog->recoil == 16) {
        dog->view.pitch = 0;
        dog->view.yaw = (int)dog->hit_yaw;
        memset(dog->velocity, 0, sizeof dog->velocity);
    }
    dog->velocity[1] += 3*UNIT/2;
    int32_t move = dog->velocity[1] + dog->fraction[1];
    int32_t whole = move >> 16;
    dog->fraction[1] = move - whole*UNIT;
    int16_t to[3] = {dog->view.xyz[0], (int16_t)(dog->view.xyz[1]+whole), dog->view.xyz[2]};
    if (dog->state == HURT_IN_WATER) {
        dog->view.xyz[1] = to[1];
        if (!--dog->age) dog->state = ABSENT;
        return;
    }
    BfmForestContact floor;
    BfmVillageGroundResult result = bfm_village_ground_air(dog->ground, dog->view.xyz, to, &floor);
    if (result != BFM_VILLAGE_GROUND_BLOCKED) memcpy(dog->view.xyz, to, sizeof to);
    if (result == BFM_VILLAGE_GROUND_FOOTING) {
        dog->velocity[1] = 0;
        if ((floor.flags & 255u) == 2) {
            dog->recoil = 0;
            dog->state = HURT_IN_WATER;
            dog->age = 30;
            effects->sound[0] = 1329;
            effects->water_impact = 1;
            memcpy(effects->effect_xyz, dog->view.xyz, sizeof effects->effect_xyz);
            effects->effect_xyz[1] += 48;
            return;
        }
    }
    if (!--dog->recoil) {
        dog->eligible = 1;
        dog->chewing = 1;
    }
}

int bfm_woods_leno_step(BfmWoodsLeno *dog, const BfmWoodsLenoInput *input,
                        BfmForestMessage **opened, BfmWoodsLenoEffects *effects)
{
    if (!dog || !input || !input->player || !opened || !effects) return 0;
    *opened = NULL;
    *effects = (BfmWoodsLenoEffects){0};
    if (dog->state == ABSENT) return 1;
    if (dog->release_timer && !--dog->release_timer) write_location(dog, 0);
    if (!dog->recoil && dog->state != HURT_IN_WATER) {
        dog->view.visible = !dog->blink || (dog->blink & 1u);
        if (dog->blink) --dog->blink;
    }
    if (dog->state == CARRIED || input->command == BFM_LENO_LIFT) {
        if (dog->state != CARRIED) {
            if (!solid(dog) || !dog->eligible) return 0;
            dog->state = CARRIED;
            dog->recoil = 0;
            dog->hold = 16;
            dog->blink = dog->release_timer = 0;
            dog->view.visible = 1;
            write_location(dog, 1);
        } else if (dog->hold > 1) {
            if (--dog->hold == 1) select_clip(dog, BFM_LENO_HELD);
        } else {
            ++dog->view.tick;
        }
        int releasing = input->command == BFM_LENO_DROP || input->command == BFM_LENO_THROW;
        if (input->grip) {
            memcpy(dog->view.xyz, input->grip, sizeof dog->view.xyz);
            if (!releasing) dog->view.yaw = (input->player_yaw + 2048) & 4095;
        }
        if (releasing) release(dog);
    } else if (dog->recoil || dog->state == HURT_IN_WATER) {
        advance_hurt(dog, effects);
    } else if (dog->greeting) {
        face(dog, dog->bone);
        if (!(++dog->chewing & 1)) dog->view.pitch ^= 0xfe0;
        return advance_greeting(dog, opened);
    } else {
        int random_due = bfm_woods_leno_random_due(dog);
        ++dog->view.tick;
        switch (dog->state) {
        case SEEK: seek(dog); break;
        case CHEW:
            face(dog, dog->bone);
            dog->view.tick = 0;
            if (!(++dog->chewing & 1)) dog->view.pitch ^= 0xfe0;
            if (!dog->greeted && near(dog->view.xyz, input->player, 64, 1)) {
                dog->greeting = 1;
                dog->wait = dog->delay;
                face(dog, input->player);
                frame_greeting(dog);
            }
            break;
        case REST:
            if (random_due) {
                select_clip(dog, input->random & 1 ? BFM_LENO_REST : BFM_LENO_BARK);
                if (!(input->random & 3)) effects->sound[0] = 1317;
            }
            break;
        case FLIGHT: flight(dog, effects); break;
        case SINK:
            dog->view.pitch = (dog->view.pitch - 48) & 4095;
            dog->view.xyz[1] += 2;
            if (!(++dog->age & 7)) {
                effects->splash = 1;
                memcpy(effects->effect_xyz, dog->view.xyz, sizeof effects->effect_xyz);
            }
            if (dog->age >= 48) return_home(dog);
            break;
        default: break;
        }
    }
    if (!effects->splash && !effects->water_impact)
        memcpy(effects->effect_xyz, dog->view.xyz, sizeof effects->effect_xyz);
    return 1;
}
