/* SPDX-License-Identifier: MIT */
#include "woods_satiate_actor.h"
#include "field_waypoint.h"
#include "forest_guard_contact.h"
#include "forest_lift.h"
#include "forest_pose.h"
#include "village_ground.h"
#include "../pc_port/forest_player_bytes.h"

#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

enum { VERTICES = 65, NORMALS = 72, MAX_KEYS = 4, UNIT = 4096 };
static const uint32_t origin = UINT32_C(0x80128158), room = UINT32_C(0x8018ea38);

typedef enum { REST, WALK, RUN, BITE, LEAP, RECOIL, CLIPS } Motion;
typedef struct {
    const uint8_t *base, *delta;
    unsigned duration;
    int reverse;
} Key;
typedef struct {
    Key keys[MAX_KEYS];
    unsigned count, duration;
    int loop;
} Clip;
typedef struct {
    const uint8_t *pixels;
    unsigned x, y, w, h;
} TileRect;

typedef enum {
    UNLOADED, START_IDLE, IDLE, START_TURN, TURN, START_PATROL, PATROL,
    START_BITE, BITING, START_LEAP, LEAPING, LANDING, TETHERED, RELEASE,
    START_CHARGED_RECOIL, HURT, HURT_LANDED, HURT_FALL, FALLING_OUT, IN_WATER
} Behavior;

struct BfmWoodsSatiateActor {
    const uint8_t *scene, *shadow;
    size_t bytes;
    MusashiForestPlayerMesh *mesh;
    BfmVillageGround *ground;
    BfmGameplayRun standalone;
    BfmBedroomRoot root;
    BfmForestGuardBox body, player_box;
    int16_t home[3], ray[2][3];
    unsigned home_yaw, flight_yaw;
    Clip clips[CLIPS];
    TileRect skin[2][2];
    Behavior behavior;
    Motion motion;
    unsigned remaining, sample, next_sample, meter, strike, strike_yaw, sound;
    int yaw_step, chase, blocked, attack_face, face_changed;
    int fractions[3], leap_speed, vertical_speed, pitch;
    int width;
};

static const uint8_t *mounted(const BfmWoodsSatiateActor *a, uint32_t address, size_t size)
{
    if (address < origin || address-origin > a->bytes || size > a->bytes-(address-origin)) return NULL;
    return a->scene+(address-origin);
}

static int read_box(const uint8_t *shape, BfmForestGuardBox *box)
{
    if (!shape || u32le(shape)) return 0;
    for (unsigned k = 0; k < 3; ++k) {
        box->low[k] = s16le(shape+4+4*k);
        box->high[k] = s16le(shape+6+4*k);
        if (box->low[k] > 0 || box->high[k] < 0) return 0;
    }
    return 1;
}

static int read_home(BfmWoodsSatiateActor *a)
{
    const uint8_t *header = mounted(a,room,0xbc);
    if (!header) return 0;
    uint32_t start = u32le(header+0xb8);
    const uint8_t *row = mounted(a,start,20);
    if (!row) return 0;
    size_t available = a->bytes-(start-origin);
    unsigned found = 0;
    for (size_t pos = 0; available-pos >= 20; pos += 20) {
        const uint8_t *entry = row+pos;
        unsigned type = u16le(entry+6);
        if (!type) return found == 1;
        if (type != 109) continue;
        if (found++ || u16le(entry+8)) return 0;
        for (unsigned k = 0; k < 3; ++k) a->home[k] = s16le(entry+2*k);
        a->home_yaw = u16le(entry+12) & 4095;
    }
    return 0;
}

/* Every key in this creature's morphs replaces its entire vertex array.
 * Surface normals and face ordering belong to the single base mesh. */
static int load_clip(BfmWoodsSatiateActor *a, Clip *clip, uint32_t list)
{
    for (unsigned i = 0; i <= MAX_KEYS; ++i) {
        const uint8_t *row = mounted(a,list+8*i,8);
        if (!row) return 0;
        uint32_t pointer = u32le(row);
        if (!pointer || pointer == UINT32_MAX) {
            clip->loop = !pointer;
            return clip->count && !u32le(row+4);
        }
        if (i == MAX_KEYS) return 0;
        const uint8_t *desc = mounted(a,pointer,12);
        if (!desc || u32le(desc+8)) return 0;
        const uint8_t *mesh = mounted(a,u32le(desc),40);
        const uint8_t *span = mounted(a,u32le(desc+4),16+8*VERTICES);
        if (!mesh || !span || u32le(mesh) != 0x41 || u32le(mesh+4) ||
            u32le(mesh+8) != 1 || u32le(mesh+16) != VERTICES ||
            u32le(span) != 1 || u32le(span+4) || u32le(span+8) || u32le(span+12) != VERTICES) return 0;
        size_t available = a->bytes - (size_t)(mesh+12-a->scene);
        uint32_t offset = u32le(mesh+12);
        if (offset > available || available-offset < 8*VERTICES) return 0;
        Key *key = &clip->keys[i];
        int direction = s16le(row+6);
        key->duration = u16le(row+4);
        if (!key->duration || (direction != -1 && direction != 1)) return 0;
        key->base = mesh+12+offset;
        key->delta = span+16;
        key->reverse = direction < 0;
        clip->duration += key->duration;
        ++clip->count;
    }
    return 0;
}

static int load_face(BfmWoodsSatiateActor *a, TileRect patch[2], uint32_t address)
{
    const uint8_t *list = mounted(a,address,36);
    if (!list || list[32] != 0xff) return 0;
    for (unsigned i = 0; i < 2; ++i) {
        const uint8_t *row = list+i*16;
        TileRect *r = &patch[i];
        r->x = u16le(row+4);
        r->y = u16le(row+6);
        r->w = u16le(row+8);
        r->h = u16le(row+10);
        if (!r->w || !r->h || r->x+r->w > 1024 || r->y+r->h > 512) return 0;
        r->pixels = mounted(a,u32le(row+12),2*r->w*r->h);
        if (!r->pixels) return 0;
    }
    return 1;
}

BfmWoodsSatiateActor *bfm_woods_satiate_actor_load(const uint8_t *scene, size_t bytes)
{
    if (!scene || bytes > BFM_FIELD_SCENE_CAPACITY) return NULL;
    BfmWoodsSatiateActor *a = calloc(1,sizeof *a);
    if (!a) return NULL;
    a->scene = scene;
    a->bytes = bytes;
    const uint8_t *profile = mounted(a,UINT32_C(0x801c9278),48);
    if (!profile || u16le(profile+44) != 3 || !read_home(a) ||
        !read_box(mounted(a,u32le(profile+8),16),&a->body) ||
        !read_box(mounted(a,UINT32_C(0x801891c0),16),&a->player_box)) goto fail;
    unsigned parts = 0;
    a->mesh = musashi_forest_player_mesh_from_table(scene,bytes,origin,u32le(profile+4),&parts);
    const MusashiForestPlayerPart *part = musashi_forest_player_mesh_part(a->mesh,0);
    a->ground = bfm_village_ground_open(scene,bytes,room);
    a->shadow = mounted(a,UINT32_C(0x80189128),12);
    const uint8_t *ray = mounted(a,UINT32_C(0x801c9320),16);
    if (!part || parts != 1 || part->vertex_count != VERTICES || part->normal_count != NORMALS || !a->shadow || !a->ground || !ray) goto fail;
    for (unsigned point = 0; point < 2; ++point)
        for (unsigned k = 0; k < 3; ++k) a->ray[point][k] = s16le(ray+point*8+k*2);
    static const uint32_t motions[CLIPS] = {
        0x801c9124u, 0x801c913cu, 0x801c915cu, 0x801c917cu, 0x801c91a4u, 0x801c9234u
    };
    for (unsigned i = 0; i < CLIPS; ++i)
        if (!load_clip(a,&a->clips[i],motions[i])) goto fail;
    if (!load_face(a,a->skin[0],UINT32_C(0x801ca248)) ||
        !load_face(a,a->skin[1],UINT32_C(0x801ca418))) goto fail;
    return a;
fail:
    bfm_woods_satiate_actor_free(a);
    return NULL;
}

void bfm_woods_satiate_actor_free(BfmWoodsSatiateActor *a)
{
    if (!a) return;
    musashi_forest_player_mesh_free(a->mesh);
    bfm_village_ground_free(a->ground);
    free(a);
}

static int64_t range2(const int16_t p[3], const int16_t q[3])
{
    int64_t dx = p[0]-q[0], dy = p[1]-q[1], dz = p[2]-q[2];
    return dx*dx+dy*dy+dz*dz;
}

static int bearing(const BfmWoodsSatiateActor *a, const BfmBedroomRoot *p)
{
    int x = a->root.xyz[0]-p->xyz[0], z = a->root.xyz[2]-p->xyz[2];
    return !x && !z ? 3072 : bfm_waypoint_heading(x,z);
}

static int angle_error(const BfmWoodsSatiateActor *a, const BfmBedroomRoot *p)
{
    int difference = bearing(a,p)-(int)a->root.yaw;
    if (difference > 2048) difference -= 4096;
    if (difference < -2048) difference += 4096;
    return difference;
}

static void turn_toward(BfmWoodsSatiateActor *a, int error, int divisor)
{
    int step = error/divisor;
    if (!step) step = (error > 0)-(error < 0);
    a->root.yaw = (a->root.yaw+step)&4095;
}

static void pose(BfmWoodsSatiateActor *a, Motion motion)
{
    a->motion = motion;
    a->sample = a->next_sample = 0;
}

static void advance_pose(BfmWoodsSatiateActor *a)
{
    const Clip *clip = &a->clips[a->motion];
    a->sample = a->next_sample;
    if (++a->next_sample == clip->duration) a->next_sample = clip->loop ? 0 : clip->duration-1;
}

static void choose_rest(BfmWoodsSatiateActor *a, BfmGameplayRun *run)
{
    a->behavior = (bfm_gameplay_random(run)&1) ? START_IDLE : START_TURN;
}

enum { WALK_CLEAR, WALK_WALL, WALK_UNSUPPORTED };

/* A supported walk keeps the integrated fractions when the ground sweep
 * adjusts the integer position at a floor or wall. */

static BfmVillageGroundResult move_actor(BfmWoodsSatiateActor *a, unsigned heading, int speed, int vertical,
                                         int airborne, int y_offset, BfmForestContact *floor)
{
    const int walking = !airborne && (a->behavior == TURN || a->behavior == PATROL);
    BfmForestTransform world;
    bfm_forest_actor_root(heading,a->root.xyz,&world);
    int delta[3] = {
        (int)(-(int64_t)world.rotation[0][2]*speed/UNIT),
        walking ? 3*65536/2 : vertical,
        (int)(-(int64_t)world.rotation[2][2]*speed/UNIT)
    };
    if (!airborne) a->blocked = WALK_UNSUPPORTED;
    int16_t wanted[3], actual[3];
    int fraction[3];
    for (unsigned k = 0; k < 3; ++k) {
        int total = a->fractions[k]+delta[k];
        int whole = total/65536-(total%65536 < 0);
        int coord = a->root.xyz[k]+whole;
        if (coord < INT16_MIN || coord > INT16_MAX) return BFM_VILLAGE_GROUND_BLOCKED;
        wanted[k] = actual[k] = (int16_t)coord;
        fraction[k] = total-65536*whole;
    }
    int16_t from[3] = {a->root.xyz[0],a->root.xyz[1],a->root.xyz[2]};
    if (from[1] > INT16_MAX-y_offset || actual[1] > INT16_MAX-y_offset)
        return BFM_VILLAGE_GROUND_BLOCKED;
    from[1] += (int16_t)y_offset;
    actual[1] += (int16_t)y_offset;
    uint16_t wall = 0;
    BfmVillageGroundResult result = airborne ? bfm_village_ground_air(a->ground,from,actual,floor) :
                                               bfm_village_ground_wall_step(a->ground,from,actual,floor,&wall);
    if (actual[1] < INT16_MIN+y_offset) return BFM_VILLAGE_GROUND_BLOCKED;
    actual[1] -= (int16_t)y_offset;
    if (result == BFM_VILLAGE_GROUND_BLOCKED) return result;
    if (!airborne) {
        /* A walking actor restores all three coordinates and their fractions
         * if it lost support. A wall slide on a walkable floor is retained. */
        if (result != BFM_VILLAGE_GROUND_FOOTING || floor->slope != 0x2000) return result;
        a->blocked = wall || wanted[0] != actual[0] || wanted[2] != actual[2] ? WALK_WALL : WALK_CLEAR;
    }
    for (unsigned k = 0; k < 3; ++k)
        a->fractions[k] = walking || wanted[k] == actual[k] ? fraction[k] : 0;
    memcpy(a->root.xyz,actual,sizeof actual);
    return result;
}

static int clear_landing(const BfmWoodsSatiateActor *a, const BfmBedroomRoot *p)
{
    BfmForestContact floor;
    int distance = (int)sqrt((double)range2(a->root.xyz,p->xyz));
    BfmForestTransform forward;
    bfm_forest_actor_root((unsigned)bearing(a,p),a->root.xyz,&forward);
    const int16_t local[3] = {0,0,(int16_t)-distance};
    int16_t target[3], below[3];
    bfm_forest_lift_world(&forward,local,target);
    uint16_t wall = 0;
    int requested_x = target[0], requested_z = target[2];
    BfmVillageGroundResult surface = bfm_village_ground_wall_step(a->ground,a->root.xyz,target,&floor,&wall);
    if (surface == BFM_VILLAGE_GROUND_BLOCKED || (surface == BFM_VILLAGE_GROUND_AIR &&
        !wall && target[0] == requested_x && target[2] == requested_z) || target[1] > INT16_MAX-6) return 0;
    memcpy(below,target,sizeof below);
    below[1] += 6;
    return bfm_village_ground_air(a->ground,target,below,&floor) == BFM_VILLAGE_GROUND_FOOTING &&
           floor.slope == 0x2000;
}

static void try_strike(BfmWoodsSatiateActor *a, const BfmBedroomRoot *p, unsigned model_heading,
                       unsigned power)
{
    BfmForestTransform world, player;
    int16_t ends[2][3], center[3];
    bfm_forest_actor_pitch_yaw((unsigned)a->pitch,model_heading,a->root.xyz,&world);
    bfm_forest_actor_root(0,p->xyz,&player);
    for (unsigned i = 0; i < 2; ++i) bfm_forest_lift_world(&world,a->ray[i],ends[i]);
    if (bfm_forest_guard_contact(&a->player_box,&player,ends[0],ends[1],center)) {
        a->strike = power;
        a->strike_yaw = a->root.yaw;
    }
}

static void begin(BfmWoodsSatiateActor *a, Behavior next, Motion motion, unsigned count, int attack_face)
{
    a->behavior = next;
    a->remaining = count;
    a->attack_face = attack_face;
    pose(a,motion);
}

/* The ordinary terrain response recognizes material 2. The common
 * out-of-bounds fall also recognizes material 27 as a splash. */
static int enter_water(BfmWoodsSatiateActor *a, BfmVillageGroundResult result, const BfmForestContact *floor)
{
    if (result != BFM_VILLAGE_GROUND_FOOTING) return 0;
    unsigned material = floor->flags & 255;
    if (material != 2 && !(a->behavior == FALLING_OUT && material == 27)) return 0;
    a->behavior = IN_WATER;
    a->remaining = 30;
    a->sound = 0x531;
    return 1;
}

int bfm_woods_satiate_actor_step(BfmWoodsSatiateActor *a, const BfmBedroomView *view,
                                 const BfmBedroomRoot *p, BfmGameplayRun *run)
{
    if (!a || !view || !p) return 0;
    a->strike = 0;
    a->sound = 0;
    if (!run) run = &a->standalone; /* The room's standalone movement entry point has no playthrough. */
    a->face_changed = 0;
    int previous_face = a->attack_face;
    if (a->behavior == UNLOADED) {
        int64_t range = range2(a->home,p->xyz);
        if (range > 1536*1536 && range < 1600*1600) {
            memset(&a->root,0,sizeof a->root);
            memcpy(a->root.xyz,a->home,sizeof a->home);
            memset(a->fractions,0,sizeof a->fractions);
            a->root.yaw = a->home_yaw;
            a->pitch = a->blocked = a->attack_face = a->chase = 0;
            a->face_changed = 1;
            a->width = UNIT;
            pose(a,REST);
            choose_rest(a,run);
        }
        return 1;
    }
    int64_t range = range2(a->root.xyz,p->xyz);
    if (a->behavior != TETHERED && range >= 2048*2048) {
        a->behavior = UNLOADED;
        return 1;
    }
    BfmForestContact floor;
    /* Steering updates the pose heading this field. Ordinary ground travel
     * uses the facing matrix prepared on the preceding field. */
    unsigned travel_heading = a->root.yaw;
    switch (a->behavior) {
    case START_IDLE:
        begin(a,IDLE,REST,30+bfm_gameplay_random(run)%31,0);
        break;
    case IDLE:
        if (range <= 128*128) a->behavior = START_BITE;
        else {
            if (!--a->remaining) a->behavior = START_PATROL;
            advance_pose(a);
        }
        break;
    case START_TURN:
        a->yaw_step = (bfm_gameplay_random(run)&1) ? 34 : -34;
        begin(a,TURN,a->blocked ? REST : WALK,
              (a->blocked ? 30 : 15)+bfm_gameplay_random(run)%31,0);
        break;
    case TURN:
        a->root.yaw = (a->root.yaw+a->yaw_step)&4095;
        if (!a->blocked) {
            BfmVillageGroundResult result = move_actor(a,travel_heading,2*65536,0,0,0,&floor);
            if (enter_water(a,result,&floor)) break;
        }
        if (range2(a->root.xyz,p->xyz) <= 128*128) {
            a->behavior = START_BITE;
            break;
        }
        if (!--a->remaining) a->behavior = START_PATROL;
        advance_pose(a);
        break;
    case START_PATROL:
        a->chase = 0;
        begin(a,PATROL,WALK,90+bfm_gameplay_random(run)%61,0);
        break;
    case PATROL: {
        if (range <= 128*128) {
            a->behavior = START_BITE;
            break;
        }
        if (a->chase && !run->player.record[0x36] && range <= 32768 && clear_landing(a,p)) {
            a->behavior = START_LEAP;
            a->leap_speed = (int)sqrt((double)range)*65536/30;
            break;
        }
        int dx = a->root.xyz[0]-a->home[0], dz = a->root.xyz[2]-a->home[2];
        int error = angle_error(a,p);
        int chase = (int64_t)dx*dx+(int64_t)dz*dz < 1280*1280 && abs(error) < 1024;
        if (chase) turn_toward(a,error,4);
        if (a->chase != chase) {
            a->chase = a->attack_face = chase;
            pose(a,chase ? RUN : WALK);
        }
        if (!chase && !--a->remaining) {
            choose_rest(a,run);
            break;
        }
        BfmVillageGroundResult result = move_actor(a,travel_heading,(chase ? 8 : 2)*65536,0,0,0,&floor);
        if (enter_water(a,result,&floor)) break;
        if (a->blocked && (!chase || a->blocked == WALK_UNSUPPORTED))
            a->behavior = START_TURN;
        advance_pose(a);
        break;
    }
    case START_BITE:
        begin(a,BITING,BITE,40,1);
        break;
    case BITING:
        turn_toward(a,angle_error(a,p),3);
        if (a->remaining == 5) a->sound = 0x524;
        if (a->remaining == 10) try_strike(a,p,travel_heading,21);
        if (!--a->remaining) {
            if (range <= 128*128) a->remaining = 40;
            else {
                if (range <= 0x30000) a->behavior = START_PATROL;
                else choose_rest(a,run);
                break;
            }
        }
        advance_pose(a);
        break;
    case START_LEAP:
        a->root.yaw = (unsigned)bearing(a,p);
        a->flight_yaw = a->root.yaw;
        a->vertical_speed = -(22*65536+32768);
        a->pitch = 0;
        begin(a,LEAPING,LEAP,0,1);
        break;
    case LEAPING: {
        if (a->remaining < 15) a->pitch += 68;
        if (a->remaining == 21) try_strike(a,p,travel_heading,31);
        a->vertical_speed += 98304;
        BfmVillageGroundResult result = move_actor(a,a->flight_yaw,a->leap_speed,a->vertical_speed,1,0,&floor);
        if (enter_water(a,result,&floor)) break;
        if (result == BFM_VILLAGE_GROUND_FOOTING && floor.slope == 0x2000) {
            a->remaining = 16;
            a->behavior = LANDING;
        } else if (++a->remaining >= 40) {
            a->behavior = FALLING_OUT;
            a->remaining = 60;
        }
        advance_pose(a);
        break;
    }
    case LANDING:
        if (!--a->remaining) {
            a->pitch = 0;
            a->behavior = START_IDLE;
        }
        advance_pose(a);
        break;
    case TETHERED: {
        int desired = (144-(int)a->meter)*UNIT/144;
        if (!a->meter) a->width = UNIT;
        a->width += (desired-a->width)/4;
        break;
    }
    case RELEASE:
        a->width += (UNIT-a->width)/4;
        if (--a->remaining) break;
        a->root.yaw = ((unsigned)bearing(a,p)+2048)&4095;
        /* Fall through. */
    case START_CHARGED_RECOIL:
        /* The blow sets the model heading. The impulse points away from
         * Musashi even when the travelling wave approaches at another angle. */
        a->flight_yaw = ((unsigned)bearing(a,p)+2048)&4095;
        a->pitch = 0;
        a->width = UNIT;
        a->vertical_speed = -16*65536;
        a->leap_speed = 8*65536;
        begin(a,HURT,RECOIL,0,0);
        break;
    case HURT:
    case HURT_FALL: {
        int tumble = a->behavior == HURT;
        if (++a->remaining > 40) {
            a->behavior = FALLING_OUT;
            a->remaining = 60;
            break;
        }
        if (tumble) a->pitch = (a->pitch-256)&4095;
        a->vertical_speed += 98304;
        BfmVillageGroundResult result = move_actor(a,a->flight_yaw,tumble ? a->leap_speed : 0,a->vertical_speed,1,tumble ? 8 : 0,&floor);
        advance_pose(a);
        if (enter_water(a,result,&floor)) break;
        if (result == BFM_VILLAGE_GROUND_FOOTING && floor.slope == 0x2000) {
            a->pitch = a->blocked = 0;
            a->remaining = 15;
            a->behavior = tumble ? HURT_LANDED : START_IDLE;
        }
        break;
    }
    case HURT_LANDED: {
        BfmVillageGroundResult result = move_actor(a,a->root.yaw,0,0,0,0,&floor);
        advance_pose(a);
        if (enter_water(a,result,&floor)) break;
        if (result != BFM_VILLAGE_GROUND_FOOTING || floor.slope != 0x2000) {
            a->behavior = HURT_FALL;
            a->remaining = 0;
            a->vertical_speed = 0;
        } else if (!--a->remaining) {
            a->behavior = START_IDLE;
        }
        break;
    }
    case FALLING_OUT:
        if (a->root.xyz[1] >= 16 || !--a->remaining) {
            a->behavior = UNLOADED;
        } else {
            a->vertical_speed += 98304;
            BfmVillageGroundResult result = move_actor(a,a->flight_yaw,a->leap_speed,a->vertical_speed,1,0,&floor);
            if (enter_water(a,result,&floor)) break;
            if (result == BFM_VILLAGE_GROUND_FOOTING && floor.slope == 0x2000) {
                a->pitch = a->blocked = 0;
                a->behavior = START_IDLE;
            }
        }
        break;
    case IN_WATER:
        if (!--a->remaining) a->behavior = UNLOADED;
        break;
    case UNLOADED:
        break;
    }
    a->face_changed = previous_face != a->attack_face;
    return 1;
}

static int q12(int64_t value)
{
    return (int)(value/UNIT-(value%UNIT < 0));
}

static MusashiForestPlayerVector pitch_vector(MusashiForestPlayerVector local,
                                              const BfmForestTransform *transform)
{
    int y = q12((int64_t)transform->rotation[1][1]*local.y+
                (int64_t)transform->rotation[1][2]*local.z);
    int z = q12((int64_t)transform->rotation[2][1]*local.y+
                (int64_t)transform->rotation[2][2]*local.z);
    return (MusashiForestPlayerVector){local.x,(int16_t)y,(int16_t)z};
}

int bfm_woods_satiate_actor_add(const BfmWoodsSatiateActor *a, BfmBedroomFigure *figure,
                                const BfmBedroomView *view)
{
    if (!a || !figure || !view) return 0;
    if (a->behavior == UNLOADED) return 1;
    const Clip *clip = &a->clips[a->motion];
    unsigned age = a->sample, i = 0;
    while (i+1 < clip->count && age >= clip->keys[i].duration)
        age -= clip->keys[i++].duration;
    const Key *key = &clip->keys[i];
    int weight = (int)(UNIT*age/key->duration);
    if (key->reverse) weight = UNIT-weight;
    MusashiForestPlayerPart part = *musashi_forest_player_mesh_part(a->mesh,0);
    MusashiForestPlayerVector vertices[VERTICES], normals[NORMALS];
    BfmForestTransform pitch;
    if (a->pitch) bfm_forest_actor_pitch_yaw((unsigned)a->pitch,0,a->root.xyz,&pitch);
    for (i = 0; i < VERTICES; ++i) {
        const uint8_t *src = key->base+8*i, *d = key->delta+8*i;
        vertices[i] = (MusashiForestPlayerVector){
            (int16_t)(s16le(src)+q12(s16le(d)*weight)),
            (int16_t)(s16le(src+2)+q12(s16le(d+2)*weight)),
            (int16_t)(s16le(src+4)+q12(s16le(d+4)*weight))
        };
        if (a->pitch) vertices[i] = pitch_vector(vertices[i],&pitch);
    }
    if (a->pitch) {
        for (i = 0; i < NORMALS; ++i) normals[i] = pitch_vector(part.normals[i],&pitch);
        part.normals = normals;
    }
    part.vertices = vertices;
    BfmBedroomRoot ground = a->root;
    return bfm_bedroom_travel_air_shadow(a->scene,a->bytes,room,&ground) &&
           bfm_bedroom_figure_add_morph(figure,view,&part,&ground,(unsigned)a->width,UNIT,a->shadow,8192);
}

int bfm_woods_satiate_actor_face(const BfmWoodsSatiateActor *a, uint16_t tiles[512][1024])
{
    if (!a || !tiles) return 0;
    if (!a->face_changed || a->behavior == UNLOADED) return 1;
    for (unsigned i = 0; i < 2; ++i) {
        const TileRect *r = &a->skin[a->attack_face][i];
        for (unsigned y = 0; y < r->h; ++y)
            for (unsigned x = 0; x < r->w; ++x)
                tiles[r->y+y][r->x+x] = u16le(r->pixels+2*(y*r->w+x));
    }
    return 1;
}

unsigned bfm_woods_satiate_actor_sound(const BfmWoodsSatiateActor *a)
{
    return a ? a->sound : 0;
}

unsigned bfm_woods_satiate_actor_strike(const BfmWoodsSatiateActor *a)
{
    return a ? a->strike : 0;
}

unsigned bfm_woods_satiate_actor_strike_yaw(const BfmWoodsSatiateActor *a)
{
    return a && a->strike ? a->strike_yaw : 0;
}

static int targetable(const BfmWoodsSatiateActor *a)
{
    return a && a->behavior != UNLOADED && a->behavior != RELEASE && a->behavior != START_CHARGED_RECOIL &&
           a->behavior != HURT && a->behavior != HURT_LANDED && a->behavior != HURT_FALL &&
           a->behavior != IN_WATER && a->behavior != FALLING_OUT;
}

static void basis(const BfmWoodsSatiateActor *a, BfmForestTransform *world)
{
    bfm_forest_actor_pitch_yaw((unsigned)a->pitch,a->root.yaw,a->root.xyz,world);
}

static int body_contact(const BfmWoodsSatiateActor *a, const int16_t start[3], const int16_t end[3],
                        int16_t contact[3], int16_t center[3], int16_t normal[3])
{
    if (!targetable(a) || a->behavior == TETHERED || !contact) return 0;
    BfmForestTransform world;
    basis(a,&world);
    return bfm_forest_guard_contact_point(&a->body,&world,start,end,center,contact,normal);
}

int bfm_woods_satiate_actor_contact(const BfmWoodsSatiateActor *a, const int16_t start[3],
                                    const int16_t end[3], int16_t contact[3], int16_t center[3])
{
    return body_contact(a,start,end,contact,center,NULL);
}

int bfm_woods_satiate_actor_target(const BfmWoodsSatiateActor *a, int16_t center[3])
{
    if (!targetable(a)) return 0;
    BfmForestTransform world;
    basis(a,&world);
    return bfm_forest_guard_contact(&a->body,&world,a->root.xyz,a->root.xyz,center);
}

int bfm_woods_satiate_actor_root(const BfmWoodsSatiateActor *a, int16_t root[3])
{
    if (!targetable(a) || !root) return 0;
    memcpy(root,a->root.xyz,sizeof a->root.xyz);
    return 1;
}

int bfm_woods_satiate_actor_charged_contact(const BfmWoodsSatiateActor *a, const int16_t from[3],
                                           const int16_t to[3], int16_t hit[3], int16_t normal[3])
{
    int16_t center[3];
    return body_contact(a,from,to,hit,center,normal);
}

int bfm_woods_satiate_actor_charged_target(const BfmWoodsSatiateActor *a, int16_t root[3])
{
    return a && a->behavior != TETHERED && bfm_woods_satiate_actor_root(a,root);
}

void bfm_woods_satiate_actor_charged_recoil(BfmWoodsSatiateActor *a, unsigned heading)
{
    if (!targetable(a) || a->behavior == TETHERED) return;
    a->root.yaw = (heading+2048)&4095;
    a->behavior = START_CHARGED_RECOIL;
}

int bfm_woods_satiate_actor_latch(BfmWoodsSatiateActor *a)
{
    if (!targetable(a) || a->behavior == TETHERED) return 0;
    a->behavior = TETHERED;
    a->width = UNIT;
    a->meter = 0;
    return 1;
}

void bfm_woods_satiate_actor_meter(BfmWoodsSatiateActor *a, unsigned meter)
{
    if (a && a->behavior == TETHERED) a->meter = meter > 128 ? 128 : meter;
}

void bfm_woods_satiate_actor_release(BfmWoodsSatiateActor *a)
{
    if (a && a->behavior == TETHERED) {
        a->behavior = RELEASE;
        a->remaining = 6;
    }
}

int bfm_woods_satiate_actor_absorb(BfmWoodsSatiateActor *a)
{
    if (!a || a->behavior != TETHERED) return 0;
    /* Successful assimilation releases the dynamic placement. A new actor
     * can only be admitted from the usual distance band around its home. */
    a->behavior = UNLOADED;
    return 1;
}
