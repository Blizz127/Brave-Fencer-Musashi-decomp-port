/* SPDX-License-Identifier: MIT */
#include "nightmare_departure.h"
#include "forest_pose.h"
#include "nightmare_stride.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>

enum DeparturePhase {
    CLOSE_CUT, ZOOM_LEAD, FIRST_CRY, SEATED,
    SPEAKING, LAUNCH, AIRBORNE, BEFORE_WALK, WALK, EXIT, FINISHED, FAILED
};

struct BfmNightmareDeparture {
    const uint8_t *scene, *destination;
    const BfmNightmareLayout *layout;
    size_t bytes;
    BfmBedroomMotion *motion;
    BfmBedroomField *field;
    BfmBedroomRoot *root;
    BfmBedroomPose *pose;
    BfmNightmareStride *stride;
    BfmForestMessage *message;
    enum DeparturePhase phase;
    int timer, radius, zoom_speed, page_closed, on_bed;
    unsigned line, walk_delay, bed_cue, floor_cue;
    int16_t waypoint[3];
};

static const uint32_t scene_base = UINT32_C(0x80128158);
/* The next mounted record bounds each message, including the last one. */
static const unsigned text_bounds[] = {0, 40, 92, 124};

static const uint8_t *mounted(const BfmNightmareDeparture *d, uint32_t address, size_t size)
{
    if (address < scene_base || size > d->bytes || address - scene_base > d->bytes - size)
        return NULL;
    return d->scene + (address - scene_base);
}

BfmNightmareDeparture *bfm_nightmare_departure_open(const uint8_t *scene, size_t bytes,
                                 uint32_t room_address, const BfmNightmareLayout *layout, BfmBedroomMotion *motion,
                                 BfmBedroomField *field, BfmBedroomRoot *root, BfmBedroomPose *pose)
{
    if (!layout || !scene || !motion || !field || !root || !pose) return NULL;
    BfmNightmareDeparture *d = calloc(1, sizeof *d);
    if (!d) return NULL;
    d->layout = layout;
    d->scene = scene;
    d->bytes = bytes;
    d->motion = motion;
    d->field = field;
    d->root = root;
    d->pose = pose;
    /* Only the room's operands are data here. The event itself is host code. */
    const uint8_t *script = mounted(d, layout->script, 48);
    if (!script || u32le(script + 24) > 65535u ||
        u16le(script + 38) != 0) goto fail;
    d->walk_delay = u32le(script + 24);
    for (unsigned axis = 0; axis < 3; ++axis) d->waypoint[axis] = s16le(script + 32 + 2 * axis);
    d->destination = mounted(d, u32le(script + 44), 36);
    if (!d->destination || u32le(d->destination) != layout->street_exit) goto fail;
    const uint8_t *landing_sounds = mounted(d, layout->landing_sounds, 14);
    if (!landing_sounds) goto fail;
    d->floor_cue = u16le(landing_sounds);
    d->bed_cue = u16le(landing_sounds + 6 * 2);
    d->stride = bfm_nightmare_stride_open(scene, bytes, room_address);
    if (!d->stride) goto fail;
    d->phase = CLOSE_CUT;
    return d;
fail:
    bfm_nightmare_departure_free(d);
    return NULL;
}

void bfm_nightmare_departure_free(BfmNightmareDeparture *d)
{
    if (!d) return;
    bfm_forest_message_free(d->message);
    bfm_nightmare_stride_free(d->stride);
    free(d);
}

static void zoom(BfmNightmareDeparture *d)
{
    if (d->radius == 300) return;
    d->zoom_speed += 3;
    d->radius -= d->zoom_speed;
    if (d->radius < 300) d->radius = 300;
}

static void close_view(BfmNightmareDeparture *d)
{
    BfmForestTransform orbit;
    bfm_forest_actor_pitch_yaw(170, 1024, d->root->xyz, &orbit);
    int target[3] = {d->root->xyz[0], -243, 0}, eye[3];
    for (unsigned axis = 0; axis < 3; ++axis) {
        int product = orbit.rotation[axis][2] * d->radius;
        eye[axis] = target[axis] + product / 4096 - (product % 4096 < 0);
    }
    bfm_bedroom_field_viewpoint(d->field, eye, target, 1, 1);
}

static int open_line(BfmNightmareDeparture *d, unsigned line)
{
    if (line >= sizeof text_bounds / sizeof *text_bounds - 1) return 0;
    size_t extent = text_bounds[line + 1] - text_bounds[line];
    const uint8_t *text = mounted(d, d->layout->message + text_bounds[line], extent);
    if (!text) return 0;
    BfmForestMessage *page = bfm_field_message_borrow(text, extent);
    if (!page) return 0;
    bfm_forest_message_free(d->message);
    d->message = page;
    d->line = line;
    d->page_closed = 0;
    return 1;
}

const BfmForestMessageFrame *bfm_nightmare_departure_message(const BfmNightmareDeparture *d)
{
    return d && d->message ? bfm_forest_message_frame(d->message) : NULL;
}

int bfm_nightmare_departure_step(BfmNightmareDeparture *d, ForestVoice *voice,
                                 unsigned *generation, int confirm, int reveal, int other_transfer,
                                 BfmNightmareRoomEvent *event)
{
    if (!d || !voice || !generation || !event || d->phase == FAILED) return 0;
    *event = (BfmNightmareRoomEvent){0};
    if (d->phase == FINISHED) return 1;
    /* The early inn suspends its wake actor during a competing transfer. */
    other_transfer = other_transfer && d->layout->number == 0x3012;
    unsigned previous_status = bfm_forest_voice_status(voice);
    bfm_forest_voice_step(voice);
    event->voice_stopped = previous_status != BFM_FOREST_VOICE_IDLE &&
                           bfm_forest_voice_status(voice) == BFM_FOREST_VOICE_IDLE;
    if (d->message && !d->page_closed) {
        const BfmForestMessageFrame *frame = bfm_forest_voice_page(voice, d->message, generation, confirm, reveal);
        if (!frame) goto failed;
        if (frame->events & BFM_FOREST_VOICE_STOP) event->voice_stopped = 1;
        if (frame->events & BFM_FOREST_MESSAGE_CLOSED) d->page_closed = 1;
    }
    BfmBedroomAction action = BFM_BEDROOM_NONE;
    switch (d->phase) {
    case CLOSE_CUT:
        d->radius = 800;
        d->zoom_speed = 16;
        d->timer = 8;
        d->phase = ZOOM_LEAD;
        break;
    case ZOOM_LEAD:
        zoom(d);
        if (--d->timer < 0) d->phase = FIRST_CRY;
        break;
    case FIRST_CRY:
        if (other_transfer) break;
        zoom(d);
        if (d->radius < 400) {
            if (!open_line(d, 0) || !bfm_bedroom_motion_direct(d->motion, d->layout->seated_body)) goto failed;
            event->cue = 0x7e7;
            d->phase = SEATED;
        }
        break;
    case SEATED:
        zoom(d);
        if (d->radius == 300 && bfm_bedroom_motion_marker(d->motion) == BFM_BODY_HOLD) {
            if (!bfm_bedroom_motion_resume(d->motion)) goto failed;
            d->phase = SPEAKING;
        }
        break;
    case SPEAKING: {
        if (other_transfer || !d->page_closed) break;
        BfmBodyMarker marker = bfm_bedroom_motion_marker(d->motion);
        if (d->line < 2 && marker == BFM_BODY_HOLD) {
            if (!open_line(d, d->line + 1) || !bfm_bedroom_motion_resume(d->motion)) goto failed;
        } else if (d->line == 2 && marker == BFM_BODY_END) {
            bfm_forest_message_free(d->message);
            d->message = NULL;
            d->phase = LAUNCH;
        }
        break;
    }
    case LAUNCH:
        action = BFM_BEDROOM_UPRIGHT;
        if (!bfm_bedroom_motion_direct(d->motion, 0x800d4be0u)) goto failed;
        d->phase = AIRBORNE;
        break;
    case AIRBORNE: {
        int ground = bfm_nightmare_stride_hop(d->stride, d->root);
        if (ground < 0) goto failed;
        if (ground) {
            if (!d->on_bed) {
                event->landed = 1;
                event->cue = d->bed_cue;
                d->on_bed = 1;
            }
            if (!other_transfer) {
                d->timer = (int)d->walk_delay;
                d->phase = BEFORE_WALK;
            }
        }
        break;
    }
    case BEFORE_WALK:
        if (--d->timer < 0) d->phase = WALK;
        break;
    case WALK: {
        const BfmBedroomView *view = bfm_bedroom_field_view(d->field);
        if (!view) goto failed;
        int arrived = bfm_nightmare_stride_walk(d->stride, d->root, d->waypoint, view->eye, view->target);
        if (arrived < 0) goto failed;
        switch (d->root->locomotion_edge) {
        case BFM_TRAVEL_WALK:
            event->started_run = 1;
            action = BFM_BEDROOM_WALK;
            break;
        case BFM_TRAVEL_FALL: action = BFM_BEDROOM_FALL; break;
        case BFM_TRAVEL_LAND:
            event->landed = 1;
            event->cue = d->floor_cue;
            action = BFM_BEDROOM_LAND;
            break;
        case BFM_TRAVEL_STAND: action = BFM_BEDROOM_STAND; break;
        default: break;
        }
        if (arrived) d->phase = EXIT;
        break;
    }
    case EXIT:
        event->destination = d->destination;
        d->phase = FINISHED;
        break;
    default:
        goto failed;
    }
    if (!bfm_bedroom_motion_step(d->motion, action, d->pose)) goto failed;
    if (d->radius) close_view(d);
    return 1;
failed:
    d->phase = FAILED;
    return 0;
}
