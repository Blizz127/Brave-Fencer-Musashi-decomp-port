/* SPDX-License-Identifier: MIT */
#include "village_stocks_prop.h"
#include "field_waypoint.h"
#include "forest_guard_contact.h"
#include "forest_pose.h"
#include "village_ground.h"
#include "../pc_port/forest_player_bytes.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

enum { PARTS = 12, MAX_KEYS = 40, ONE = 65536 };
enum Clip { REST, WANDER, PAUSE, AT_STATION, RUN, CLIPS };
enum Activity { ABSENT, BY_STOCKS, WALK_TO_STATION, WATCH, DEPARTING };

static const uint32_t origin = UINT32_C(0x80128158), room = UINT32_C(0x80192b18);

typedef struct {
    const uint8_t *keys[MAX_KEYS];
    unsigned count;
} Motion;

struct BfmVillageStocksProp {
    const uint8_t *scene, *shadow;
    size_t bytes;
    MusashiForestPlayerMesh *mesh;
    BfmVillageGround *ground;
    BfmForestGuardBox stocks;
    BfmForestTransform jon;
    BfmBedroomRoot body;
    int16_t home[3], station[3];
    int32_t velocity[3], roam_speed, greeting_speed, departure_speed;
    unsigned fraction[3], cycle, remaining, key, blocked;
    Motion clips[CLIPS];
    enum Clip clip;
    enum Activity activity;
    int send_requested;
    BfmGameplayRun standalone;
};

static const uint8_t *span(const BfmVillageStocksProp *dog, uint32_t address, size_t n)
{
    if (address < origin || n > dog->bytes || address - origin > dog->bytes - n) return NULL;
    return dog->scene + (address - origin);
}

static int read_clip(BfmVillageStocksProp *dog, enum Clip id, uint32_t address)
{
    Motion *clip = &dog->clips[id];
    for (unsigned k = 0; k <= MAX_KEYS; ++k) {
        const uint8_t *row = span(dog, address + 8*k, 8);
        if (!row) return 0;
        uint32_t tag = u32le(row);
        if (!tag) return clip->count && u32le(row+4) == 0xffff;
        if (k == MAX_KEYS || (tag >> 24) != 0x81 || (tag & 3) || u32le(row+4) != 1) return 0;
        clip->keys[k] = span(dog, tag & ~UINT32_C(0x01000000), 8*PARTS);
        if (!clip->keys[k]) return 0;
        ++clip->count;
    }
    return 0;
}

static int read_placement(BfmVillageStocksProp *dog)
{
    const uint8_t *header = span(dog,room,0xbc);
    if (!header) return 0;
    uint32_t first = u32le(header+0xb4), end = u32le(header+0xb8);
    if (end <= first || (end-first)%20) return 0;
    const uint8_t *rows = span(dog,first,end-first);
    if (!rows) return 0;
    unsigned found = 0;
    for (size_t offset = 0; offset < end-first; offset += 20) {
        const uint8_t *row = rows+offset;
        unsigned kind = u16le(row+6);
        if (!kind) return found == 1;
        if (kind != 282) continue;
        if (found++ || u16le(row+8)) return 0;
        for (unsigned k=0; k<3; ++k) dog->body.xyz[k] = s16le(row+2*k);
        dog->body.yaw = u16le(row+12) & 4095u;
    }
    return 0;
}

BfmVillageStocksProp *bfm_village_stocks_prop_open(const uint8_t *scene, size_t bytes,
                                                  const BfmSaveRecord *save)
{
    if (!scene || !save || bytes > BFM_FIELD_SCENE_CAPACITY) return NULL;
    BfmVillageStocksProp *dog = calloc(1,sizeof *dog);
    if (!dog) return NULL;
    dog->scene = scene;
    dog->bytes = bytes;
    uint32_t progress = u32le(save->progress+8);
    if (progress < 20 || progress >= 70 || (progress != 60 && (save->flags[229/8] & (1u<<(229%8)))))
        return dog;

    const uint8_t *profile = span(dog,UINT32_C(0x80190fdc),12);
    const uint8_t *marks = span(dog,UINT32_C(0x80191010),52);
    const uint8_t *station = span(dog,UINT32_C(0x80192a64),8);
    const uint8_t *box = span(dog,UINT32_C(0x801c0b08),16);
    dog->shadow = span(dog,UINT32_C(0x8018ee24),12);
    if (!profile || !marks || !station || !box || !dog->shadow || u32le(box) || !read_placement(dog))
        goto fail;
    int16_t jon[3];
    for (unsigned i=0; i<3; ++i) {
        dog->home[i] = s16le(marks+2*i);
        dog->station[i] = s16le(marks+8+2*i);
        jon[i] = s16le(station+2*i);
        dog->stocks.low[i] = s16le(box+4+4*i);
        dog->stocks.high[i] = s16le(box+6+4*i);
        if (dog->stocks.low[i] > dog->stocks.high[i]) goto fail;
    }
    /* Strides are along local -Z in Q16. */
    if (u32le(marks+16) || u32le(marks+20) || u32le(marks+28) || u32le(marks+32) ||
        u32le(marks+24) != (uint32_t)(-4*ONE) || u32le(marks+36) != (uint32_t)-ONE ||
        u32le(marks+40) || u32le(marks+44) || u32le(marks+48) != (uint32_t)(-12*ONE)) goto fail;
    dog->greeting_speed = -4*ONE;
    dog->roam_speed = -ONE;
    dog->departure_speed = -12*ONE;
    bfm_forest_actor_root(u16le(station+6),jon,&dog->jon);
    unsigned parts;
    dog->mesh = musashi_forest_player_mesh_from_table(scene,bytes,origin,u32le(profile+4),&parts);
    dog->ground = bfm_village_ground_open(scene,bytes,room);
    if (!dog->mesh || parts != PARTS || !dog->ground ||
        !read_clip(dog,REST,UINT32_C(0x801be2f0)) || !read_clip(dog,WANDER,UINT32_C(0x801be498)) ||
        !read_clip(dog,PAUSE,UINT32_C(0x801be3f0)) || !read_clip(dog,AT_STATION,UINT32_C(0x801be398)) ||
        !read_clip(dog,RUN,UINT32_C(0x801be5e0)))
        goto fail;
    dog->remaining = 1;
    dog->activity = BY_STOCKS;
    return dog;
fail:
    bfm_village_stocks_prop_free(dog);
    return NULL;
}

void bfm_village_stocks_prop_free(BfmVillageStocksProp *dog)
{
    if (!dog) return;
    musashi_forest_player_mesh_free(dog->mesh);
    bfm_village_ground_free(dog->ground);
    free(dog);
}

static int floor_div(int64_t value, int divisor)
{
    return (int)(value / divisor - (value % divisor < 0));
}

static void stride(BfmVillageStocksProp *dog, int32_t speed)
{
    BfmForestTransform root;
    bfm_forest_actor_root(dog->body.yaw,dog->body.xyz,&root);
    for (unsigned i=0; i<3; i+=2)
        dog->velocity[i] = floor_div((int64_t)speed*root.rotation[i][2],4096);
}

static unsigned heading(const BfmVillageStocksProp *dog, const int16_t target[3])
{
    return (unsigned)bfm_waypoint_heading(dog->body.xyz[0]-target[0],dog->body.xyz[2]-target[2]);
}

static int near(const int16_t xyz[3],const int16_t target[3],int radius)
{
    int64_t x = xyz[0]-target[0], z = xyz[2]-target[2];
    return x*x + z*z < radius*radius;
}

static void play(BfmVillageStocksProp *dog,enum Clip clip)
{
    dog->clip = clip;
    dog->key = 0;
}

static int advance_xyz(const BfmVillageStocksProp *dog, int16_t next[3], unsigned fraction[3])
{
    for (unsigned k=0; k<3; ++k) {
        int64_t sum = (int64_t)fraction[k] + dog->velocity[k];
        int whole = floor_div(sum,ONE), coordinate = next[k]+whole;
        if (coordinate < INT16_MIN || coordinate > INT16_MAX) return 0;
        next[k] = (int16_t)coordinate;
        fraction[k] = (unsigned)(sum-(int64_t)whole*ONE);
    }
    return 1;
}

static int move(BfmVillageStocksProp *dog, int *obstacle)
{
    int16_t next[3], contact[3];
    unsigned fraction[3];
    memcpy(next,dog->body.xyz,sizeof next);
    memcpy(fraction,dog->fraction,sizeof fraction);
    *obstacle = 0;
    if (dog->velocity[1] > INT32_MAX - (ONE + ONE/2)) return 0;
    dog->velocity[1] += ONE + ONE/2;
    if (!advance_xyz(dog,next,fraction)) return 0;
    BfmForestContact floor;
    int wall = 0;
    BfmVillageGroundResult result =
        bfm_village_ground_air_contact(dog->ground,dog->body.xyz,next,&floor,&wall);
    if (result == BFM_VILLAGE_GROUND_BLOCKED) {
        *obstacle = 1;
        return 1;
    }
    if (result == BFM_VILLAGE_GROUND_FOOTING && (dog->activity != DEPARTING || (floor.slope & 0x6000)))
        dog->velocity[1] = -4*ONE;
    if (dog->activity == DEPARTING) {
        /* The departing actor keeps one more stride after a wall projection. */
        if (wall && !advance_xyz(dog,next,fraction)) return 0;
    } else if (bfm_forest_guard_contact(&dog->stocks,&dog->jon,dog->body.xyz,next,contact)) {
        *obstacle = 1;
        int x = dog->body.xyz[0]-dog->jon.translation[0], z = dog->body.xyz[2]-dog->jon.translation[2];
        if (x > dog->stocks.low[0] && x < dog->stocks.high[0] &&
            z > dog->stocks.low[2] && z < dog->stocks.high[2]) dog->body.xyz[2] += 64;
        return 1;
    }
    memcpy(dog->body.xyz,next,sizeof next);
    memcpy(dog->fraction,fraction,sizeof fraction);
    return 1;
}

void bfm_village_stocks_prop_greet(BfmVillageStocksProp *dog)
{
    if (!dog || dog->activity != BY_STOCKS) return;
    dog->activity = WALK_TO_STATION;
    play(dog,REST);
}

int bfm_village_stocks_prop_send(BfmVillageStocksProp *dog)
{
    if (!dog || dog->activity == ABSENT) return 0;
    if (dog->activity != DEPARTING) dog->send_requested = 1;
    return 1;
}

int bfm_village_stocks_prop_step(BfmVillageStocksProp *dog, BfmGameplayRun *run)
{
    if (!dog) return 0;
    if (dog->activity == ABSENT) return 1;
    dog->key = (dog->key+1) % dog->clips[dog->clip].count;
    if (dog->send_requested) {
        dog->send_requested = 0;
        dog->activity = DEPARTING;
        dog->remaining = 120;
        dog->body.yaw = 3072;
        stride(dog,dog->departure_speed);
        play(dog,RUN);
    }
    int obstacle = 0;
    if (dog->activity == DEPARTING) {
        if (!move(dog,&obstacle)) return 0;
        if (!--dog->remaining) dog->activity = ABSENT;
    } else if (dog->activity == WALK_TO_STATION) {
        stride(dog,dog->greeting_speed);
        if (!move(dog,&obstacle)) return 0;
        if (obstacle) {
            dog->blocked = (dog->blocked+1) & 63;
            dog->body.yaw = (dog->body.yaw + (dog->blocked < 31 ? 512 : -512)) & 4095u;
        } else dog->blocked = 0;
        int turn = (int)heading(dog,dog->station)-(int)dog->body.yaw;
        if (turn > 2048) turn -= 4096;
        if (turn < -2048) turn += 4096;
        int step = turn / 4;
        if (!step && turn) step = turn > 0 ? 1 : -1;
        dog->body.yaw = (dog->body.yaw+step) & 4095u;
        if (near(dog->body.xyz,dog->station,32)) {
            dog->activity = WATCH;
            dog->body.yaw = heading(dog,dog->home);
            play(dog,AT_STATION);
        }
    } else if (dog->activity == BY_STOCKS) {
        if (dog->cycle) {
            if (!move(dog,&obstacle)) return 0;
            if (!bfm_bedroom_travel_air_shadow(dog->scene,dog->bytes,room,&dog->body)) return 0;
            if (dog->body.shadow_visible && dog->body.xyz[1] < dog->body.shadow_xyz[1]-48)
                dog->body.xyz[2] += 64;
        }
        if (!--dog->remaining) {
            dog->cycle = (dog->cycle+1) & 3;
            dog->remaining = dog->cycle ? 30 : 20;
            play(dog,dog->cycle ? WANDER : PAUSE);
            if (dog->cycle) {
                if (near(dog->body.xyz,dog->home,128))
                    dog->body.yaw = (dog->body.yaw + (bfm_gameplay_random(run ? run : &dog->standalone)&511)) & 4095u;
                else dog->body.yaw = heading(dog,dog->home);
                stride(dog,dog->roam_speed);
            }
        }
    }
    return 1;
}

int bfm_village_stocks_prop_add(const BfmVillageStocksProp *dog, BfmBedroomFigure *figure,
                               const BfmBedroomView *view)
{
    if (!dog || !figure || !view) return 0;
    if (dog->activity == ABSENT) return 1;
    BfmForestPose pose;
    if (!bfm_forest_actor_pose_decode(dog->clips[dog->clip].keys[dog->key],PARTS*8,PARTS,&pose)) return 0;
    BfmForestTransform root;
    bfm_forest_actor_root(dog->body.yaw,dog->body.xyz,&root);
    BfmBedroomRoot ground = dog->body;
    if (!bfm_bedroom_travel_air_shadow(dog->scene,dog->bytes,room,&ground)) return 0;
    BfmForestActor actor = {.mesh=dog->mesh,.pose=&pose,.root=&root,.visible_parts=(1u<<PARTS)-1};
    return bfm_bedroom_figure_add_actor(figure,view,&actor,&ground,dog->shadow,8192);
}
