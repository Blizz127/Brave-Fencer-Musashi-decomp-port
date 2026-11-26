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

int bfm_library_script_begin(BfmBedroomScript *s, const uint8_t *scene, size_t bytes)
{
    *s = (BfmBedroomScript){ .scene = scene, .size = bytes };
    if (!scene || !scene_range(s, 0x8019f498u, 8)) return 0;
    s->cursor = 0x8019f498u - scene_base;
    return 1;
}

/* The player and visitors continue updating while the room event waits for
 * movement, dialogue, or a delay from its mounted script. */
int bfm_bedroom_script_tick(BfmBedroomScript *s, int player_queue_idle, int message_active,
                            BfmBedroomApply apply, void *context)
{
    if (s->failed || !apply) return 0;
    if (s->finished) return 1;
    if ((s->waiting_for_message && message_active) || (s->delay && --s->delay) ||
        (s->waiting_for_player && !player_queue_idle)) return 1;
    if (s->waiting_for_score && !apply(context, BFM_BED_SOUND_READY, 0)) return 1;
    s->waiting_for_score = 0;
    s->waiting_for_player = 0;
    s->waiting_for_message = 0;
    for (unsigned budget = 0; budget < 128 && s->cursor <= s->size && s->size - s->cursor >= 8; ++budget) {
        const uint8_t *command = s->scene + s->cursor;
        uint32_t op = word(command), arg = word(command + 4);
        BfmBedroomEvent event;
        s->cursor += 8;
        switch (op) {
        case 0x02:
            s->finished = 1;
            if (!apply(context, BFM_BED_TRANSFER, arg)) goto invalid;
            return 1;
        case 0x0c: event = BFM_BED_FAREWELL; break;
        case 0x0d: event = BFM_BED_NEGATIVE_REPLY; break;
        case 0x20: event = BFM_BED_CAMERA_RELEASE; break;
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
            case 0x80182324u: event = BFM_BED_SHOW_FUSION; break;
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
            case 0x80180b34u: event = BFM_BED_BUTLER_RESTS; break;
            case 0x80180b44u: event = BFM_BED_SOUND_FADE; break;
            case 0x8017f294u: event = BFM_BED_BOOKS; break;
            case 0x8017f2b8u: event = BFM_BED_BOOKS_RESUME; break;
            case 0x80180ae8u: event = BFM_BED_HANDOVER; break;
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
        case 0x59: event = BFM_BED_REST; break;
        case 0x43:
            if (!apply(context, BFM_BED_SOUND_CHANGE, arg)) goto invalid;
            /* Thirty updates elapse before the first admission poll. */
            s->delay = 31;
            s->waiting_for_score = 1;
            return 1;
        case 0x3b:
            if (!scene_range(s, arg, 28)) goto invalid;
            event = BFM_BED_AWARD;
            break;
        case 0x0e: event = BFM_BED_COLLECT; break;
        case 0x4a: event = BFM_BED_HUD_SHOW; break;
        case 0x4b: event = BFM_BED_HUD_HIDE; break;
        case 0x36:
            if (s->size - s->cursor < 8 || word(s->scene + s->cursor) != 0) goto invalid;
            if (arg == 0x8017f6c8u) {
                /* Read returns the selected one-based book and chapter. */
                if (!s->book) {
                    s->cursor += 8;
                    continue;
                }
            } else if ((arg == 0x80180ad0u || arg == 0x8017f6b0u) && s->answer && s->answer <= 2) {
                if (s->answer == 1) {
                    s->cursor += 8;
                    continue;
                }
            } else goto invalid;
            arg = word(s->scene + s->cursor + 4);
            s->cursor += 8;
            /* fall through */
        case 0x34:
            if (!scene_range(s, arg, 8) || ((arg - scene_base) & 7u)) goto invalid;
            s->cursor = arg - scene_base;
            continue;
        case 0x41: event = BFM_BED_CUE; break;
        case 0x03: event = BFM_BED_WALK; break;
        case 0x05: event = BFM_BED_RETREAT; break;
        case 0x1d: event = BFM_BED_NOTICE; break;
        case 0x37: event = BFM_BED_VISITOR_LAYOUT; break;
        case 0x38: event = BFM_BED_PLAYER_LAYOUT; break;
        case 0x07: event = BFM_BED_TURN; break;
        case 0x39: event = BFM_BED_PROJECT_VISITOR; break;
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
