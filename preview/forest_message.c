/* SPDX-License-Identifier: MIT */
#include "forest_message.h"

#include <stdlib.h>
#include <string.h>

enum { DOORWAY_BYTES = 160 };

enum MessagePhase {
    PRINT, OPENING, SCROLL, WAIT_VOICE, WAIT_TIMER, WAIT_CONFIRM, WAIT_DISMISS, BUILD_CHOICE, WAIT_CHOICE, NOTICE_HOLD, CLOSING, CLOSED, INVALID
};

struct BfmForestMessage {
    BfmForestMessageFrame frame;
    size_t size, cursor;
    enum MessagePhase phase;
    unsigned row, column, ink;
    unsigned timer, delay, glyph_interval, close_delay;
    int speaker_name, voiced_page, rapid, voice_interrupted, sync_on_open;
    int notice, last_shortcut, voice_script;
    unsigned choice_count;
    const uint8_t *program, *insertion, *resume;
    size_t insertion_bytes, resume_bytes;
    uint8_t *saved_flags;
    int active_flag; /* Only a nonmodal notice owns a temporary flag. */
    int asserted_active_flag;
};

static BfmForestMessage *open_message(const uint8_t *program, size_t length, int copy)
{
    BfmForestMessage *message;
    if (!program || !length || length > SIZE_MAX - sizeof(*message))
        return NULL;
    message = calloc(1, sizeof(*message) + (copy ? length : 0));
    if (message) {
        if (copy) {
            uint8_t *owned = (uint8_t *)(message + 1);
            memcpy(owned, program, length);
            program = owned;
        }
        message->program = program;
        message->active_flag = -1;
        message->size = length;
        message->frame.focused = 1;
        message->frame.text_columns = 36;
        if (length >= 4 && program[0] == 0x16 && program[1] == 2 && program[2] == 1 && !program[3]) {
            message->frame.style = 1;
            message->frame.border_step = 1;
            message->frame.border_scale = 512;
        }
        message->ink = 1;
    }
    return message;
}

BfmForestMessage *bfm_field_message_open(const uint8_t *program, size_t length)
{
    return open_message(program, length, 1);
}

BfmForestMessage *bfm_field_message_borrow(const uint8_t *program, size_t length)
{
    return open_message(program, length, 0);
}

BfmForestMessage *bfm_field_message_borrow_flags(const uint8_t *program, size_t length,
                                                uint8_t flags[64])
{
    if (!flags) return NULL;
    BfmForestMessage *message = bfm_field_message_borrow(program, length);
    if (message) message->saved_flags = flags;
    return message;
}

BfmForestMessage *bfm_field_message_borrow_notice(const uint8_t *program, size_t length,
                                                uint8_t flags[64], unsigned active_flag)
{
    if (active_flag >= 64u * 8u) return NULL;
    BfmForestMessage *message = bfm_field_message_borrow_flags(program, length, flags);
    if (message) message->active_flag = (int)active_flag;
    return message;
}

int bfm_field_message_keeps_control(const BfmForestMessage *message)
{
    return message && message->active_flag >= 0;
}

int bfm_field_message_insert_text(BfmForestMessage *message, const uint8_t *text, size_t bytes)
{
    if (!message || !text || !bytes || message->insertion || message->resume) return 0;
    const uint8_t *end = memchr(text, 0, bytes);
    if (!end) return 0;
    message->insertion = text;
    message->insertion_bytes = (size_t)(end-text)+1;
    return 1;
}

BfmForestMessage *bfm_forest_message_open(const uint8_t *program, size_t length)
{
    if (length != DOORWAY_BYTES) return NULL;
    return bfm_field_message_open(program, length);
}

void bfm_forest_message_free(BfmForestMessage *message)
{
    if (message && message->asserted_active_flag) {
        unsigned flag = message->active_flag;
        message->saved_flags[flag / 8] &= (uint8_t)~(1u << (flag % 8));
    }
    free(message);
}

const BfmForestMessageFrame *bfm_forest_message_frame(const BfmForestMessage *message)
{
    return message && message->phase != INVALID ? &message->frame : NULL;
}

static int set_style(BfmForestMessage *message, unsigned style)
{
    if (style != 1 && style != 2 && style != 6)
        return 0;
    message->frame.style = style;
    return 1;
}

/* Consume nonprinting commands together. A body glyph, line break or page
 * transition ends this update. Speaker names fill their line immediately. */
static int print_update(BfmForestMessage *m)
{
    BfmForestMessageFrame *f = &m->frame;
    unsigned glyphs = 0;
    while (m->cursor < m->size) {
        unsigned op = m->program[m->cursor++];
        const uint8_t *arg = m->program + m->cursor;
        size_t left = m->size - m->cursor;
        switch (op) {
        case 0x00:
            if (m->resume) {
                m->program = m->resume;
                m->size = m->resume_bytes;
                m->cursor = 0;
                m->resume = NULL;
                break;
            }
            if (m->notice) return 0;
            f->wait_for_confirm = 1;
            m->phase = WAIT_DISMISS;
            return 1;
        case 0x01: /* Ink */
            if (left < 1 || arg[0] < 1 || arg[0] >= 16) return 0;
            m->ink = arg[0];
            ++m->cursor;
            break;
        case 0x02: { /* Open dialogue border */
            if (left < 2) return 0;
            unsigned layout = arg[0];
            unsigned style = layout;
            if (layout == 0xa2 || layout == 0xa6 || layout == 0xc2 ||
                layout == 0x4e || layout == 10 || layout == 14)
                style = layout & 7u;
            if (!set_style(m, style) || (arg[1] > 4 && !(layout == 0x0a && arg[1] == 9)) ||
                (layout == 1 ? arg[1] > 1 : !arg[1])) return 0;
            f->text_columns = 36 - 8 * ((layout >> 5) & 3u);
            /* Rectangular notices and signs have no portrait. */
            f->portrait = arg[0] == 1 ? 0 : arg[1];
            m->cursor += 2;
            f->border_step = 2;
            m->phase = OPENING;
            return 1;
        }
        case 0x03: /* The closing pass keeps the cursor on this command. */
            if (left < 1) return 0;
            /* Leno and Jon leave different delays before the closing border.
             * An interrupted voice skips the delay and the close command,
             * so the following byte still determines how the page ends. */
            int delayed_voice_close = m->voice_script &&
                (arg[0] == 1 || arg[0] == 10 || arg[0] == 20 || arg[0] == 30);
            if (delayed_voice_close && m->voice_interrupted) {
                ++m->cursor;
                break;
            }
            /* An unvoiced portrait reaction can end on a timed hold. */
            int timed_close = arg[0] && (m->notice || (!m->voice_script && arg[0] == 30));
            if (arg[0] && !timed_close && !delayed_voice_close) return 0;
            if (timed_close) {
                m->timer = arg[0];
                m->phase = NOTICE_HOLD;
                return 1;
            }
            if (delayed_voice_close) {
                m->close_delay = arg[0];
                m->rapid = 0;
            }
            --m->cursor;
            f->wait_for_confirm = m->voice_interrupted;
            m->phase = m->voice_interrupted ? WAIT_DISMISS : CLOSING;
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
                f->inline_prompt = 1;
                f->prompt_row = m->row;
                f->prompt_column = m->column < f->text_columns ? m->column : f->text_columns - 1;
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
            m->column = 0;
            if (m->row < 2)
                ++m->row;
            else
                m->phase = SCROLL;
            return 1;
        case 0x0b: /* Request voice. The high parameter bit permits layout
                    * to continue before the explicit synchronization point. */
            if (left < 3) return 0;
            m->voice_script = 1;
            if (m->rapid) {
                m->cursor += 3;
                break;
            }
            m->cursor += 3;
            m->voiced_page = 1;
            /* A zero cue keeps this page voiced while the current recording
             * continues. The following text can print on this update. */
            if (!arg[1] && !arg[2]) break;
            f->voice_bank = arg[0] & 0x7f;
            f->voice_cue = arg[1] | ((unsigned)arg[2] << 8);
            f->events |= BFM_FOREST_VOICE_REQUEST;
            if (!(arg[0] & 0x80)) {
                /* An immediately following border opens during the seek.
                 * Inline cues synchronize before printing continues. */
                if (m->cursor < m->size && m->program[m->cursor] == 0x02) {
                    m->sync_on_open = 1;
                    break;
                }
                m->phase = WAIT_VOICE;
                return 1;
            }
            break;
        case 0x0f: /* Print the selected book entry, then resume its speaker. */
            if (!m->insertion || m->resume) return 0;
            m->resume = arg;
            m->resume_bytes = left;
            m->program = m->insertion;
            m->size = m->insertion_bytes;
            m->cursor = 0;
            m->insertion = NULL;
            break;
        case 0x10: /* Set saved flag */
        case 0x11: { /* Clear saved flag */
            if (left < 2) return 0;
            unsigned flag = arg[0] | ((unsigned)arg[1] << 8);
            int actor_event = op == 0x10 && flag >= 0x125 && flag <= 0x129;
            if (m->saved_flags) {
                if (flag >= 64u * 8u) return 0;
                uint8_t mask = (uint8_t)(1u << (flag % 8));
                if (op == 0x10) m->saved_flags[flag / 8] |= mask;
                else m->saved_flags[flag / 8] &= (uint8_t)~mask;
                if ((int)flag == m->active_flag) m->asserted_active_flag = op == 0x10;
            } else if (!actor_event) return 0;
            if (actor_event) {
                f->actor_flags |= 1u << (flag - 0x125);
                if (flag == 0x125)
                    f->events |= BFM_FOREST_FIRST_GESTURE;
                else if (flag == 0x126)
                    f->events |= BFM_FOREST_SECOND_GESTURE;
            }
            m->cursor += 2;
            break;
        }
        case 0x14:
            if (left < 2) return 0;
            if (!m->voice_interrupted)
                m->timer = arg[0] | ((unsigned)arg[1] << 8);
            m->cursor += 2;
            break;
        case 0x15:
            if (!m->timer) break;
            m->phase = WAIT_TIMER;
            return 1;
        case 0x16: /* System notices run through their own display timer. */
            m->notice = 1;
            break;
        case 0x17:
            m->last_shortcut = 1;
            break;
        case 0x0c: case 0x0d:
            m->choice_count = op == 0x0c ? 2 : 3;
            if (m->row + m->choice_count > 3) return 0;
            f->choice_row = m->row;
            m->phase = BUILD_CHOICE;
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
            if (op < 0x20 || (op > 0x7e && (op < 0x90 || op > 0x95)) || m->column >= f->text_columns)
                return 0;
            f->cells[m->row][m->column].glyph = (uint8_t)op;
            f->cells[m->row][m->column++].ink = (uint8_t)m->ink;
            ++glyphs;
            if ((!m->speaker_name && !m->rapid) || glyphs == 36) {
                m->delay = m->glyph_interval;
                return 1;
            }
            break;
        }
    }
    return 0;
}

/* Reveal takes effect after this update's ordinary printing. A voice cut
 * leaves its page under button control through the final dismissal. */
static void reveal(BfmForestMessage *m, int pressed)
{
    if (!pressed || m->rapid || m->notice || m->phase == WAIT_CHOICE) return;
    m->rapid = 1;
    if (m->voiced_page) {
        m->voiced_page = 0;
        m->voice_interrupted = 1;
        m->frame.events |= BFM_FOREST_VOICE_STOP;
    }
}

/* Choice captions use a 36-glyph budget per update. The menu takes
 * input only after the mounted option terminator. */
static int build_choice(BfmForestMessage *m)
{
    for (unsigned glyphs = 0; glyphs < 36 && m->cursor < m->size;) {
        unsigned ch = m->program[m->cursor++];
        if (ch >= 0x20 && ch <= 0x7e) {
            if (m->column == m->frame.text_columns) return 0;
            m->frame.cells[m->row][m->column++] = (BfmForestMessageCell){(uint8_t)ch, (uint8_t)m->ink};
            ++glyphs;
        } else if (ch == 0x01) {
            if (m->cursor == m->size) return 0;
            unsigned ink = m->program[m->cursor++];
            if (ink < 1 || ink >= 16) return 0;
            m->ink = ink;
        } else if (ch == 0x0a && m->row + 1 < m->frame.choice_row + m->choice_count) {
            ++m->row;
            m->column = 0;
        } else if (ch == 0 || ch == 7) {
            if (m->row + 1 != m->frame.choice_row + m->choice_count || !m->column) return 0;
            m->frame.choices = m->choice_count;
            m->frame.answer = 1;
            m->phase = WAIT_CHOICE;
            return 1;
        } else if (ch != 0x15 && ch != 3) return 0;
    }
    return m->cursor < m->size;
}

int bfm_field_message_choose(BfmForestMessage *m, int direction, int last, int accept)
{
    if (!m || m->phase != WAIT_CHOICE) return 0;
    BfmForestMessageFrame *f = &m->frame;
    if (accept) m->phase = CLOSING;
    else if (direction < 0) f->answer = f->answer > 1 ? f->answer-1 : f->choices;
    else if (direction > 0) f->answer = f->answer < f->choices ? f->answer+1 : 1;
    else if (last && m->last_shortcut) f->answer = f->choices;
    else return 0;
    return 1;
}

const BfmForestMessageFrame *bfm_field_message_tick(BfmForestMessage *m,
                                                     unsigned voice_status,
                                                     int confirm_edge,
                                                     int reveal_edge)
{
    if (!m || m->phase == INVALID)
        return NULL;
    BfmForestMessageFrame *f = &m->frame;
    f->events = 0;
    f->actor_flags = 0;
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
        reveal(m, reveal_edge);
        break;
    case OPENING:
        if (f->border_step < 8)
            ++f->border_step;
        else {
            m->phase = m->sync_on_open ? WAIT_VOICE : PRINT;
            m->sync_on_open = 0;
            f->text_visible = 1;
        }
        break;
    case SCROLL:
        if (f->scroll == 9) {
            memmove(f->cells[0], f->cells[1], 2 * sizeof(f->cells[0]));
            memset(f->cells[2], 0, sizeof(f->cells[2]));
            f->scroll = 0;
            m->phase = PRINT;
        } else {
            f->scroll = f->scroll ? 9 : 4;
        }
        reveal(m, reveal_edge);
        break;
    case WAIT_VOICE:
        if (voice_status == 0) {
            m->voiced_page = 0;
            m->phase = PRINT;
        } else if (voice_status == 2 || voice_status == 4) {
            m->phase = PRINT;
        } else {
            reveal(m, reveal_edge);
            if (m->rapid) m->phase = PRINT;
        }
        break;
    case WAIT_TIMER:
        if (m->timer) {
            reveal(m, reveal_edge);
            if (m->rapid) {
                m->timer = 0;
                if (!m->voice_interrupted) m->rapid = 0;
            }
        }
        if (!m->timer) m->phase = PRINT;
        break;
    case WAIT_CONFIRM:
        if (confirm_edge) {
            f->inline_prompt = 0;
            f->wait_for_confirm = 0;
            m->phase = PRINT;
        }
        break;
    case BUILD_CHOICE:
        if (!build_choice(m)) {
            m->phase = INVALID;
            return NULL;
        }
        break;
    case WAIT_CHOICE:
        break;
    case NOTICE_HOLD:
        if (!m->timer) m->phase = CLOSING;
        break;
    case WAIT_DISMISS:
        if (!confirm_edge) break;
        f->wait_for_confirm = 0;
        m->phase = CLOSING;
        /* The border begins shrinking on the dismissal update. */
        /* fall through */
    case CLOSING:
        if (m->close_delay) {
            --m->close_delay;
            reveal(m, reveal_edge);
            if (!m->rapid) break;
            m->close_delay = 0;
        }
        f->text_visible = 0;
        if (!f->border_step) {
            f->focused = 0;
            f->events |= BFM_FOREST_MESSAGE_CLOSED;
            m->phase = CLOSED;
        } else {
            f->border_step = f->border_step > 4 ? f->border_step - 2 : 0;
            if (!m->notice && f->style != 1) f->style = 2;
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

const BfmForestMessageFrame *bfm_forest_message_tick(BfmForestMessage *m,
                                                      unsigned voice_status,
                                                      int confirm_edge)
{
    return bfm_field_message_tick(m, voice_status, confirm_edge, 0);
}
