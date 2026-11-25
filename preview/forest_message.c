/* SPDX-License-Identifier: MIT */
#include "forest_message.h"

#include <stdlib.h>
#include <string.h>

enum MessagePhase {
    PRINT, OPENING, WAIT_VOICE, WAIT_TIMER, WAIT_CONFIRM, CLOSING, CLOSED, INVALID
};

struct BfmForestMessage {
    BfmForestMessageFrame frame;
    uint8_t program[160];
    size_t cursor;
    enum MessagePhase phase;
    unsigned row, column, ink;
    unsigned timer, delay, glyph_interval;
    int speaker_name, voiced_page;
};

BfmForestMessage *bfm_forest_message_open(const uint8_t *program, size_t length)
{
    BfmForestMessage *message;
    if (!program || length != sizeof(message->program))
        return NULL;
    message = calloc(1, sizeof(*message));
    if (message) {
        memcpy(message->program, program, length);
        message->frame.focused = 1;
        message->ink = 1;
    }
    return message;
}

void bfm_forest_message_free(BfmForestMessage *message)
{
    free(message);
}

const BfmForestMessageFrame *bfm_forest_message_frame(const BfmForestMessage *message)
{
    return message && message->phase != INVALID ? &message->frame : NULL;
}

static int set_style(BfmForestMessage *message, unsigned style)
{
    if (style != 2 && style != 6)
        return 0;
    message->frame.style = style;
    return 1;
}

/* Consume nonprinting commands together. A body glyph, line break or page
 * transition ends this update. Speaker names fill their line immediately. */
static int print_update(BfmForestMessage *m)
{
    BfmForestMessageFrame *f = &m->frame;
    while (m->cursor < sizeof(m->program)) {
        unsigned op = m->program[m->cursor++];
        const uint8_t *arg = m->program + m->cursor;
        size_t left = sizeof(m->program) - m->cursor;
        switch (op) {
        case 0x01: /* Ink */
            if (left < 1 || (arg[0] != 1 && arg[0] != 2)) return 0;
            m->ink = arg[0];
            ++m->cursor;
            break;
        case 0x02: /* Open dialogue border */
            if (left < 2 || !set_style(m, arg[0]) || arg[1] != 1) return 0;
            m->cursor += 2;
            f->border_step = 2;
            m->phase = OPENING;
            return 1;
        case 0x03: /* The closing pass keeps the cursor on this command. */
            if (left < 1 || arg[0] != 0) return 0;
            --m->cursor;
            m->phase = CLOSING;
            return 1;
        case 0x04: /* Extra updates between body glyphs */
            if (left < 1) return 0;
            m->glyph_interval = arg[0];
            ++m->cursor;
            break;
        case 0x05: /* Begin a focused message */
            break;
        case 0x07:
            if (!m->voiced_page) {
                f->wait_for_confirm = 1;
                m->phase = WAIT_CONFIRM;
                return 1;
            }
            break;
        case 0x08:
            memset(f->cells, 0, sizeof(f->cells));
            m->row = m->column = 0;
            return 1;
        case 0x09:
            if (left < 1 || !set_style(m, arg[0])) return 0;
            ++m->cursor;
            break;
        case 0x0a:
            if (m->row + 1 >= 3) return 0;
            ++m->row;
            m->column = 0;
            return 1;
        case 0x0b: /* Request voice. The high parameter bit permits layout
                    * to continue before the explicit synchronization point. */
            if (left < 3) return 0;
            f->voice_bank = arg[0] & 0x7f;
            f->voice_cue = arg[1] | ((unsigned)arg[2] << 8);
            f->events |= BFM_FOREST_VOICE_REQUEST;
            m->cursor += 3;
            m->voiced_page = 1;
            if (!(arg[0] & 0x80)) {
                m->phase = WAIT_VOICE;
                return 1;
            }
            break;
        case 0x10: {
            if (left < 2) return 0;
            unsigned event = arg[0] | ((unsigned)arg[1] << 8);
            if (event == 0x125)
                f->events |= BFM_FOREST_FIRST_GESTURE;
            else if (event == 0x126)
                f->events |= BFM_FOREST_SECOND_GESTURE;
            else
                return 0;
            m->cursor += 2;
            break;
        }
        case 0x14:
            if (left < 2) return 0;
            m->timer = arg[0] | ((unsigned)arg[1] << 8);
            m->cursor += 2;
            break;
        case 0x15:
            m->phase = WAIT_TIMER;
            return 1;
        case 0x18:
            m->speaker_name = 1;
            break;
        case 0x19:
            m->speaker_name = 0;
            if (m->voiced_page) {
                m->phase = WAIT_VOICE;
                return 1;
            }
            break;
        default:
            if (op < 0x20 || op > 0x7e || m->column >= 36)
                return 0;
            f->cells[m->row][m->column].glyph = (uint8_t)op;
            f->cells[m->row][m->column++].ink = (uint8_t)m->ink;
            if (!m->speaker_name) {
                m->delay = m->glyph_interval;
                return 1;
            }
            break;
        }
    }
    return 0;
}

const BfmForestMessageFrame *bfm_forest_message_tick(BfmForestMessage *m,
                                                     unsigned voice_status,
                                                     int confirm_edge)
{
    if (!m || m->phase == INVALID)
        return NULL;
    BfmForestMessageFrame *f = &m->frame;
    f->events = 0;
    if (m->timer)
        --m->timer;
    switch (m->phase) {
    case PRINT:
        if (m->delay)
            --m->delay;
        else if (!print_update(m)) {
            m->phase = INVALID;
            return NULL;
        }
        break;
    case OPENING:
        if (f->border_step < 8)
            ++f->border_step;
        else {
            m->phase = PRINT;
            f->text_visible = 1;
        }
        break;
    case WAIT_VOICE:
        if (voice_status == 0) {
            m->voiced_page = 0;
            m->phase = PRINT;
        } else if (voice_status == 2 || voice_status == 4) {
            m->phase = PRINT;
        }
        break;
    case WAIT_TIMER:
        if (m->timer == 0)
            m->phase = PRINT;
        break;
    case WAIT_CONFIRM:
        if (confirm_edge) {
            f->wait_for_confirm = 0;
            m->phase = PRINT;
        }
        break;
    case CLOSING:
        f->text_visible = 0;
        if (!f->border_step) {
            f->focused = 0;
            f->events |= BFM_FOREST_MESSAGE_CLOSED;
            m->phase = CLOSED;
        } else {
            f->border_step = f->border_step > 4 ? f->border_step - 2 : 0;
            f->style = 2;
        }
        break;
    case CLOSED:
    case INVALID:
        break;
    }
    f->border_scale = f->border_step * 512;
    if (f->border_step) {
        if (f->style == 6 && f->jagged_mix < 4)
            ++f->jagged_mix;
        else if (f->style == 2 && f->jagged_mix)
            --f->jagged_mix;
    }
    return f;
}
