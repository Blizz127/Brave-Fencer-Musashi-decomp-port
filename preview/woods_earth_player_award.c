/* SPDX-License-Identifier: MIT */
#include "woods_earth_player_award.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>
#include <string.h>

enum { MAX_KEYS = 48, POSE_SIZE = 256, EARTH_FLAG = 22 };
static const uint32_t scene_origin = UINT32_C(0x80128158);
static const uint32_t body_origin = UINT32_C(0x801eba80);
static const uint32_t raise_program = UINT32_C(0x80194cf0);
static const uint32_t return_program = UINT32_C(0x80194d94);
static const uint32_t program_end = UINT32_C(0x80194e58);

typedef struct {
    const uint8_t *pose;
    unsigned face;
    int face_changed, fusion, lumina, marker;
} BodyKey;

typedef struct {
    BodyKey keys[MAX_KEYS];
    size_t count;
    int terminal_sound;
} BodyClip;

enum ActionPhase {
    ARRIVE, TURN, PREPARE, BEFORE_RAISE, RAISE, EFFECT, MESSAGE,
    AFTER_MESSAGE, RETURN, FINISHED
};

struct BfmWoodsEarthPlayerAward {
    uint8_t *body;
    BodyClip raise, lower;
    const BodyClip *playing;
    const uint8_t *message;
    size_t message_bytes, key;
    int timer, ended;
    enum ActionPhase phase;
};

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t n)
{
    if (address < scene_origin) return NULL;
    size_t pos = address-scene_origin;
    return pos <= bytes && n <= bytes-pos ? scene+pos : NULL;
}

/* These two single-speed programs interleave model keys with face, blade and
 * event controls. A control and its following pose belong to the same tick. */
static int load_clip(BodyClip *clip, const uint8_t *commands, size_t words,
                     const uint8_t *body, size_t bytes)
{
    BodyKey next = {0};
    for (size_t i = 0; i < words; ++i) {
        uint32_t cmd = u32le(commands+4*i);
        switch (cmd) {
        case 0:
            clip->terminal_sound = next.marker;
            return i+1 == words && clip->count > 0;
        case 7:
            next.marker = 1;
            break;
        case 15:
            if (++i >= words) return 0;
            next.face = u32le(commands+4*i);
            if (next.face > 8) return 0;
            next.face_changed = 1;
            break;
        case 17:
            next.fusion = 0;
            break;
        case 18:
        case 19:
            next.lumina = cmd == 18;
            break;
        default:
            if (cmd < body_origin+4 || clip->count == MAX_KEYS) return 0;
            size_t pos = cmd-body_origin;
            if ((pos-4)%POSE_SIZE || pos > bytes || POSE_SIZE > bytes-pos) return 0;
            next.pose = body+pos;
            clip->keys[clip->count++] = next;
            next.marker = next.face_changed = 0;
            break;
        }
    }
    return 0;
}

BfmWoodsEarthPlayerAward *bfm_woods_earth_player_award_open(
    const MusashiDiscMedia *disc, const uint8_t *scene, size_t bytes, uint32_t descriptor)
{
    if (!disc || !scene || descriptor != UINT32_C(0x80195b1c)) return NULL;
    const uint8_t *action = mounted(scene, bytes, descriptor, 32);
    const uint8_t *request = mounted(scene, bytes, UINT32_C(0x80195bd4), 8);
    const uint8_t *stream = mounted(scene, bytes, raise_program, program_end-raise_program);
    if (!action || !request || !stream || u32le(request) != 0x3c || u32le(request+4) != descriptor ||
        u32le(action+4) != 2 || u32le(action+8) || u32le(action+12) ||
        u32le(action+28) != UINT32_C(0x8017ff34)) return NULL;
    uint32_t text = u32le(action+24);
    if (text != UINT32_C(0x80196960)) return NULL;
    size_t text_size = UINT32_C(0x80196a0c)-text;
    const uint8_t *message = mounted(scene, bytes, text, text_size);
    if (!message) return NULL;
    BfmWoodsEarthPlayerAward *a = calloc(1, sizeof *a);
    if (!a) return NULL;
    size_t body_bytes = 0;
    a->body = bfm_village_event_data(disc, 60, &body_bytes);
    size_t first_bytes = return_program-raise_program;
    if (!a->body || body_bytes < 4 || u32le(a->body) != 106 ||
        !load_clip(&a->raise, stream, first_bytes/4, a->body, body_bytes) ||
        !load_clip(&a->lower, stream+first_bytes, (program_end-return_program)/4, a->body, body_bytes)) {
        bfm_woods_earth_player_award_free(a);
        return NULL;
    }
    a->message = message;
    a->message_bytes = text_size;
    return a;
}

void bfm_woods_earth_player_award_free(BfmWoodsEarthPlayerAward *a)
{
    if (a) free(a->body);
    free(a);
}

static void bind_clip(BfmWoodsEarthPlayerAward *a, const BodyClip *clip, BfmEarthPlayerAwardFrame *frame)
{
    a->playing = clip;
    a->key = 0;
    a->ended = 0;
    frame->face_changed = clip->keys[0].face_changed;
}

static void earn_scroll(BfmSaveRecord *record, BfmGameplayRun *run)
{
    uint8_t bit = 1u << (EARTH_FLAG%8);
    record->flags[EARTH_FLAG/8] |= bit;
    run->story_flags[EARTH_FLAG/8] |= bit;
    record->progress[8] = 133;
    memset(record->progress+9, 0, 3);
    memcpy(run->player.record+8, record->progress+8, 4);
}

int bfm_woods_earth_player_award_step(BfmWoodsEarthPlayerAward *a,
    BfmEarthPlayerAwardInput input, BfmSaveRecord *record, BfmGameplayRun *run,
    BfmEarthPlayerAwardFrame *out)
{
    if (!a || !record || !run || !run->has_player || !out) return 0;
    BfmEarthPlayerAwardFrame frame = {0};
    switch (a->phase) {
    case ARRIVE:
        /* This descriptor has no staging walk. The next field begins turning. */
        if ((record->flags[EARTH_FLAG/8] | run->story_flags[EARTH_FLAG/8]) & (1u << (EARTH_FLAG%8)))
            return 0;
        a->phase = TURN;
        frame.face_camera = 1;
        break;
    case TURN:
        if (input.facing_camera_done) a->phase = PREPARE;
        else frame.face_camera = 1;
        break;
    case PREPARE:
        a->timer = 1;
        a->phase = BEFORE_RAISE;
        break;
    case BEFORE_RAISE:
        if (--a->timer < 0) {
            bind_clip(a, &a->raise, &frame);
            frame.enter_presentation = 1;
            a->phase = RAISE;
        }
        break;
    case RAISE:
        if (a->ended) {
            frame.spawn_earth_effect = 1;
            a->phase = EFFECT;
        }
        break;
    case EFFECT:
        if (input.effect_message_ready) {
            frame.message = a->message;
            frame.message_bytes = a->message_bytes;
            a->phase = MESSAGE;
        }
        break;
    case MESSAGE:
        if (input.window_closed) {
            frame.finish_earth_effect = 1;
            frame.stop_sound = 0x639;
            a->timer = 96;
            a->phase = AFTER_MESSAGE;
        }
        break;
    case AFTER_MESSAGE:
        if (--a->timer < 0) {
            bind_clip(a, &a->lower, &frame);
            frame.restore_presentation = 1;
            a->phase = RETURN;
        }
        break;
    case RETURN:
        if (a->ended ? a->playing->terminal_sound : a->playing->keys[a->key].marker) {
            frame.sound = 0x445;
        } else if (a->ended) {
            earn_scroll(record, run);
            frame.committed = 1;
            a->phase = FINISHED;
        }
        break;
    case FINISHED:
        break;
    }
    frame.complete = a->phase == FINISHED;
    if (a->playing && !frame.complete) {
        /* Binding consumes key zero immediately. The normal body service
         * follows the player action even in the field that bound the clip. */
        if (!a->ended) {
            if (a->key+1 == a->playing->count) a->ended = 1;
            else frame.face_changed |= a->playing->keys[++a->key].face_changed;
        }
        const BodyKey *key = &a->playing->keys[a->key];
        frame.pose = key->pose;
        frame.face_slot = key->face;
        frame.show_fusion = key->fusion;
        frame.show_lumina = key->lumina;
    }
    *out = frame;
    return 1;
}
