/* SPDX-License-Identifier: MIT */
#include "inn_greeting.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>
#include <string.h>

/* Both US inn modules contain the same conversation rows and text groups at
 * different load addresses. Keep the row and page offsets relative to their
 * own groups so each mounted module supplies its own pointers and artwork. */
typedef struct {
    uint32_t origin;
    size_t rows, pages, service_box, service_portrait;
} InnLayout;
static const InnLayout early_inn = {0x801ef468u, 0x40d4, 0x42f4, 0x99690, 0x98d28};
static const InnLayout illness_inn = {0x801e25e8u, 0x40bc, 0x4200, 0x949f8, 0x94090};

enum {
    FIRST_EXCHANGE = 0, RETURN_EXCHANGE = 0x70, FOLLOW_UP_EXCHANGE = 0x98,
    HOTELO_REPLIES = 0xb0, DECLINE_ROW = 0x11c, FAREWELL_ROW = 0x12c,
    MOTELO_WELCOME = 0x40, FOLLOW_UP_PROMPT = 0xd4, MOTELO_FOLLOW_UP = 0x108,
    MOTELO_EARLY_CHAT = 0x60c, MOTELO_LATE_CHAT = 0x658, SERVICE_QUESTION = 0xb88,
    VISITED = 276
};
enum GreetingPhase { TALK, ASK, CHAT, SERVICE, FOLLOW_UP, FAREWELL, DEPART, END_AT_COUNTER };

struct BfmInnGreeting {
    const InnLayout *layout;
    const uint8_t *module, *service_box, *service_portrait;
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
    size_t limit = talk->layout->pages + SERVICE_QUESTION;
    if (offset < talk->layout->pages || offset >= limit || limit > talk->bytes ||
        talk->module[offset] != 5) return 0;
    BfmForestMessage *next = bfm_field_message_borrow(talk->module + offset, limit - offset);
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
    size_t offset_in_module = talk->layout->rows + row;
    if (offset_in_module > talk->bytes || talk->bytes - offset_in_module < 8) return 0;
    const uint8_t *entry = talk->module + offset_in_module;
    uint32_t speaker = u32le(entry), pointer = u32le(entry + 4);
    if (speaker > 1) return 0;
    size_t offset;
    if (keeper_text && !speaker && !pointer) offset = keeper_text;
    else if (pointer >= talk->layout->origin) offset = pointer - talk->layout->origin;
    else return 0;
    if (!open_text(talk, offset, speaker == 1)) return 0;
    talk->row = row;
    return 1;
}

static int motelo(const BfmInnGreeting *talk)
{
    if (talk->layout != &illness_inn) return 0;
    uint32_t story = u32le(talk->record->progress + 8);
    return story >= 200 && (story < 300 || (talk->record->flags[250/8] & (1u<<(250%8))));
}

static BfmInnGreeting *open_counter(const InnLayout *layout, const uint8_t *module, size_t bytes,
    const uint8_t *resident, size_t resident_bytes, BfmSaveRecord *record)
{
    if (!module || !record || bytes < layout->pages + SERVICE_QUESTION + 4 ||
        !resident || resident_bytes < layout->service_box + 12) return NULL;
    BfmInnGreeting *talk = calloc(1, sizeof *talk);
    if (!talk) return NULL;
    talk->layout = layout;
    talk->module = module;
    talk->service_box = resident + layout->service_box;
    talk->service_portrait = resident + layout->service_portrait;
    talk->bytes = bytes;
    talk->record = record;
    int brother = motelo(talk);
    talk->first = !brother && !(record->flags[VISITED / 8] & (1u << (VISITED % 8)));
    size_t welcome = layout->pages + (brother ? MOTELO_WELCOME : 0);
    if (!open_row(talk, talk->first ? FIRST_EXCHANGE : RETURN_EXCHANGE, welcome)) {
        bfm_inn_greeting_free(talk);
        return NULL;
    }
    if (layout == &illness_inn && talk->first)
        record->flags[VISITED/8] |= 1u << (VISITED%8);
    return talk;
}

BfmInnGreeting *bfm_inn_greeting_open(const uint8_t *module, size_t bytes,
    const uint8_t *resident, size_t resident_bytes, BfmSaveRecord *record)
{
    return open_counter(&early_inn,module,bytes,resident,resident_bytes,record);
}

BfmInnGreeting *bfm_inn_greeting_open_illness(const uint8_t *module, size_t bytes,
    const uint8_t *resident, size_t resident_bytes, BfmSaveRecord *record)
{
    if (!module || bytes < 0x4060 || u32le(module+0x4014) != 0x2058 ||
        u32le(module+0x405c) != 0xa1052) return NULL;
    return open_counter(&illness_inn,module,bytes,resident,resident_bytes,record);
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
    size_t prompt = motelo(talk) ? MOTELO_FOLLOW_UP : FOLLOW_UP_PROMPT;
    if (!open_row(talk, FOLLOW_UP_EXCHANGE, talk->layout->pages + prompt)) return 0;
    talk->followed_up = 1;
    talk->phase = FOLLOW_UP;
    talk->age = 0;
    return 1;
}

int bfm_inn_greeting_finish_service(BfmInnGreeting *talk)
{
    return bfm_inn_greeting_service(talk) && ask_again(talk);
}

static int start_chat(BfmInnGreeting *talk, unsigned progress)
{
    /* On early visits the same counter choice offers new local news as
     * the story advances. The room owns each complete dialogue program. */
    static const unsigned next_topic[] = {20, 70, 100, 130};
    size_t page;
    if (motelo(talk) && progress < 600) {
        page = talk->layout->pages + (progress < 300 ? MOTELO_EARLY_CHAT : MOTELO_LATE_CHAT);
    } else {
        size_t row = 0;
        if (progress < 600) {
            while (row < sizeof next_topic / sizeof next_topic[0] && progress >= next_topic[row])
                ++row;
        } else if (progress < 900) {
            row = 7;
            if (progress >= 690) ++row;
            if (progress >= 740) ++row;
        } else {
            static const unsigned later_topics[] = {950,1000,1100,1200};
            while (row < sizeof later_topics / sizeof later_topics[0] && progress >= later_topics[row])
                ++row;
            row += 12;
        }
        uint32_t address = u32le(talk->module + talk->layout->rows + HOTELO_REPLIES + row * 4);
        if (address < talk->layout->origin) return 0;
        page = address - talk->layout->origin;
    }
    if (!open_text(talk,page,0)) return 0;
    if (progress == 930) {
        /* Discussing the missing profits advances this story step to 940. */
        talk->record->progress[8] = 940u & 255;
        talk->record->progress[9] = 940u >> 8;
    }
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
            if (talk->layout == &early_inn) talk->record->flags[VISITED / 8] |= (uint8_t)(1u << (VISITED % 8));
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
            if (progress < 200 || talk->layout == &illness_inn) return start_chat(talk, progress);
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

/* The room descriptions, price highlighting and keeper's reply belong to
 * the mounted room event. Its paid-room reply contains a room digit field. */
static int draw_service(const BfmInnGreeting *talk, int dialogue, const BfmForestMessageArt *art,
                        const uint16_t tiles[512][1024], uint32_t pixels[240][320])
{
    static const size_t descriptions[] = {0xac,0xf8,0x140,0x190,0x1d0,0x1d0};
    size_t offset = 0;
    if (dialogue == BFM_INN_LOW_FUNDS) offset = 0x20;
    else if (dialogue == BFM_INN_PAID_DIRECTIONS) offset = 0x50;
    else if (dialogue >= BFM_INN_FIRST_ROOM_DESCRIPTION && dialogue < BFM_INN_PAID_DIRECTIONS)
        offset = descriptions[dialogue - BFM_INN_FIRST_ROOM_DESCRIPTION];
    offset += talk->layout->pages + SERVICE_QUESTION;
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
    return bfm_library_question_draw(art, talk->service_box, talk->service_portrait, tiles, message, bytes,
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
