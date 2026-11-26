/* SPDX-License-Identifier: MIT */
#include "village_mayor_stage.h"
#include "village_mayor_camera.h"
#include "village_mayor_cast.h"
#include "village_mayor_door.h"
#include "../pc_port/forest_player_bytes.h"
#include <limits.h>
#include <stdlib.h>

enum { PORCH, PREPARING, TALKING, LEAVING };
struct BfmVillageMayorStage {
    const uint8_t *scene, *mayor;
    size_t bytes, mayor_bytes;
    uint32_t room;
    BfmVillageMayorCast *cast;
    BfmVillageMayorDoor *door;
    BfmVillageMayorCamera camera;
    BfmBedroomTravel *walk;
    BfmVillageCamera shown;
    int door_angle, station_x, station_z, knock_z, retreat, knock;
    unsigned first_message, elapsed, release_age, phase;
};

static const uint8_t *field_record(const BfmVillageMayorStage *s, uint32_t address, size_t count)
{
    const uint32_t base = UINT32_C(0x80128158);
    if (address < base || count > s->bytes || address - base > s->bytes - count) return NULL;
    return s->scene + (address - base);
}

static int load_walk(BfmVillageMayorStage *s)
{
    const uint32_t event_base = UINT32_C(0x801ef468);
    const size_t station = UINT32_C(0x801f0900) - event_base;
    const size_t prepare = UINT32_C(0x801f0944) - event_base;
    if (s->mayor_bytes < prepare + 8 || u32le(s->mayor+prepare) != 0x35) return 0;
    const uint8_t *script = field_record(s, u32le(s->mayor+prepare+4), 128);
    if (!script || u32le(script+8) != 4 || s16le(script+12) != 0 ||
        u32le(script+24) != 0x10 || u32le(script+32) != 5 || u32le(script+120) != 2) return 0;
    s->station_x = s16le(s->mayor+station);
    s->station_z = s16le(s->mayor+station+4);
    s->knock_z = s->station_z + s16le(script+14);
    s->retreat = s16le(script+36);
    if (s->knock_z < INT16_MIN || s->knock_z > INT16_MAX || s->retreat <= 0) return 0;
    /* Aligning the field actors precedes three timed parts of their entrance:
     * the knock, the door opening and the Mayor's walk to the porch. */
    s->first_message = 16;
    for (unsigned i = 0; i < 16; ++i)
        if (u32le(script+8*i) == 15) {
            unsigned wait = u32le(script+8*i+4);
            if (wait > 0xffff) return 0;
            s->first_message += wait;
        }
    return 1;
}

BfmVillageMayorStage *bfm_village_mayor_stage_open(const uint8_t *scene, size_t bytes,
                        uint32_t room, const uint8_t *mayor, size_t mayor_bytes, uint32_t shadow)
{
    if (!scene || !mayor || room != UINT32_C(0x80192b18)) return NULL;
    BfmVillageMayorStage *s = calloc(1,sizeof *s);
    if (!s) return NULL;
    s->scene = scene;
    s->bytes = bytes;
    s->room = room;
    s->mayor = mayor;
    s->mayor_bytes = mayor_bytes;
    if (!load_walk(s) || !(s->door = bfm_village_mayor_door_load(scene,bytes,room)) ||
        !(s->cast = bfm_village_mayor_cast_load(scene,bytes,room,mayor,mayor_bytes,shadow))) {
        bfm_village_mayor_stage_free(s);
        return NULL;
    }
    return s;
}

void bfm_village_mayor_stage_free(BfmVillageMayorStage *s)
{
    if (!s) return;
    bfm_village_mayor_cast_free(s->cast);
    bfm_village_mayor_door_free(s->door);
    bfm_bedroom_travel_free(s->walk);
    free(s);
}

int bfm_village_mayor_stage_begin(BfmVillageMayorStage *s, const BfmSaveRecord *state,
                   const BfmBedroomRoot *player, const BfmVillageCamera *camera, int square)
{
    if (!s || !state || !player || !camera || !square || s->phase != PORCH) return 0;
    unsigned minute = u16le(state->progress+0x1a);
    if (!bfm_village_mayor_door_on_porch(s->door,player->xyz) || minute < 8*60 || minute >= 20*60 ||
        u32le(state->progress+8) != 10) return 0;
    if (!bfm_village_mayor_camera_begin(&s->camera,s->scene,s->bytes,s->mayor,s->mayor_bytes,camera))
        return 0;
    BfmBedroomTravel *walk = bfm_bedroom_travel_from_root(s->scene,s->bytes,s->room,player);
    if (!walk) return 0;
    bfm_bedroom_travel_free(s->walk);
    s->walk = walk;
    s->shown = *camera;
    s->elapsed = s->release_age = 0;
    s->knock = 0;
    s->phase = PREPARING;
    return 1;
}

int bfm_village_mayor_stage_step(BfmVillageMayorStage *s, BfmVillageCamera *normal)
{
    if (!s || !normal) return 0;
    if (s->phase == PORCH) return 1;
    /* The knock action starts on the player update after the approach ends. */
    const BfmBedroomRoot *player = bfm_bedroom_travel_root(s->walk);
    s->knock = s->phase == PREPARING && s->elapsed > 17 && s->elapsed < 63 &&
               player->locomotion_edge == BFM_TRAVEL_STAND;
    /* Approach and face the closed door. After knocking, Musashi makes room
     * for the resident by retreating without turning away from the door. */
    if (s->phase == PREPARING) {
        if (s->elapsed == 17 && !bfm_bedroom_travel_walk(s->walk,(int16_t)s->station_x,
                                                         (int16_t)s->knock_z,0)) return 0;
        if (s->elapsed == 63 && !bfm_bedroom_travel_retreat(s->walk,(int16_t)s->retreat)) return 0;
    }
    if (!bfm_bedroom_travel_step(s->walk,s->shown.eye,s->shown.target)) return 0;
    if (s->phase == PREPARING && s->elapsed >= 18 && s->elapsed < 63 &&
        !bfm_bedroom_travel_root(s->walk)->busy)
        bfm_bedroom_travel_face_point(s->walk,s->station_x,s->station_z);

    int leaving = s->phase == LEAVING;
    bfm_village_mayor_camera_step(&s->camera,s->elapsed,s->release_age,leaving,normal);
    bfm_village_mayor_cast_step(s->cast,s->elapsed,s->release_age,1,leaving);
    s->door_angle = bfm_village_mayor_door_angle(s->elapsed,leaving,s->release_age);
    s->shown = *normal;
    if (s->phase == PREPARING && s->elapsed == s->first_message) s->phase = TALKING;
    if (leaving && s->release_age++ >= s->camera.release_delay + 25) {
        s->phase = PORCH;
        bfm_village_mayor_cast_step(s->cast,0,0,0,0);
    }
    if (s->elapsed < UINT_MAX) ++s->elapsed;
    return 1;
}

int bfm_village_mayor_stage_knock(const BfmVillageMayorStage *s)
{
    return s && s->knock;
}

int bfm_village_mayor_stage_message_ready(const BfmVillageMayorStage *s)
{
    return s && s->phase == TALKING;
}

void bfm_village_mayor_stage_finish(BfmVillageMayorStage *s)
{
    if (s && s->phase == TALKING) {
        s->phase = LEAVING;
        s->release_age = 0;
    }
}

int bfm_village_mayor_stage_active(const BfmVillageMayorStage *s)
{
    return s && s->phase != PORCH;
}

const BfmBedroomRoot *bfm_village_mayor_stage_player(const BfmVillageMayorStage *s)
{
    return s ? bfm_bedroom_travel_root(s->walk) : NULL;
}

const int16_t *bfm_village_mayor_stage_position(const BfmVillageMayorStage *s)
{
    return s ? bfm_village_mayor_cast_position(s->cast) : NULL;
}

int bfm_village_mayor_stage_add(BfmVillageMayorStage *s, BfmBedroomFigure *figure,
                                const BfmBedroomView *view)
{
    return s && bfm_village_mayor_cast_add(s->cast,figure,view);
}


int bfm_village_mayor_stage_prepare(BfmVillageMayorStage *s, const BfmBedroomView *view,
                                    const uint16_t tiles[512][1024], uint32_t *pixels)
{
    if (!s || !view || !tiles || !pixels) return 0;
    bfm_village_mayor_door_prepare(s->door,view,tiles,pixels,s->door_angle);
    return 1;
}

int bfm_village_mayor_stage_next_depth(const BfmVillageMayorStage *s)
{
    return s ? bfm_village_mayor_door_next_depth(s->door) : INT_MIN;
}

void bfm_village_mayor_stage_draw_to(BfmVillageMayorStage *s, int depth)
{
    if (s) bfm_village_mayor_door_draw_to(s->door,depth);
}
