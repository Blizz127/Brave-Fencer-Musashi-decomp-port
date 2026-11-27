/* SPDX-License-Identifier: MIT */
#include "woods_rootrick_challenge.h"
#include "../pc_port/forest_player_bytes.h"

static const uint32_t origin = UINT32_C(0x80128158);
static const uint32_t challenge = UINT32_C(0x80195198);
static const uint32_t outcomes[] = {
    UINT32_C(0x801952d8), UINT32_C(0x801953d8),
    UINT32_C(0x801954b8), UINT32_C(0x801955b0)
};
static const uint32_t messages = UINT32_C(0x801962c4);
static const uint32_t outcome_messages = UINT32_C(0x8019657c);
static const uint32_t messages_end = UINT32_C(0x801967f8);
static const uint32_t final_message = UINT32_C(0x80196788);

static const uint8_t *field_data(const BfmRootrickChallenge *s, uint32_t address, size_t bytes)
{
    if (address < origin || bytes > s->bytes || address - origin > s->bytes - bytes) return NULL;
    return s->scene + (address - origin);
}

static int begin(BfmRootrickChallenge *s, const uint8_t *scene, size_t bytes,
                 unsigned outcome)
{
    if (!s || !scene || outcome > 3) return 0;
    uint32_t first = outcome ? outcomes[outcome - 1] : challenge;
    uint32_t first_text = outcome ? outcome_messages : messages;
    uint32_t text_end = outcome ? messages_end : outcome_messages;
    BfmRootrickChallenge next = { .scene = scene, .bytes = bytes,
                                  .cursor = first - origin, .end = outcomes[outcome] - origin,
                                  .outcome = outcome, .window_flags = {64,64} };
    const uint8_t *room = field_data(&next, UINT32_C(0x80190ce4), 4);
    if (!room || u32le(room) != UINT32_C(0x801961e8) ||
        !field_data(&next, first_text, text_end - first_text)) return 0;
    *s = next;
    return 1;
}

int bfm_woods_rootrick_challenge_begin(BfmRootrickChallenge *s, const uint8_t *scene, size_t bytes)
{
    return begin(s, scene, bytes, 0);
}

int bfm_woods_rootrick_outcome_begin(BfmRootrickChallenge *s, const uint8_t *scene,
                                     size_t bytes, unsigned earned_outcome)
{
    return earned_outcome && begin(s, scene, bytes, earned_outcome);
}

static int speech_gesture(uint8_t flags[64], BfmRootrickApply apply, void *context)
{
    const unsigned flag = 0x125;
    const unsigned mask = 1u << (flag % 8);
    if (!(flags[flag/8] & mask)) return 1;
    const BfmRootrickCommand gesture = { .event = BFM_ROOTRICK_BIND_POSE,
                                         .operand = UINT32_C(0x801f45d4) };
    if (!apply(context, &gesture)) return 0;
    flags[flag/8] &= (uint8_t)~mask;
    return 1;
}

static BfmRootrickResult step(BfmRootrickChallenge *s, int window_active, int actor_release,
    int score_ready, uint8_t flags[64], BfmRootrickApply apply, void *context)
{
    if (!s || !s->scene || s->failed || !flags || !apply) return BFM_ROOTRICK_ERROR;
    if (s->terminal) return BFM_ROOTRICK_FINISHED;
    if (s->listening && !speech_gesture(flags, apply, context)) goto invalid;
    if (s->waiting_message && window_active) return BFM_ROOTRICK_RUNNING;
    s->waiting_message = 0;
    if (s->waiting_score && !score_ready) return BFM_ROOTRICK_RUNNING;
    s->waiting_score = 0;
    if (s->delay && !actor_release && --s->delay) return BFM_ROOTRICK_RUNNING;
    s->delay = 0;

    while (s->cursor < s->end) {
        const uint8_t *row = s->scene + s->cursor;
        uint32_t op = u32le(row), arg = u32le(row + 4);
        BfmRootrickCommand command = { .operand = arg };
        s->cursor += 8;
        switch (op) {
        case 0x00: case 0x01: {
            uint32_t first = s->outcome ? outcome_messages : messages;
            uint32_t end = s->outcome ? messages_end : outcome_messages;
            if (arg < first || arg >= end || (arg & 3u)) goto invalid;
            command.bytes = end - arg;
            command.data = field_data(s, arg, command.bytes);
            if (!command.data || command.data[0] != 5) goto invalid;
            command.event = op ? BFM_ROOTRICK_PLAYER_MESSAGE : BFM_ROOTRICK_ACTOR_MESSAGE;
            command.window_offset = s->offsets[op];
            command.window_flags = s->window_flags[op];
            if (!apply(context, &command)) goto invalid;
            s->last_message = arg;
            s->waiting_message = 1;
            return BFM_ROOTRICK_RUNNING;
        }
        case 0x02:
            s->terminal = 1;
            return BFM_ROOTRICK_RUNNING; /* The actor observes completion on its next poll. */
        case 0x03:
            command.event = BFM_ROOTRICK_WALK_PLAYER;
            if (s->walk_from_player) {
                command.event = BFM_ROOTRICK_WALK_FROM_PLAYER;
                command.operand = ((uint32_t)(uint16_t)-384) << 16;
                s->walk_from_player = 0;
            }
            break;
        case 0x04: command.event = BFM_ROOTRICK_WALK_FROM_ACTOR; break;
        case 0x07: command.event = BFM_ROOTRICK_PLAYER_YAW; break;
        case 0x08: command.event = BFM_ROOTRICK_PLAYER_RELATIVE_YAW; break;
        case 0x09: command.event = BFM_ROOTRICK_FACE_POINT; break;
        case 0x0a: command.event = BFM_ROOTRICK_FACE_ACTOR; break;
        case 0x0f:
            if (!arg) goto invalid;
            s->delay = arg;
            return BFM_ROOTRICK_RUNNING;
        case 0x11: case 0x13:
            command.event = BFM_ROOTRICK_PLAYER_CONTROL_BITS;
            command.operand = op == 0x11 ? UINT32_C(1) << 25 : UINT32_C(1) << 27;
            break;
        case 0x17: command.event = BFM_ROOTRICK_PLAYER_ATTACK; break;
        case 0x18: command.event = BFM_ROOTRICK_PLAYER_TARGET; break;
        case 0x1d: command.event = BFM_ROOTRICK_PLAYER_NOTICE; break;
        case 0x1e:
            command.event = BFM_ROOTRICK_CAMERA_SHOT;
            command.bytes = 16;
            command.data = field_data(s, arg, command.bytes);
            if (!command.data) goto invalid;
            break;
        case 0x23: command.event = BFM_ROOTRICK_BIND_POSE; break;
        case 0x24: command.event = BFM_ROOTRICK_SET_YAW; break;
        case 0x25: command.event = BFM_ROOTRICK_TURN_ACTOR; break;
        case 0x26: command.event = BFM_ROOTRICK_TURN_TO_PLAYER; break;
        case 0x27: command.event = BFM_ROOTRICK_DRIVE_ACTOR; break;
        case 0x2c: command.event = BFM_ROOTRICK_ACTOR_STAGE; break;
        case 0x30: command.event = BFM_ROOTRICK_ACTOR_VISIBLE; break;
        case 0x33:
            switch (arg) {
            case UINT32_C(0x801847c8): command.event = BFM_ROOTRICK_RACE_CAMERA; break;
            case UINT32_C(0x801847f0): command.event = BFM_ROOTRICK_ROOM_CAMERA; break;
            case UINT32_C(0x8018479c):
                if (s->outcome != 2) goto invalid;
                s->walk_from_player = 1;
                continue;
            case UINT32_C(0x80184590): case UINT32_C(0x801845c0):
                command.event = BFM_ROOTRICK_SUMMIT_BOULDER;
                command.operand = arg == UINT32_C(0x801845c0);
                break;
            case UINT32_C(0x80184818): case UINT32_C(0x80184838):
                command.event = BFM_ROOTRICK_STAGE_SCORE;
                command.operand = arg == UINT32_C(0x80184818) ? 29u : 30u;
                break;
            case UINT32_C(0x80184858):
                if (s->outcome < 2 || s->last_message != final_message) goto invalid;
                flags[158 / 8] |= (uint8_t)(1u << (158 % 8));
                continue;
            default: goto invalid;
            }
            break;
        case 0x37: case 0x38: {
            unsigned slot = op - 0x37;
            s->offsets[slot] = s16le(row + 4);
            s->window_flags[slot] = u16le(row + 6) ? 72u : 64u;
            continue;
        }
        case 0x3e:
            if (arg != UINT32_C(0x80184a34) || s->outcome < 2) goto invalid;
            s->waiting_score = 1;
            return BFM_ROOTRICK_RUNNING;
        case 0x4a: command.event = BFM_ROOTRICK_SHOW_HUD; break;
        case 0x4b: command.event = BFM_ROOTRICK_HIDE_HUD; break;
        case 0x5b:
            if (arg != UINT32_C(0x801849d4) || s->listening) goto invalid;
            s->listening = 1;
            if (!speech_gesture(flags, apply, context)) goto invalid;
            continue;
        case 0x5c:
            if (arg) goto invalid;
            s->listening = 0;
            continue;
        default: goto invalid;
        }
        if (!apply(context, &command)) goto invalid;
    }
invalid:
    s->failed = 1;
    return BFM_ROOTRICK_ERROR;
}

BfmRootrickResult bfm_woods_rootrick_challenge_step(BfmRootrickChallenge *s,
    int window_active, int actor_release, uint8_t flags[64], BfmRootrickApply apply, void *context)
{
    if (!s || s->outcome) return BFM_ROOTRICK_ERROR;
    return step(s, window_active, actor_release, 0, flags, apply, context);
}

BfmRootrickResult bfm_woods_rootrick_outcome_step(BfmRootrickChallenge *s,
    int window_active, int actor_release, int score_ready,
    uint8_t flags[64], BfmRootrickApply apply, void *context)
{
    if (!s || !s->outcome) return BFM_ROOTRICK_ERROR;
    return step(s, window_active, actor_release, score_ready, flags, apply, context);
}

int bfm_woods_rootrick_outcome_can_retire(const BfmRootrickChallenge *s, int score_ready)
{
    return s && s->outcome && s->terminal && !s->failed && (s->outcome != 1 || score_ready);
}
