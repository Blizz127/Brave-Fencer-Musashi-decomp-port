/* SPDX-License-Identifier: MIT */
#include "village_mine_lake_flyer.h"
#include "field_waypoint.h"
#include "../pc_port/forest_player_bytes.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

enum { PARTS = 15, FLIGHT_FRAMES = 20 };
static const uint32_t scene_base = UINT32_C(0x80128158);

typedef struct {
    int16_t home[3], heading, height, radius;
} Placement;

typedef struct {
    int32_t position[3]; /* Q16 */
    int16_t foot[3], target[3];
    int speed, turn_divisor, pitch, yaw;
    unsigned until_retarget;
    BfmForestTransform submitted;
    BfmBedroomRoot shadow;
} Flyer;

struct BfmMineLakeFlyers {
    const BfmVillageGround *ground;
    const uint8_t *scene, *shadow_texture;
    size_t bytes;
    Placement placement[BFM_MINE_LAKE_FLYERS];
    Flyer actor[BFM_MINE_LAKE_FLYERS];
    MusashiForestPlayerMesh *mesh;
    BfmForestPose poses[FLIGHT_FRAMES];
    unsigned frame, sound_phase;
    int shadow_at_birth;
    int entered;
};

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t count)
{
    if (address < scene_base || count > bytes || address - scene_base > bytes - count) return NULL;
    return scene + (address - scene_base);
}

static int load_rows(BfmMineLakeFlyers *flyers, const uint8_t *scene, size_t bytes, const uint8_t *room)
{
    uint32_t first = u32le(room + 0xb4), end = u32le(room + 0xb8);
    if (end <= first || (end - first) % 20) return 0;
    const uint8_t *rows = mounted(scene, bytes, first, end - first);
    if (!rows) return 0;
    unsigned count = 0;
    for (size_t i = 0; i < end - first; i += 20) {
        const uint8_t *row = rows + i;
        if (!u16le(row + 6)) return count == BFM_MINE_LAKE_FLYERS;
        if (u16le(row + 6) != 10) continue;
        if (count == BFM_MINE_LAKE_FLYERS || u16le(row + 8) != 0x8010 || u16le(row + 10) ||
            s16le(row + 16) <= 0) return 0;
        Placement *placement = &flyers->placement[count++];
        for (unsigned axis = 0; axis < 3; ++axis) placement->home[axis] = s16le(row + 2 * axis);
        placement->heading = s16le(row + 12);
        placement->height = s16le(row + 14);
        placement->radius = s16le(row + 16);
    }
    return 0;
}

BfmMineLakeFlyers *bfm_mine_lake_flyers_open(const uint8_t *scene, size_t bytes,
                                            const BfmVillageGround *ground)
{
    if (!scene || !ground) return NULL;
    const uint8_t *room = mounted(scene, bytes, 0x80185a50, 0xbc);
    const uint8_t *binding = mounted(scene, bytes, 0x80181d20, 4);
    const uint8_t *profile = mounted(scene, bytes, 0x8019c764, 12);
    const uint8_t *flight = mounted(scene, bytes, 0x8019ecc0, 8 * (FLIGHT_FRAMES + 1));
    if (!binding || u32le(binding) != 0x80185a50 || !room || !profile || !flight ||
        u32le(flight + 8 * FLIGHT_FRAMES) || u32le(flight + 8 * FLIGHT_FRAMES + 4) != 0xffff) return NULL;
    BfmMineLakeFlyers *flyers = calloc(1, sizeof *flyers);
    if (!flyers) return NULL;
    flyers->ground = ground;
    flyers->scene = scene;
    flyers->bytes = bytes;
    flyers->shadow_texture = mounted(scene, bytes, 0x80183684, 12);
    if (!flyers->shadow_texture) goto fail;
    unsigned parts = 0;
    flyers->mesh = musashi_forest_player_mesh_from_table(scene, bytes, scene_base, u32le(profile + 4), &parts);
    if (!flyers->mesh || parts != PARTS || !load_rows(flyers, scene, bytes, room)) goto fail;
    for (unsigned i = 0; i < FLIGHT_FRAMES; ++i) {
        uint32_t key = u32le(flight + 8 * i);
        if ((key >> 24) != 0x81 || u32le(flight + 8 * i + 4) != 1) goto fail;
        const uint8_t *pose = mounted(scene, bytes, key & UINT32_C(0xf0ffffff), 8 * PARTS);
        if (!pose || !bfm_forest_actor_pose_decode(pose, 8 * PARTS, PARTS, &flyers->poses[i])) goto fail;
    }
    return flyers;
fail:
    bfm_mine_lake_flyers_free(flyers);
    return NULL;
}

void bfm_mine_lake_flyers_free(BfmMineLakeFlyers *flyers)
{
    if (flyers) {
        musashi_forest_player_mesh_free(flyers->mesh);
        free(flyers);
    }
}

void bfm_mine_lake_flyers_enter(BfmMineLakeFlyers *flyers, BfmGameplayRun *run)
{
    if (!flyers || !run) return;
    for (unsigned i = 0; i < BFM_MINE_LAKE_FLYERS; ++i) {
        const Placement *placement = &flyers->placement[i];
        Flyer *actor = &flyers->actor[i];
        *actor = (Flyer){.speed = -(12 + (bfm_gameplay_random(run) & 7)), .yaw = placement->heading};
        for (unsigned j = 0; j < 3; ++j) {
            actor->foot[j] = placement->home[j];
            actor->position[j] = (int32_t)placement->home[j] * 65536;
        }
        bfm_forest_actor_pitch_yaw(0, actor->yaw, actor->foot, &actor->submitted);
        memcpy(actor->shadow.xyz, actor->foot, sizeof actor->foot);
        actor->shadow.shadow_visible = 1;
        actor->shadow.shadow_normal[1] = -4096;
    }
    flyers->shadow_at_birth = 1;
    flyers->frame = 0;
    flyers->sound_phase = 1; /* The constructor was the first class dispatch. */
    flyers->entered = 1;
}

static int flight_sine(unsigned heading)
{
    return (int)lround(4096.0 * sin((heading & 4095u) * 0.0015339807878856412));
}

static void choose_target(Flyer *actor, const Placement *placement, BfmGameplayRun *run)
{
    actor->until_retarget = 128 + (bfm_gameplay_random(run) & 127);
    actor->turn_divisor = 10 + (bfm_gameplay_random(run) & 31);
    unsigned angle = bfm_gameplay_random(run) & 4095;
    int radius = bfm_gameplay_random(run) % placement->radius;
    actor->target[0] = placement->home[0] + ((flight_sine(angle) * radius) >> 12);
    actor->target[2] = placement->home[2] + ((flight_sine(angle + 1024) * radius) >> 12);
    /* Height is relative to Musashi throughout the flight. */
    actor->target[1] = placement->height - (bfm_gameplay_random(run) & 255);
}

static int turn_step(int yaw, int heading, int divisor)
{
    int delta = heading - (yaw & 4095);
    if (delta > 2048) delta -= 4096;
    if (delta < -2048) delta += 4096;
    int step = delta / divisor;
    return step ? step : (delta > 0) - (delta < 0);
}

static void advance(BfmMineLakeFlyers *flyers, Flyer *actor, const int16_t hero[3])
{
    int dx = actor->target[0] - actor->foot[0], dz = actor->target[2] - actor->foot[2];
    int dy = hero[1] + actor->target[1] - actor->foot[1];
    int heading = (bfm_waypoint_heading(-dz, dx) - 1024) & 4095;
    int along = (dz * flight_sine(heading + 1024) + dx * flight_sine(heading)) >> 12;
    actor->pitch = bfm_waypoint_heading(dy, -along);
    actor->yaw = (actor->yaw + turn_step(actor->yaw, heading, actor->turn_divisor)) & 4095;
    /* Motion reads the model orientation submitted at the previous field. */
    int16_t next[3];
    for (unsigned i = 0; i < 3; ++i) {
        actor->position[i] += actor->speed * 16 * actor->submitted.rotation[i][2];
        next[i] = (int16_t)(actor->position[i] >> 16);
    }
    uint16_t material;
    bfm_village_ground_rock_probe(flyers->ground, 1, actor->foot, next, &material);
    for (unsigned i = 0; i < 3; ++i) {
        actor->foot[i] = next[i];
        actor->position[i] = (int32_t)next[i] * 65536 + (uint16_t)actor->position[i];
    }
    bfm_forest_actor_pitch_yaw(actor->pitch, actor->yaw, actor->foot, &actor->submitted);
}

static uint16_t sound_control(const Flyer *actor, const int16_t hero[3])
{
    int64_t distance = 0;
    for (unsigned i = 0; i < 3; ++i) {
        int delta = actor->foot[i] - hero[i];
        distance += (int64_t)delta * delta;
    }
    int gain = 127;
    if (distance > 4096) gain -= (int)((distance - 4096) / 9984);
    return (uint16_t)(gain | 0x1000);
}

unsigned bfm_mine_lake_flyers_step(BfmMineLakeFlyers *flyers, BfmGameplayRun *run,
    const int16_t hero[3], uint16_t controls[BFM_MINE_LAKE_FLYERS])
{
    if (!flyers || !flyers->entered || !run || !hero) return 0;
    flyers->shadow_at_birth = 0;
    flyers->frame = (flyers->frame + 1) % FLIGHT_FRAMES;
    flyers->sound_phase = (flyers->sound_phase + 1) & 15;
    for (unsigned i = 0; i < BFM_MINE_LAKE_FLYERS; ++i) {
        Flyer *actor = &flyers->actor[i];
        if (!actor->until_retarget) choose_target(actor, &flyers->placement[i], run);
        else {
            advance(flyers, actor, hero);
            if (!--actor->until_retarget) choose_target(actor, &flyers->placement[i], run);
        }
        /* Each fixed flyer immediately owns its following floor-sprite slot. */
        memcpy(actor->shadow.xyz, actor->foot, sizeof actor->foot);
        actor->shadow.yaw = (uint16_t)actor->yaw;
        bfm_bedroom_travel_air_shadow(flyers->scene, flyers->bytes, 0x80185a50, &actor->shadow);
        if (!flyers->sound_phase && controls) controls[i] = sound_control(actor, hero);
    }
    return !flyers->sound_phase && controls ? BFM_MINE_LAKE_FLYERS : 0;
}

int bfm_mine_lake_flyers_draw(const BfmMineLakeFlyers *flyers, BfmBedroomFigure *figure,
                              const BfmBedroomView *view)
{
    if (!flyers || !figure || !view) return 0;
    if (!flyers->entered) return 1;
    for (unsigned i = 0; i < BFM_MINE_LAKE_FLYERS; ++i) {
        const Flyer *actor = &flyers->actor[i];
        int screen[3];
        if (bfm_bedroom_project_packet(view, actor->foot, screen) &&
            screen[0] > 160 - 200 && screen[0] <= 160 + 200 &&
            screen[1] > 120 - 200 && screen[1] <= 120 + 200) {
            BfmForestActor model = {.mesh = flyers->mesh, .pose = &flyers->poses[flyers->frame],
                                  .root = &actor->submitted, .visible_parts = (1u << PARTS) - 1};
            if (!bfm_bedroom_figure_add_actor(figure, view, &model, NULL, NULL, 0)) return 0;
        }
        if (!bfm_bedroom_figure_add_floor_shadow(figure, view, &actor->shadow, flyers->shadow_texture,
                                                4096, flyers->shadow_at_birth)) return 0;
    }
    return 1;
}
