/* SPDX-License-Identifier: MIT */
#include "village_illness_grocery_cast.h"
#include "bedroom_visitor.h"
#include "field_waypoint.h"
#include "forest_pose.h"
#include "village_ground.h"
#include "../pc_port/forest_player_bytes.h"
#include <limits.h>
#include <stdlib.h>

enum { MAYOR, HOTELO, PEOPLE };
enum { IDLE, WALK, TALK, CLIPS };
typedef enum { ABSENT, HOME, INBOUND, AT_GROCERY, OUTBOUND, CONVERSING, LEAVING } Duty;

typedef struct {
    uint32_t address;
    const uint8_t *keys;
    unsigned count;
} Clip;

typedef struct {
    const uint8_t *points;
    int count;
} Route;

typedef struct {
    const uint8_t *home, *post, *hours;
    Clip clips[CLIPS];
    Route routes[2];
    BfmBedroomRoot root;
    int fraction[2];
    Duty duty;
    int leg, point, direction;
    unsigned clip, key, turn_goal, turn_left, leave_age, command, door_wait;
} Person;

struct BfmVillageIllnessGroceryCast {
    const uint8_t *scene;
    size_t bytes;
    BfmBedroomVisitor *bodies;
    BfmVillageGround *ground;
    Person people[PEOPLE];
};

static const uint32_t room = UINT32_C(0x80190780);
static const uint32_t base = UINT32_C(0x80128158);

static const uint8_t *mounted(const BfmVillageIllnessGroceryCast *cast,
                              uint32_t addr, size_t size)
{
    if (addr < base || size > cast->bytes || addr-base > cast->bytes-size) return NULL;
    return cast->scene + (addr-base);
}

static int grocery_placements(const BfmVillageIllnessGroceryCast *cast)
{
    const uint8_t *desc = mounted(cast,room,0xbc);
    if (!desc) return 0;
    uint32_t start = u32le(desc+0xb4), end = u32le(desc+0xb8);
    if (end < start || (end-start)%20) return 0;
    const uint8_t *rows = mounted(cast,start,end-start);
    if (!rows) return 0;
    unsigned count[PEOPLE] = {0};
    for (size_t offset = 0; offset < end-start; offset += 20) {
        unsigned kind = u16le(rows+offset+6);
        if (!kind) break;
        if (kind == 378 || kind == 379) ++count[kind-378];
    }
    return count[MAYOR] == 1 && count[HOTELO] == 1;
}

static int load_clip(const BfmVillageIllnessGroceryCast *cast, Clip *clip, uint32_t address)
{
    const uint8_t *keys = mounted(cast,address,8);
    if (!keys) return 0;
    size_t rows = (cast->bytes - (size_t)(keys-cast->scene)) / 8;
    for (size_t i = 0; i < rows; ++i) {
        uint32_t tagged = u32le(keys + 8*i), duration = u32le(keys + 8*i + 4);
        if (!tagged) {
            if (!i || duration != UINT16_MAX) return 0;
            *clip = (Clip){address,keys,(unsigned)i};
            return 1;
        }
        if ((tagged >> 24) != 0x81 || (tagged & 3u) || duration != 1 ||
            !mounted(cast,tagged & ~UINT32_C(0x01000000),10*8)) return 0;
    }
    return 0;
}

static int load_route(const BfmVillageIllnessGroceryCast *cast, Route *route, uint32_t address)
{
    const uint8_t *row = mounted(cast,address,4);
    if (!row) return 0;
    int count = s16le(row);
    if (count < 2 || !mounted(cast,address,4u + (unsigned)count*4u)) return 0;
    *route = (Route){row+4,count};
    return 1;
}

static int in_hours(const uint8_t *hours, const BfmSaveRecord *save)
{
    unsigned now = u16le(save->progress+0x38);
    return now >= (unsigned)hours[0]*256 + hours[1] &&
           now < (unsigned)hours[2]*256 + hours[3];
}

static void animate(Person *p, unsigned clip)
{
    p->clip = clip;
    p->key = 0;
}

static int station(const BfmVillageIllnessGroceryCast *cast, Person *p, Duty duty)
{
    const uint8_t *marker = duty == AT_GROCERY ? p->post : p->home;
    for (unsigned k = 0; k < 3; ++k) p->root.xyz[k] = s16le(marker+2*k);
    p->root.yaw = u16le(marker+6) & 4095u;
    p->fraction[0] = p->fraction[1] = 0;
    p->turn_left = p->command = 0;
    p->duty = duty;
    animate(p,IDLE);
    return duty != AT_GROCERY ||
        bfm_bedroom_travel_air_shadow(cast->scene,cast->bytes,room,&p->root);
}

void bfm_village_illness_grocery_cast_free(BfmVillageIllnessGroceryCast *cast)
{
    if (cast) {
        bfm_bedroom_visitor_free(cast->bodies);
        bfm_village_ground_free(cast->ground);
        free(cast);
    }
}

BfmVillageIllnessGroceryCast *bfm_village_illness_grocery_cast_open(
    const uint8_t *scene, size_t bytes, const BfmSaveRecord *save)
{
    if (!scene || bytes > BFM_FIELD_SCENE_CAPACITY || !save) return NULL;
    BfmVillageIllnessGroceryCast *cast = calloc(1,sizeof *cast);
    if (!cast) return NULL;
    cast->scene = scene;
    cast->bytes = bytes;
    if (!grocery_placements(cast)) goto fail;
    const uint32_t home[PEOPLE] = {0x8018e958u,0x8018fbecu};
    const uint32_t hours[PEOPLE] = {0x8018f018u,0x8018fc10u};
    const uint32_t paths[PEOPLE][2] = {
        {0x801b94ecu,0x801b94f8u}, {0x801b9550u,0x801b9560u}
    };
    uint32_t models[PEOPLE];
    uint32_t story = u32le(save->progress+8);
    for (unsigned i = 0; i < PEOPLE; ++i) {
        Person *p = &cast->people[i];
        p->home = mounted(cast,home[i],8);
        p->post = mounted(cast,home[i]+8,8);
        const uint8_t *kit = mounted(cast,home[i]+16,16);
        p->hours = mounted(cast,hours[i],4);
        if (!p->home || !p->post || !kit || !p->hours) goto fail;
        models[i] = u32le(kit);
        for (unsigned j = 0; j < CLIPS; ++j)
            if (!load_clip(cast,&p->clips[j],u32le(kit+4+4*j))) goto fail;
        for (unsigned j = 0; j < 2; ++j)
            if (!load_route(cast,&p->routes[j],paths[i][j])) goto fail;
        /* Hotelo uses the Mayor's 09:04 window on construction and his own
         * later departure time only when already waiting at home. */
        int admitted = i == MAYOR ? story < 300 : story >= 200 && story < 210;
        if (admitted && !station(cast,p,in_hours(cast->people[MAYOR].hours,save)
                                 ? AT_GROCERY : HOME)) goto fail;
    }
    cast->bodies = bfm_bedroom_visitor_load(scene,bytes,models,PEOPLE,UINT32_C(0x8018d748));
    cast->ground = bfm_village_ground_open(scene,bytes,room);
    if (cast->bodies && cast->ground) return cast;
fail:
    bfm_village_illness_grocery_cast_free(cast);
    return NULL;
}

static void turn(Person *p, unsigned yaw, int divisor)
{
    int delta = (int)yaw - (int)p->root.yaw;
    if (delta > 2048) delta -= 4096;
    if (delta < -2048) delta += 4096;
    int amount = delta/divisor;
    if (!amount) amount = (delta>0) - (delta<0);
    p->root.yaw = (p->root.yaw+amount) & 4095u;
}

static unsigned heading(const Person *p)
{
    const uint8_t *point = p->routes[p->leg].points + 4*p->point;
    return (unsigned)bfm_waypoint_heading(p->root.xyz[0]-s16le(point),
                                         p->root.xyz[2]-s16le(point+2));
}

static int follow_floor(const BfmVillageIllnessGroceryCast *cast, Person *p)
{
    if (!bfm_bedroom_travel_air_shadow(cast->scene,cast->bytes,room,&p->root)) return 0;
    if (p->root.shadow_visible) {
        p->root.xyz[1] = p->root.shadow_xyz[1];
        p->fraction[0] = p->fraction[1] = 0;
    }
    return 1;
}

static int stride(const BfmVillageIllnessGroceryCast *cast, Person *p, unsigned heading_yaw,
                  int route)
{
    const int16_t previous[3] = {p->root.xyz[0],p->root.xyz[1],p->root.xyz[2]};
    BfmForestTransform root;
    bfm_forest_actor_root(heading_yaw,p->root.xyz,&root);
    for (unsigned j = 0; j < 2; ++j) {
        unsigned axis = j*2;
        int delta = p->fraction[j] - 96*root.rotation[axis][2];
        int units = delta/65536 - (delta%65536 < 0);
        int next = p->root.xyz[axis] + units;
        if (next < INT16_MIN || next > INT16_MAX) return 0;
        p->root.xyz[axis] = (int16_t)next;
        p->fraction[j] = delta - units*65536;
    }
    if (route) return follow_floor(cast,p);
    /* Hotelo's final walk uses the ordinary field collision sweep. The
     * resident keeps its Q16 fraction when a surface corrects the whole
     * part. The downward probe supplies the Y at the corrected endpoint. */
    uint16_t material;
    if (bfm_village_ground_support_probe(cast->ground,0,previous,p->root.xyz,&material)) {
        int16_t foot[3] = {p->root.xyz[0],p->root.xyz[1],p->root.xyz[2]};
        if (bfm_village_ground_support_probe(cast->ground,2,p->root.xyz,foot,&material) & 0x6000u)
            p->root.xyz[1] = foot[1];
    }
    return bfm_bedroom_travel_air_shadow(cast->scene,cast->bytes,room,&p->root);
}

static int begin_leg(const BfmVillageIllnessGroceryCast *cast, Person *p,
                     unsigned who, int leg)
{
    const Route *path = &p->routes[leg];
    p->leg = leg;
    /* The Mayor calls his door before taking each step of the short porch
     * path. At normal speed its leaf accelerates by two angle units per
     * field, reaching its 1536-unit stop after 39 door updates. */
    p->door_wait = who == MAYOR && leg == 0 ? 39 : 0;
    p->direction = p->duty == INBOUND ? 1 : -1;
    if (who == HOTELO && leg == 1) p->direction = -p->direction;
    int start = p->direction > 0 ? 0 : path->count-1;
    p->root.xyz[0] = s16le(path->points+4*start);
    p->root.xyz[2] = s16le(path->points+4*start+2);
    p->point = start+p->direction;
    p->root.yaw = heading(p);
    return follow_floor(cast,p);
}

static int commute(const BfmVillageIllnessGroceryCast *cast, Person *p, unsigned who)
{
    const Route *path = &p->routes[p->leg];
    if (p->door_wait) {
        --p->door_wait;
        return 1;
    }
    if (p->point < 0 || p->point == path->count) {
        int next = p->leg + (p->duty == INBOUND ? 1 : -1);
        if (next == -1 || next == 2)
            return station(cast,p,p->duty == INBOUND ? AT_GROCERY : HOME);
        return begin_leg(cast,p,who,next);
    }
    unsigned previous_yaw = p->root.yaw;
    turn(p,heading(p),4);
    if (!stride(cast,p,previous_yaw,1)) return 0;
    const uint8_t *target = path->points + 4*p->point;
    int64_t dx = p->root.xyz[0]-s16le(target), dz = p->root.xyz[2]-s16le(target+2);
    if (dx*dx+dz*dz <= 256) p->point += p->direction;
    return 1;
}

static int service_duty(const BfmVillageIllnessGroceryCast *cast, Person *p, unsigned who,
                        const BfmSaveRecord *save, unsigned scene_tick)
{
    if (p->duty == INBOUND || p->duty == OUTBOUND) return commute(cast,p,who);
    if (p->duty != HOME && p->duty != AT_GROCERY) return 1;
    if (who == HOTELO && (scene_tick & 3)) return 1;
    const uint8_t *hours = p->duty == HOME ? p->hours : cast->people[MAYOR].hours;
    int opening = in_hours(hours,save);
    if (opening == (p->duty == AT_GROCERY)) return 1;
    p->duty = opening ? INBOUND : OUTBOUND;
    animate(p,WALK);
    return begin_leg(cast,p,who,opening ? 0 : 1);
}

static int service_hotelo(const BfmVillageIllnessGroceryCast *cast, Person *p, unsigned command)
{
    if (p->duty == LEAVING) {
        unsigned old_yaw = p->root.yaw;
        turn(p,1024,8);
        if (p->leave_age >= 15 && !stride(cast,p,old_yaw,0)) return 0;
        if (++p->leave_age == 60) return station(cast,p,ABSENT);
        return 1;
    }
    if (command) {
        if (p->duty != AT_GROCERY && p->duty != CONVERSING) return 0;
        if (p->duty == AT_GROCERY) {
            p->duty = CONVERSING;
            animate(p,IDLE);
            p->command = command;
            return 1;
        }
        p->command = command;
    }
    if (p->duty != CONVERSING) return 1;
    if (p->command == 5) {
        p->duty = LEAVING;
        p->leave_age = 0;
        animate(p,WALK);
        return 1;
    }
    unsigned clip = p->command == 1 || p->command == 3 ? TALK : IDLE;
    if (p->clip != clip) animate(p,clip);
    turn(p,p->command <= 2 ? 3072 : 3584,8);
    return 1;
}

int bfm_village_illness_grocery_cast_step(BfmVillageIllnessGroceryCast *cast,
    const BfmSaveRecord *save, unsigned scene_tick, int event_active,
    uint32_t mayor_program, int mayor_turn, int mayor_yaw, unsigned hotelo_command)
{
    if (!cast || !save || hotelo_command > 5) return 0;
    Person *mayor = &cast->people[MAYOR], *hotelo = &cast->people[HOTELO];
    for (unsigned i = 0; i < PEOPLE; ++i) {
        Person *p = &cast->people[i];
        if (p->duty != ABSENT && p->duty != HOME)
            p->key = (p->key+1) % p->clips[p->clip].count;
    }
    if (event_active && mayor->duty == AT_GROCERY) {
        mayor->duty = CONVERSING;
        animate(mayor,IDLE);
    } else if (!event_active && mayor->duty == CONVERSING) {
        if (!station(cast,mayor,AT_GROCERY)) return 0;
    } else if (!service_duty(cast,mayor,MAYOR,save,scene_tick)) return 0;
    if (mayor_program) {
        unsigned clip = mayor_program == mayor->clips[TALK].address ? TALK : IDLE;
        if (mayor->clips[clip].address != mayor_program) return 0;
        animate(mayor,clip);
    }
    /* The resident advances its existing turn before reading this field's
     * script commands. A new target first affects the next update. */
    if (mayor->turn_left) {
        if (--mayor->turn_left) turn(mayor,mayor->turn_goal,6);
        else mayor->root.yaw = mayor->turn_goal;
    }
    if (mayor_turn) {
        if (mayor_yaw < 0 || mayor_yaw >= 4096) return 0;
        mayor->turn_goal = (unsigned)mayor_yaw;
        mayor->turn_left = 30;
    }
    if (hotelo_command || hotelo->duty == CONVERSING || hotelo->duty == LEAVING)
        return service_hotelo(cast,hotelo,hotelo_command);
    return service_duty(cast,hotelo,HOTELO,save,scene_tick);
}

int bfm_village_illness_grocery_cast_ready(const BfmVillageIllnessGroceryCast *cast, unsigned story)
{
    return cast && story >= 200 && story < 230 && cast->people[MAYOR].duty == AT_GROCERY &&
           (story >= 210 || cast->people[HOTELO].duty == AT_GROCERY);
}

const int16_t *bfm_village_illness_grocery_cast_position(
    const BfmVillageIllnessGroceryCast *cast, unsigned actor_class)
{
    if (!cast || actor_class < 378 || actor_class > 379) return NULL;
    const Person *p = &cast->people[actor_class-378];
    return p->duty == ABSENT || p->duty == HOME ? NULL : p->root.xyz;
}

int bfm_village_illness_grocery_cast_add(const BfmVillageIllnessGroceryCast *cast,
    BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    if (!cast || !figure || !view) return 0;
    for (unsigned i = 0; i < PEOPLE; ++i) {
        const Person *p = &cast->people[i];
        if (p->duty == ABSENT || p->duty == HOME) continue;
        int xyz[3] = {p->root.xyz[0],p->root.xyz[1],p->root.xyz[2]};
        uint32_t pose = u32le(p->clips[p->clip].keys + 8*p->key) & ~UINT32_C(0x01000000);
        if (!bfm_bedroom_visitor_add_scaled(cast->bodies,figure,view,i,xyz,p->root.yaw,pose,
                                            4096,4096,&p->root)) return 0;
    }
    return 1;
}
