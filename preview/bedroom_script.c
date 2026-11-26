/* SPDX-License-Identifier: MIT */
#include "bedroom_script.h"

enum { WAKE_SCRIPT = 0x93d40 };
static const uint32_t scene_base = UINT32_C(0x80128158);

static uint32_t word(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static int scene_range(const BfmBedroomScript *s, uint32_t address, size_t count)
{
    return address >= scene_base && count <= s->size && address - scene_base <= s->size - count;
}

int bfm_bedroom_script_begin(BfmBedroomScript *script, const uint8_t *scene, size_t bytes)
{
    if (!scene || bytes < WAKE_SCRIPT + 8u) return 0;
    *script = (BfmBedroomScript){ .scene = scene, .size = bytes, .cursor = WAKE_SCRIPT };
    return 1;
}

/* The normal earned save runs the early-story program until its first
 * conversation. Blocking player actions continue to tick alongside it. */
int bfm_bedroom_script_tick(BfmBedroomScript *s, int player_queue_idle,
                            BfmBedroomApply apply, void *context)
{
    if (s->failed || !apply) return 0;
    if (s->reached_dialogue || (s->delay && --s->delay) ||
        (s->waiting_for_player && !player_queue_idle)) return 1;
    s->waiting_for_player = 0;
    while (s->cursor <= s->size && s->size - s->cursor >= 8) {
        const uint8_t *command = s->scene + s->cursor;
        uint32_t op = word(command), arg = word(command + 4);
        BfmBedroomEvent event;
        s->cursor += 8;
        switch (op) {
        case 0x22:
        case 0x21:
            if (!scene_range(s, arg, 16)) goto invalid;
            event = op == 0x22 ? BFM_BED_CAMERA_CUT : BFM_BED_CAMERA_MOVE;
            break;
        case 0x49:
            switch (arg) {
            case 0x8017be9cu: event = BFM_BED_LIE; break;
            case 0x8017c128u: event = BFM_BED_RISE; break;
            case 0x8017c240u: event = BFM_BED_YAWN; break;
            default: goto invalid;
            }
            break;
        case 0x33:
            if (arg != 0x801807d8u) goto invalid;
            event = BFM_BED_UPRIGHT;
            break;
        case 0x3f:
            if (arg > 0x7fffffffu) goto invalid;
            s->delay = arg;
            return 1;
        case 0x40:
            s->waiting_for_player = 1;
            return 1;
        case 0x41: event = BFM_BED_CUE; break;
        case 0x03: event = BFM_BED_WALK; break;
        case 0x07: event = BFM_BED_TURN; break;
        case 0x3a: event = BFM_BED_WINDOW_POSITION; break;
        case 0x01:
            if (!scene_range(s, arg, 1)) goto invalid;
            s->reached_dialogue = 1;
            event = BFM_BED_WINDOW;
            break;
        default: goto invalid;
        }
        if (!apply(context, event, arg)) goto invalid;
        if (s->reached_dialogue) return 1;
    }
invalid:
    s->failed = 1;
    return 0;
}
