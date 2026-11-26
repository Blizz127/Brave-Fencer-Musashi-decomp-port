/* SPDX-License-Identifier: MIT */
#include "village_residents.h"
#include "bedroom_visitor.h"
#include "forest_guard_contact.h"
#include "village_arrival_motion.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>
#include <string.h>

enum { IDLE_KEYS = 8, WALK_KEYS = 16, BODY_PARTS = 10, PLACEMENT_BYTES = 20 };
static const uint32_t scene_origin = UINT32_C(0x80128158);

typedef enum { RESTING, ADMITTED, STARTING, SPEAKING, FINISHING, RETURNING } Conversation;
typedef struct {
    const uint8_t *program;
    size_t length;
} Reply;

struct BfmVillageResidents {
    BfmBedroomVisitor *body;
    const uint8_t *scene;
    size_t bytes;
    uint32_t room;
    int position[3], heading;
    uint32_t idle[IDLE_KEYS], talk[IDLE_KEYS], walk[WALK_KEYS];
    const uint8_t *station, *offstage, *route;
    unsigned points, waypoint, walk_age;
    enum { OFF_DUTY, WALKING_IN, ON_DUTY, WALKING_OUT } duty;
    BfmForestGuardBox post_box, walk_box;
    Reply replies[3];
    const uint8_t *tail_limits;
    Conversation conversation;
    uint64_t delay;
    unsigned prepare_ticks, finish_ticks, pose_start;
};

static const uint8_t *record(const uint8_t *scene, size_t bytes, uint32_t address, size_t size)
{
    if (address < scene_origin || size > bytes || address - scene_origin > bytes - size)
        return NULL;
    return scene + (address - scene_origin);
}

static int has_hillside_resident(const uint8_t *scene, size_t bytes, const uint8_t *room)
{
    uint32_t list = u32le(room + 0xb4), end = u32le(room + 0xb8);
    if (end <= list || (end - list) % PLACEMENT_BYTES) return 0;
    const uint8_t *placements = record(scene, bytes, list, end - list);
    if (!placements) return 0;
    for (size_t offset = 0; offset < end - list; offset += PLACEMENT_BYTES) {
        unsigned kind = u16le(placements + offset + 6);
        if (kind == 160) return 1;
        if (!kind) break;
    }
    return 0;
}

static int read_clip(const uint8_t *scene, size_t bytes, uint32_t address, uint32_t *poses, unsigned count)
{
    const uint8_t *clip = record(scene, bytes, address, (count + 1) * 8);
    if (!clip || u32le(clip + count * 8) || u32le(clip + count * 8 + 4) != 0xffffu)
        return 0;
    for (unsigned i = 0; i < count; ++i) {
        uint32_t tagged = u32le(clip + i * 8);
        poses[i] = tagged & ~UINT32_C(0x01000000);
        if (tagged >> 24 != 0x81 || u32le(clip + i * 8 + 4) != 1 ||
            !record(scene, bytes, poses[i], BODY_PARTS * 8)) return 0;
    }

    return 1;
}

static int read_reply(const uint8_t *scene, size_t bytes, uint32_t address, size_t span, Reply *reply)
{
    const uint8_t *p = record(scene, bytes, address, span);
    if (!p || p[0] != 5) return 0;
    const uint8_t *end = memchr(p, 0, span);
    if (!end) return 0;
    reply->program = p;
    reply->length = (size_t)(end - p) + 1;
    return 1;
}

static int read_box(const uint8_t *scene, size_t bytes, uint32_t address, BfmForestGuardBox *box)
{
    const uint8_t *shape = record(scene, bytes, address, 16);
    if (!shape || u32le(shape)) return 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        box->low[axis] = s16le(shape + 4 + axis * 4);
        box->high[axis] = s16le(shape + 6 + axis * 4);
        if (box->low[axis] > box->high[axis]) return 0;
    }
    return 1;
}

static int work_hours(const BfmSaveRecord *state)
{
    unsigned minute = u16le(state->progress + 0x1a);
    return minute >= 9 * 60 && minute < 18 * 60;
}

static void set_station(BfmVillageResidents *r, const uint8_t *station)
{
    for (unsigned i = 0; i < 3; ++i) r->position[i] = s16le(station + i * 2);
    r->heading = s16le(station + 6);
}

BfmVillageResidents *bfm_village_residents_load(const uint8_t *scene, size_t bytes,
                                                 uint32_t room, uint32_t shadow,
                                                 const BfmSaveRecord *state)
{
    if (!scene || !state || room != UINT32_C(0x8018c794)) return NULL;
    const uint8_t *descriptor = record(scene, bytes, room, 0xbc);
    if (!descriptor || !has_hillside_resident(scene, bytes, descriptor)) return NULL;

    /* The hillside resident's daytime standing station contains position and
     * bearing. Its model and idle clip belong to the following resource kit. */
    const uint8_t *station = record(scene, bytes, UINT32_C(0x80189850), 8);
    const uint8_t *kit = record(scene, bytes, UINT32_C(0x80189860), 16);
    if (!station || !kit) return NULL;
    BfmVillageResidents *residents = calloc(1, sizeof(*residents));
    if (!residents) return NULL;
    /* The standing conversation leads into a close camera, waits before
     * revealing its balloon, then holds briefly before releasing control. */
    const uint8_t *lead = record(scene, bytes, UINT32_C(0x801898c4), 8);
    const uint8_t *tail = record(scene, bytes, UINT32_C(0x801898ec), 8);
    residents->tail_limits = record(scene, bytes, UINT32_C(0x80185c44), 12);
    residents->station = station;
    residents->scene = scene;
    residents->bytes = bytes;
    residents->room = room;
    residents->offstage = record(scene, bytes, UINT32_C(0x80189904), 8);
    const uint8_t *path = record(scene, bytes, UINT32_C(0x801bdc00), 4);
    residents->points = path ? u32le(path) : 0;
    if (residents->points >= 2 && residents->points <= 256)
        residents->route = record(scene, bytes, UINT32_C(0x801bdc04), residents->points * 4);
    if (!residents->offstage || !residents->route ||
        !read_box(scene, bytes, UINT32_C(0x80189874), &residents->post_box) ||
        !read_box(scene, bytes, UINT32_C(0x80189884), &residents->walk_box) ||
        !residents->tail_limits || !lead || !tail || u32le(lead) != 15 || u32le(tail) != 15 ||
        !read_clip(scene, bytes, u32le(kit + 4), residents->idle, IDLE_KEYS) ||
        !read_clip(scene, bytes, u32le(kit + 8), residents->walk, WALK_KEYS) ||
        !read_clip(scene, bytes, u32le(kit + 12), residents->talk, IDLE_KEYS) ||
        !read_reply(scene, bytes, UINT32_C(0x801a9ee0), 124, &residents->replies[0]) ||
        !read_reply(scene, bytes, UINT32_C(0x801a9f5c), 160, &residents->replies[1]) ||
        !read_reply(scene, bytes, UINT32_C(0x801a9ffc), 140, &residents->replies[2])) {
        free(residents);
        return NULL;
    }
    residents->prepare_ticks = u32le(lead + 4);
    residents->finish_ticks = u32le(tail + 4);
    uint32_t model = u32le(kit);
    residents->body = bfm_bedroom_visitor_load(scene, bytes, &model, 1, shadow);
    if (!residents->body) {
        free(residents);
        return NULL;
    }
    residents->duty = work_hours(state) ? ON_DUTY : OFF_DUTY;
    set_station(residents, station);
    return residents;
}

void bfm_village_residents_free(BfmVillageResidents *residents)
{
    if (!residents) return;
    bfm_bedroom_visitor_free(residents->body);
    free(residents);
}

static int target_heading(const BfmVillageResidents *r)
{
    const uint8_t *point = r->route + 4 * r->waypoint;
    int32_t target[3] = {s16le(point), 0, s16le(point + 2)};
    int32_t origin[3] = {r->position[0], 0, r->position[2]};
    return bfm_village_motion_view_yaw(target, origin);
}

/* The walking actor casts straight down at its new X/Z. The field's shadow
 * query uses this same vertical segment and two-unit surface clearance. */
static int follow_ground(BfmVillageResidents *r, int walking)
{
    BfmBedroomRoot root = {0};
    for (unsigned i = 0; i < 3; ++i) root.xyz[i] = (int16_t)r->position[i];
    if (!walking) root.xyz[1] += 16;
    if (!bfm_bedroom_travel_air_shadow(r->scene, r->bytes, r->room, &root)) return 0;
    r->position[1] = root.shadow_visible ? root.shadow_xyz[1] : r->position[1] - (walking ? 16 : 0);
    return 1;
}

int bfm_village_residents_step(BfmVillageResidents *r, const BfmSaveRecord *state)
{
    if (!r || r->conversation != RESTING) return 1;
    int daytime = work_hours(state);
    if (r->duty == OFF_DUTY || r->duty == ON_DUTY) {
        if (daytime == (r->duty == ON_DUTY)) return 1;
        set_station(r, daytime ? r->offstage : r->station);
        r->duty = daytime ? WALKING_IN : WALKING_OUT;
        r->waypoint = daytime ? r->points - 2 : 1;
        r->walk_age = 0;
        r->heading = target_heading(r);
        return !daytime || follow_ground(r, 0);
    }
    if (r->waypoint == r->points) {
        r->duty = r->duty == WALKING_IN ? ON_DUTY : OFF_DUTY;
        set_station(r, r->station);
        return 1;
    }

    int16_t position[3];
    for (unsigned i = 0; i < 3; ++i) position[i] = (int16_t)r->position[i];
    BfmForestTransform basis;
    bfm_forest_actor_root((unsigned)r->heading, position, &basis);
    int turn = target_heading(r) - r->heading;
    if (turn > 2048) turn -= 4096;
    if (turn < -2048) turn += 4096;
    int amount = turn / 4;
    if (!amount) amount = (turn > 0) - (turn < 0);
    r->heading = (r->heading + amount) & 4095;
    for (unsigned axis = 0; axis < 3; axis += 2) {
        int delta = -6 * basis.rotation[axis][2];
        r->position[axis] += delta / 4096 - (delta % 4096 < 0);
    }
    if (!follow_ground(r, 1)) return 0;
    const uint8_t *goal = r->route + r->waypoint * 4;
    int64_t dx = r->position[0] - s16le(goal), dz = r->position[2] - s16le(goal + 2);
    if (dx * dx + dz * dz <= 16 * 16) {
        if (r->duty == WALKING_OUT) ++r->waypoint;
        else r->waypoint = r->waypoint ? r->waypoint - 1 : r->points;
    }
    ++r->walk_age;
    return 1;
}

int bfm_village_residents_contact(const BfmVillageResidents *r, unsigned field_tick,
                                  const int16_t previous[3], int16_t endpoint[3])
{
    if (!r || r->duty == OFF_DUTY || r->conversation != RESTING) return 0;
    const BfmForestGuardBox *box = r->duty == ON_DUTY ? &r->post_box : &r->walk_box;
    int16_t position[3], from[3], to[3], hit[3];
    for (unsigned i = 0; i < 3; ++i) {
        position[i] = (int16_t)r->position[i];
        from[i] = previous[i];
        to[i] = endpoint[i];
    }
    BfmForestTransform root;
    bfm_forest_actor_root((unsigned)r->heading, position, &root);
    from[1] -= 16;
    to[1] -= 16;
    if (!bfm_forest_body_slide(box, &root, from, to, hit)) return 0;
    int16_t center[3];
    if (bfm_forest_guard_contact(box, &root, from, from, center)) {
        unsigned phase = (field_tick >> 5) & 3;
        int dx = phase < 2 ? 7 : 8, dz = phase == 2 ? 7 : 8;
        hit[0] += from[0] > center[0] ? dx : -dx;
        hit[2] += from[2] > center[2] ? dz : -dz;
        hit[1] = endpoint[1] - 16;
    }
    endpoint[0] = hit[0];
    endpoint[1] = hit[1] + 16;
    endpoint[2] = hit[2];
    return 1;
}

int bfm_village_residents_add(const BfmVillageResidents *residents, BfmBedroomFigure *figure,
                              const BfmBedroomView *view, unsigned age)
{
    if (!residents) return 0;
    if (residents->duty == OFF_DUTY) return 1;
    const uint32_t *clip = residents->conversation >= STARTING ? residents->talk : residents->idle;
    unsigned key = (age - residents->pose_start) % IDLE_KEYS;
    if (residents->duty == WALKING_IN || residents->duty == WALKING_OUT) {
        clip = residents->walk;
        key = residents->walk_age % WALK_KEYS;
    }
    return bfm_bedroom_visitor_add(residents->body, figure, view, 0,
                                   residents->position, residents->heading, clip[key]);
}

int bfm_village_residents_station(const BfmVillageResidents *residents, int16_t xyz[3], unsigned *yaw)
{
    if (!residents || residents->duty == OFF_DUTY || !xyz || !yaw) return 0;
    for (unsigned i = 0; i < 3; ++i) xyz[i] = (int16_t)residents->position[i];
    *yaw = (unsigned)residents->heading & 4095u;
    return 1;
}

int bfm_village_residents_begin_talk(BfmVillageResidents *residents, const int16_t player[3],
                                     const BfmSaveRecord *state, int square_edge)
{
    if (!residents || residents->duty != ON_DUTY || !player || !state ||
        !square_edge || residents->conversation != RESTING) return 0;
    uint32_t progress = u32le(state->progress + 8);
    if (!work_hours(state) || (state->flags[240 / 8] & 1u) ||
        progress == 133 || progress == 134) return 0;
    int64_t distance_squared = 0;
    for (unsigned i = 0; i < 3; ++i) {
        int64_t delta = (int)player[i] - residents->position[i];
        distance_squared += delta * delta;
    }
    if (distance_squared >= 128 * 128) return 0;
    residents->conversation = ADMITTED;
    return 1;
}

int bfm_village_residents_talking(const BfmVillageResidents *residents)
{
    return residents && residents->conversation != RESTING;
}

int bfm_village_residents_camera_returning(const BfmVillageResidents *residents)
{
    return residents && residents->conversation == RETURNING;
}

int bfm_village_residents_step_talk(BfmVillageResidents *residents, BfmSaveRecord *state,
                                    unsigned age, BfmForestMessage **opened)
{
    if (!opened) return 0;
    *opened = NULL;
    if (!residents || !state) return 0;
    if (residents->conversation == RESTING || residents->conversation == SPEAKING) return 1;
    if (residents->conversation == ADMITTED) {
        /* The accepted Square starts the standing conversation on the next
         * actor update. Its camera and Musashi's approach run during the wait. */
        residents->conversation = STARTING;
        residents->pose_start = age;
        residents->delay = residents->prepare_ticks;
        return 1;
    }
    if (residents->delay && --residents->delay) return 1;
    if (residents->conversation == FINISHING) {
        /* The release cue runs the first of 25 return-camera updates now.
         * The standing actor keeps its speaking pose until the last update. */
        residents->conversation = RETURNING;
        residents->delay = 24;
        return 1;
    }
    if (residents->conversation == RETURNING) {
        residents->conversation = RESTING;
        residents->pose_start = age;
        return 1;
    }
    unsigned tally = state->variables[220];
    unsigned reply_index = tally == 0 ? 0 : (tally & 1u ? 1 : 2);
    const Reply *reply = &residents->replies[reply_index];
    *opened = bfm_field_message_borrow(reply->program, reply->length);
    if (!*opened) return 0;
    state->variables[220] = (uint8_t)(tally == 255 ? 2 : tally + 1);
    state->flags[222 / 8] |= 1u << (222 % 8);
    residents->conversation = SPEAKING;
    residents->pose_start = age;
    return 1;
}

void bfm_village_residents_close_talk(BfmVillageResidents *residents)
{
    if (residents && residents->conversation == SPEAKING) {
        residents->conversation = FINISHING;
        /* The resident observes the closed window on its next update. The
         * first visible return follows the wait because the camera steps
         * before the resident script on each field logic update. */
        residents->delay = (uint64_t)residents->finish_ticks + 2;
    }
}

int bfm_village_residents_place_window(const BfmVillageResidents *residents,
                                       const BfmBedroomView *view, const int16_t player[3],
                                       int *anchor, int *portrait_on_right)
{
    if (!residents || !view || !player || !anchor || !portrait_on_right) return 0;
    int16_t station[3];
    for (unsigned i = 0; i < 3; ++i) station[i] = (int16_t)residents->position[i];
    return bfm_village_speaker_anchor(view, station, player, residents->tail_limits,
                                     anchor, portrait_on_right);
}

int bfm_village_speaker_anchor(const BfmBedroomView *view, const int16_t station[3],
                              const int16_t player[3], const uint8_t limits[12],
                              int *anchor, int *portrait_on_right)
{
    int speaker[3], listener[3];
    if (!bfm_bedroom_project(view, station, speaker) ||
        !bfm_bedroom_project(view, player, listener)) return 0;
    int right = speaker[0] >= listener[0];
    int scale = right ? -7232 : 7232;
    int position = (speaker[0] - 160) * 4096 / scale;
    /* The field balloons use separate inner and outer bands. The mounted
     * limits and facing side position the pointed tail along the lower rim. */
    int inner = position > s16le(limits + 2) && position < s16le(limits + 8);
    int low = s16le(limits + (inner ? 4 : 0));
    int high = s16le(limits + (inner ? 6 : 10));
    if (position < low) position = low;
    if (position > high) position = high;
    int fixed = position * scale;
    *anchor = 160 + right + fixed / 4096 - (fixed % 4096 < 0);
    *portrait_on_right = right;
    return 1;
}
