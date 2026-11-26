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

/* The player and visitors continue updating while the room event waits for
 * movement, dialogue, or a delay from its mounted script. */
int bfm_bedroom_script_tick(BfmBedroomScript *s, int player_queue_idle, int message_active,
                            BfmBedroomApply apply, void *context)
{
    if (s->failed || !apply) return 0;
    if ((s->waiting_for_message && message_active) || (s->delay && --s->delay) ||
        (s->waiting_for_player && !player_queue_idle)) return 1;
    s->waiting_for_player = 0;
    s->waiting_for_message = 0;
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
            switch (arg) {
            case 0x801807d8u: event = BFM_BED_UPRIGHT; break;
            case 0x80180a38u: event = BFM_BED_VISITORS_ENTER; break;
            case 0x80180b04u: event = BFM_BED_RIBSON_SPEAKS; break;
            case 0x80180b14u: event = BFM_BED_RIBSON_RESTS; break;
            case 0x80180b24u: event = BFM_BED_BUTLER_SPEAKS; break;
            default: goto invalid;
            }
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
        case 0x05: event = BFM_BED_RETREAT; break;
        case 0x1d: event = BFM_BED_NOTICE; break;
        case 0x37: event = BFM_BED_VISITOR_LAYOUT; break;
        case 0x38: event = BFM_BED_PLAYER_LAYOUT; break;
        case 0x07: event = BFM_BED_TURN; break;
        case 0x3a: event = BFM_BED_WINDOW_POSITION; break;
        case 0x00:
        case 0x01:
            if (!scene_range(s, arg, 1)) goto invalid;
            s->waiting_for_message = 1;
            event = op == 0x00 ? BFM_BED_VISITOR_WINDOW : BFM_BED_WINDOW;
            break;
        default: goto invalid;
        }
        if (!apply(context, event, arg)) goto invalid;
        if (s->waiting_for_message) return 1;
    }
invalid:
    s->failed = 1;
    return 0;
}
