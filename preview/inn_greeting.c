/* SPDX-License-Identifier: MIT */
#include "inn_greeting.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>
#include <string.h>

enum {
    FIRST_EXCHANGE = 0x40d4, RETURN_EXCHANGE = 0x4144,
    FOLLOW_UP_EXCHANGE = 0x416c, HOTELO_REPLIES = 0x4184,
    DECLINE_ROW = 0x41f0, FAREWELL_ROW = 0x4200,
    RETURN_WELCOME = 0x42f4, FOLLOW_UP_PROMPT = 0x43c8, SERVICE_QUESTION = 0x4e7c,
    SERVICE_BOX = 0x99690, VISITED = 276
};
static const uint32_t module_origin = UINT32_C(0x801ef468);
enum GreetingPhase { TALK, ASK, CHAT, SERVICE, FOLLOW_UP, FAREWELL, DEPART, END_AT_COUNTER };

struct BfmInnGreeting {
    const uint8_t *module, *service_box;
    size_t bytes, row;
    BfmSaveRecord *record;
    BfmForestMessage *message;
    BfmForestMessageFrame update;
    enum GreetingPhase phase;
    unsigned age;
    int hero, first, followed_up, updated;
};

static int open_text(BfmInnGreeting *talk, size_t offset, int hero)
{
    if (offset < RETURN_WELCOME || offset >= SERVICE_QUESTION || SERVICE_QUESTION > talk->bytes ||
        talk->module[offset] != 5) return 0;
    BfmForestMessage *next = bfm_field_message_borrow(talk->module + offset, SERVICE_QUESTION - offset);
    if (!next) return 0;
    bfm_forest_message_free(talk->message);
    talk->message = next;
    talk->hero = hero;
    return 1;
}

/* The first-visit sequence supplies both its speaker and its message. On
 * return the room fills in the keeper's message slot for its current host. */
static int open_row(BfmInnGreeting *talk, size_t row, size_t keeper_text)
{
    if (row > talk->bytes || talk->bytes - row < 8) return 0;
    const uint8_t *entry = talk->module + row;
    uint32_t speaker = u32le(entry), pointer = u32le(entry + 4);
    if (speaker > 1) return 0;
    size_t offset;
    if (keeper_text && !speaker && !pointer) offset = keeper_text;
    else if (pointer >= module_origin) offset = pointer - module_origin;
    else return 0;
    if (!open_text(talk, offset, speaker == 1)) return 0;
    talk->row = row;
    return 1;
}

BfmInnGreeting *bfm_inn_greeting_open(const uint8_t *module, size_t bytes,
                                     const uint8_t *resident, size_t resident_bytes, BfmSaveRecord *record)
{
    if (!module || !record || bytes < SERVICE_QUESTION + 4 ||
        !resident || resident_bytes < SERVICE_BOX + 12) return NULL;
    BfmInnGreeting *talk = calloc(1, sizeof *talk);
    if (!talk) return NULL;
    talk->module = module;
    talk->service_box = resident + SERVICE_BOX;
    talk->bytes = bytes;
    talk->record = record;
    talk->first = !(record->flags[VISITED / 8] & (1u << (VISITED % 8)));
    if (!open_row(talk, talk->first ? FIRST_EXCHANGE : RETURN_EXCHANGE, RETURN_WELCOME)) {
        bfm_inn_greeting_free(talk);
        return NULL;
    }
    return talk;
}

void bfm_inn_greeting_free(BfmInnGreeting *talk)
{
    if (!talk) return;
    bfm_forest_message_free(talk->message);
    free(talk);
}

int bfm_inn_greeting_service(const BfmInnGreeting *talk) { return talk && talk->phase == SERVICE; }
int bfm_inn_greeting_leaving(const BfmInnGreeting *talk) { return talk && talk->phase == DEPART; }
int bfm_inn_greeting_finished(const BfmInnGreeting *talk) { return talk && talk->phase == END_AT_COUNTER; }

static int ask_again(BfmInnGreeting *talk)
{
    if (!open_row(talk, FOLLOW_UP_EXCHANGE, FOLLOW_UP_PROMPT)) return 0;
    talk->followed_up = 1;
    talk->phase = FOLLOW_UP;
    talk->age = 0;
    return 1;
}

int bfm_inn_greeting_finish_service(BfmInnGreeting *talk)
{
    return bfm_inn_greeting_service(talk) && ask_again(talk);
}

static int start_hotelo_chat(BfmInnGreeting *talk, unsigned progress)
{
    /* On early visits the same counter choice offers new local news as
     * the story advances. The room owns each complete dialogue program. */
    static const unsigned next_topic[] = {20, 70, 100, 130};
    size_t row = 0;
    while (row < sizeof next_topic / sizeof next_topic[0] && progress >= next_topic[row])
        ++row;
    uint32_t address = u32le(talk->module + HOTELO_REPLIES + row * 4);
    if (address < module_origin || !open_text(talk, address - module_origin, 0)) return 0;
    talk->phase = CHAT;
    return 1;
}

const BfmForestMessageFrame *bfm_inn_greeting_frame(const BfmInnGreeting *talk)
{
    return talk ? bfm_forest_message_frame(talk->message) : NULL;
}

const BfmForestMessageFrame *bfm_inn_greeting_update(const BfmInnGreeting *talk)
{
    return talk && talk->updated ? &talk->update : NULL;
}

static int finish_window(BfmInnGreeting *talk, unsigned answer)
{
    switch (talk->phase) {
    case TALK: {
        int ready = !talk->first || talk->row == FIRST_EXCHANGE + 16;
        if (!open_row(talk, talk->row + 8, 0)) return 0;
        if (ready) {
            talk->phase = ASK;
            /* The completed first encounter is retained across Save/Continue. */
            talk->record->flags[VISITED / 8] |= (uint8_t)(1u << (VISITED % 8));
        }
        return 1;
    }
    case FOLLOW_UP:
        if (!open_row(talk, talk->row + 8, 0)) return 0;
        talk->phase = ASK;
        return 1;
    case CHAT:
        return ask_again(talk);
    case ASK:
        if (!answer || answer > 3) return 0;
        if (answer == 2) {
            unsigned progress = u32le(talk->record->progress + 8);
            if (progress < 200) return start_hotelo_chat(talk, progress);
        }
        if (answer == 3) {
            if (!open_row(talk, talk->followed_up ? FAREWELL_ROW : DECLINE_ROW, 0)) return 0;
            talk->phase = FAREWELL;
            return 1;
        }
        /* Later inn hosts use their own story-dependent conversations. */
        talk->phase = answer == 1 ? SERVICE : END_AT_COUNTER;
        break;
    case FAREWELL:
        talk->phase = DEPART;
        break;
    default:
        return 0;
    }
    bfm_forest_message_free(talk->message);
    talk->message = NULL;
    talk->age = 0;
    return 1;
}

int bfm_inn_greeting_step(BfmInnGreeting *talk, uint16_t pressed, unsigned voice_status)
{
    if (!talk) return 0;
    talk->updated = 0;
    ++talk->age;
    if (!talk->message)
        return talk->phase == SERVICE || talk->phase == END_AT_COUNTER || talk->phase == DEPART;
    int direction = !!(pressed & 0x40) - !!(pressed & 0x10);
    int confirm = !!(pressed & 0x7000);
    bfm_field_message_choose(talk->message, direction, confirm && !(pressed & 0x4000), pressed & 0x4000);
    const BfmForestMessageFrame *frame = bfm_field_message_tick(talk->message, voice_status,
                                                               confirm, pressed & 0x2000);
    if (!frame) return 0;
    talk->update = *frame;
    talk->updated = 1;
    if (frame->events & BFM_FOREST_MESSAGE_CLOSED) return finish_window(talk, frame->answer);
    return 1;
}

/* The counter portrait is unreflected. Its visible strip stops one row
 * above the full-height face used in field conversation. */
static void short_at(uint8_t *destination, int value)
{
    uint16_t word = (uint16_t)value;
    destination[0] = (uint8_t)word;
    destination[1] = (uint8_t)(word >> 8);
}

/* The room descriptions, price highlighting and keeper's reply belong to
 * the mounted room event. Its paid-room reply contains a room digit field. */
static int draw_service(const BfmInnGreeting *talk, int dialogue, const BfmForestMessageArt *art,
                        const uint16_t tiles[512][1024], uint32_t pixels[240][320])
{
    uint8_t face[16];
    short_at(face, 109 - 160);
    short_at(face + 2, 31 - 120);
    short_at(face + 4, 32);
    short_at(face + 6, 39);
    short_at(face + 8, (456 - 448) * 4);
    short_at(face + 10, 400 - 256);
    short_at(face + 12, 457 * 64 + 352 / 16);
    short_at(face + 14, 16 + 448 / 64);
    static const size_t descriptions[] = {0x4f28, 0x4f74, 0x4fbc, 0x500c, 0x504c, 0x504c};
    size_t offset = SERVICE_QUESTION;
    if (dialogue == BFM_INN_LOW_FUNDS) offset = 0x4e9c;
    else if (dialogue == BFM_INN_PAID_DIRECTIONS) offset = 0x4ecc;
    else if (dialogue >= BFM_INN_FIRST_ROOM_DESCRIPTION && dialogue < BFM_INN_PAID_DIRECTIONS)
        offset = descriptions[dialogue - BFM_INN_FIRST_ROOM_DESCRIPTION];
    if (offset > talk->bytes || talk->bytes - offset < 4) return 0;

    const uint8_t *message = talk->module + offset;
    size_t bytes = talk->bytes - offset;
    uint8_t directions[96];
    if (dialogue == BFM_INN_PAID_DIRECTIONS) {
        const uint8_t *end = memchr(message + 3, 0, bytes - 3);
        if (!end || (size_t)(end - message) >= sizeof directions) return 0;
        bytes = (size_t)(end - message) + 1;
        memcpy(directions, message, bytes);
        uint8_t *digit = memchr(directions + 3, 14, bytes - 3);
        if (!digit) return 0;
        *digit = '1';
        message = directions;
    }
    return bfm_library_question_draw(art, talk->service_box, face, tiles, message, bytes,
                                     talk->age, pixels);
}

int bfm_inn_greeting_draw(const BfmInnGreeting *talk, int dialogue, const BfmForestMessageArt *art,
                          const uint16_t tiles[512][1024], uint32_t pixels[240][320])
{
    if (!talk || !art || !tiles || !pixels) return 0;
    if (talk->phase == SERVICE) return draw_service(talk, dialogue, art, tiles, pixels);
    if (talk->phase == END_AT_COUNTER || talk->phase == DEPART) return 1;
    const BfmForestMessageFrame *frame = bfm_inn_greeting_frame(talk);
    if (!frame) return 0;
    BfmForestMessageFrame shown = *frame;
    if (talk->age & 4u) shown.inline_prompt = 0;
    /* Both speakers remain at their fixed counter positions for these lines. */
    bfm_bedroom_message_art_draw_side(art, &shown, talk->hero ? 172 : 187, !talk->hero, pixels);
    return 1;
}
