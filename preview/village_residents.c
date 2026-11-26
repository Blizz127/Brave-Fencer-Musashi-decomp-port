/* SPDX-License-Identifier: MIT */
#include "village_residents.h"
#include "bedroom_visitor.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>
#include <string.h>

enum { IDLE_KEYS = 8, BODY_PARTS = 10, PLACEMENT_BYTES = 20 };
static const uint32_t scene_origin = UINT32_C(0x80128158);

typedef enum { RESTING, ADMITTED, STARTING, SPEAKING, FINISHING, RETURNING } Conversation;
typedef struct {
    const uint8_t *program;
    size_t length;
} Reply;

struct BfmVillageResidents {
    BfmBedroomVisitor *body;
    int position[3], heading;
    uint32_t idle[IDLE_KEYS], talk[IDLE_KEYS];
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

static int read_clip(const uint8_t *scene, size_t bytes, uint32_t address, uint32_t poses[IDLE_KEYS])
{
    const uint8_t *clip = record(scene, bytes, address, (IDLE_KEYS + 1) * 8);
    if (!clip || u32le(clip + IDLE_KEYS * 8) || u32le(clip + IDLE_KEYS * 8 + 4) != 0xffffu)
        return 0;
    for (unsigned i = 0; i < IDLE_KEYS; ++i) {
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

BfmVillageResidents *bfm_village_residents_load(const uint8_t *scene, size_t bytes,
                                                 uint32_t room, uint32_t shadow)
{
    if (!scene || room != UINT32_C(0x8018c794)) return NULL;
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
    if (!residents->tail_limits || !lead || !tail || u32le(lead) != 15 || u32le(tail) != 15 ||
        !read_clip(scene, bytes, u32le(kit + 4), residents->idle) ||
        !read_clip(scene, bytes, u32le(kit + 12), residents->talk) ||
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
    for (unsigned i = 0; i < 3; ++i) residents->position[i] = s16le(station + i * 2);
    residents->heading = s16le(station + 6);
    return residents;
}

void bfm_village_residents_free(BfmVillageResidents *residents)
{
    if (!residents) return;
    bfm_bedroom_visitor_free(residents->body);
    free(residents);
}

int bfm_village_residents_add(const BfmVillageResidents *residents, BfmBedroomFigure *figure,
                              const BfmBedroomView *view, unsigned age)
{
    if (!residents) return 0;
    const uint32_t *clip = residents->conversation >= STARTING ? residents->talk : residents->idle;
    unsigned key = (age - residents->pose_start) % IDLE_KEYS;
    return bfm_bedroom_visitor_add(residents->body, figure, view, 0,
                                   residents->position, residents->heading, clip[key]);
}

int bfm_village_residents_station(const BfmVillageResidents *residents, int16_t xyz[3], unsigned *yaw)
{
    if (!residents || !xyz || !yaw) return 0;
    for (unsigned i = 0; i < 3; ++i) xyz[i] = (int16_t)residents->position[i];
    *yaw = (unsigned)residents->heading & 4095u;
    return 1;
}

int bfm_village_residents_begin_talk(BfmVillageResidents *residents, const int16_t player[3],
                                     const BfmSaveRecord *state, int square_edge)
{
    if (!residents || !player || !state || !square_edge || residents->conversation != RESTING) return 0;
    unsigned minute = u16le(state->progress + 0x1a);
    uint32_t progress = u32le(state->progress + 8);
    if (minute < 9 * 60 || minute >= 18 * 60 || (state->flags[240 / 8] & 1u) ||
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
