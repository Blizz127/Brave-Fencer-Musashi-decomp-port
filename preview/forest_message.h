/* SPDX-License-Identifier: MIT */
#ifndef BFM_FOREST_MESSAGE_H
#define BFM_FOREST_MESSAGE_H

#include <stddef.h>
#include <stdint.h>

typedef struct BfmForestMessage BfmForestMessage;

typedef struct {
    uint8_t glyph, ink;
} BfmForestMessageCell;

enum {
    BFM_FOREST_FIRST_GESTURE  = 1u << 0,
    BFM_FOREST_SECOND_GESTURE = 1u << 1,
    BFM_FOREST_VOICE_REQUEST  = 1u << 2,
    BFM_FOREST_MESSAGE_CLOSED = 1u << 3,
    BFM_FOREST_VOICE_STOP     = 1u << 4
};

typedef struct {
    /* Zero glyph means an empty cell. Ink is the mounted text palette index. */
    BfmForestMessageCell cells[3][36];
    unsigned scroll, prompt_row, prompt_column, inline_prompt;
    unsigned choices, answer;
    unsigned events, voice_bank, voice_cue;
    /* Actor message flags 0x125 through 0x129 set during this update. */
    unsigned actor_flags;
    unsigned focused, text_visible, wait_for_confirm;
    /* Opening and closing size, and blend from round to pointed corners. */
    unsigned border_step, border_scale, jagged_mix, style, portrait;
} BfmForestMessageFrame;

/* Open on the update where the doorway gives focus to its message slot.
 * Copies the 160-byte mounted program. The first tick is the NEXT update. */
BfmForestMessage *bfm_forest_message_open(const uint8_t *program, size_t length);
/* Copies the field stream within its supplied mounted extent. */
BfmForestMessage *bfm_field_message_open(const uint8_t *program, size_t length);
/* The room keeps its mounted bytes alive until this cursor is freed. */
BfmForestMessage *bfm_field_message_borrow(const uint8_t *program, size_t length);
/* Apply fresh navigation/accept edges while the choice window owns input. */
int bfm_field_message_choose(BfmForestMessage *message, int direction, int last, int accept);
void bfm_forest_message_free(BfmForestMessage *message);
const BfmForestMessageFrame *bfm_forest_message_frame(const BfmForestMessage *message);

/* One 30 Hz game update. Supply the current voice service status (idle 0,
 * pending 1, ready 2 or 4) and the focused confirm button's rising edge.
 * Apply returned events once. The view belongs to the cursor until its next
 * update or destruction. NULL means an unsupported or malformed command. */
const BfmForestMessageFrame *bfm_forest_message_tick(BfmForestMessage *message,
                                                     unsigned voice_status,
                                                     int confirm_edge);

/* Pass the focused field window's reveal button edge in addition to any
 * accepted confirm edge. The voice owner consumes BFM_FOREST_VOICE_STOP. */
const BfmForestMessageFrame *bfm_field_message_tick(BfmForestMessage *message,
                                                    unsigned voice_status,
                                                    int confirm_edge,
                                                    int reveal_edge);

#endif
