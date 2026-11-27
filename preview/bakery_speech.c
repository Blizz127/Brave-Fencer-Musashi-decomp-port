/* SPDX-License-Identifier: MIT */
#include "bakery_speech.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    FIRST_HELLO = 0xb8c, RETURN_HELLO = 0xbf4, ASK_AGAIN = 0xc14,
    TOPICS = 0xc2c, FIRST_DECLINE = 0xc98, GOODBYE = 0xca8,
    GREETING = 0xd9c, SALE_GREETING = 0xdcc, SERVICE_TEXT = 0x1728,
    JAM_VISITED = 270
};

enum BakeryPhase { CONVERSATION, QUESTION, CHAT, SHOPPING, FAREWELL, RETURN_TO_FIELD, LEAVING };
static const uint32_t event_base = UINT32_C(0x801ef468);

struct BfmBakerySpeech {
    const uint8_t *event;
    BfmSaveRecord *record;
    BfmForestMessage *message;
    BfmForestMessageFrame update;
    size_t bytes, row;
    enum BakeryPhase phase;
    BfmBakeryCaption caption;
    unsigned caption_item, caption_slot;
    int caption_tens;
    int hero, followed_up, updated;
    unsigned age, farewell_delay;
};

static int open_page(BfmBakerySpeech *s, uint32_t address, unsigned speaker)
{
    if (speaker > 1 || address < event_base + GREETING || address >= event_base + SERVICE_TEXT)
        return 0;
    size_t offset = address - event_base;
    if (s->event[offset] != 5) return 0;
    BfmForestMessage *next = bfm_field_message_borrow(s->event + offset, SERVICE_TEXT - offset);
    if (!next) return 0;
    bfm_forest_message_free(s->message);
    s->message = next;
    s->hero = (int)speaker;
    s->age = 0;
    return 1;
}

static int open_row(BfmBakerySpeech *s, size_t row)
{
    unsigned speaker = u32le(s->event + row);
    uint32_t address = u32le(s->event + row + 4);
    if (row == RETURN_HELLO && !address && !speaker) {
        size_t welcome = s->record->progress[0x39] < 18 ? GREETING : SALE_GREETING;
        address = event_base + (uint32_t)welcome;
    }
    if (!open_page(s, address, speaker)) return 0;
    s->row = row;
    return 1;
}

BfmBakerySpeech *bfm_bakery_speech_open(const uint8_t *module, size_t bytes, BfmSaveRecord *record)
{
    if (!module || !record || bytes < SERVICE_TEXT) return NULL;
    BfmBakerySpeech *s = calloc(1, sizeof *s);
    if (!s) return NULL;
    s->event = module;
    s->bytes = bytes;
    s->record = record;
    int visited = record->flags[JAM_VISITED/8] & (1u << (JAM_VISITED%8));
    if (!open_row(s, visited ? RETURN_HELLO : FIRST_HELLO)) {
        free(s);
        return NULL;
    }
    record->flags[JAM_VISITED/8] |= 1u << (JAM_VISITED%8);
    return s;
}

void bfm_bakery_speech_free(BfmBakerySpeech *s)
{
    if (!s) return;
    bfm_forest_message_free(s->message);
    free(s);
}

const BfmForestMessageFrame *bfm_bakery_speech_frame(const BfmBakerySpeech *s)
{
    return s ? bfm_forest_message_frame(s->message) : NULL;
}

const BfmForestMessageFrame *bfm_bakery_speech_update(const BfmBakerySpeech *s)
{
    return s && s->updated ? &s->update : NULL;
}

int bfm_bakery_speech_hero(const BfmBakerySpeech *s) { return s && s->hero; }
int bfm_bakery_speech_service(const BfmBakerySpeech *s) { return s && s->phase == SHOPPING; }
int bfm_bakery_speech_leaving(const BfmBakerySpeech *s) { return s && s->phase == LEAVING; }

static int ask_again(BfmBakerySpeech *s)
{
    if (!open_row(s, ASK_AGAIN)) return 0;
    s->followed_up = 1;
    s->phase = CONVERSATION;
    return 1;
}

int bfm_bakery_speech_finish_service(BfmBakerySpeech *s)
{
    return bfm_bakery_speech_service(s) && ask_again(s);
}

static int open_topic(BfmBakerySpeech *s)
{
    unsigned story = u32le(s->record->progress + 8);
    uint32_t page;
    /* The middle chapter gives Jam a separate line after Musashi has met her
     * father. Other chapters select a line from the bakery's topic list. */
    if (story >= 310 && story < 600) {
        page = event_base + 0x12cc;
    } else {
        static const unsigned early[] = {20, 70, 100, 130};
        static const unsigned middle[] = {300, 600, 690, 740};
        static const unsigned late[] = {950, 1000, 1100, 1200};
        const unsigned *thresholds = story < 200 ? early : story < 900 ? middle : late;
        size_t table = story < 200 ? TOPICS : story < 900 ? 0xc40 : 0xc5c;
        size_t index = 0;
        while (index < 4 && story >= thresholds[index]) ++index;
        page = u32le(s->event + table + index * 4);
    }
    if (!open_page(s, page, 0)) return 0;
    if (story == 930) {
        s->record->progress[8] = (uint8_t)940;
        s->record->progress[9] = (uint8_t)(940 >> 8);
    }
    s->phase = CHAT;
    return 1;
}

static int window_closed(BfmBakerySpeech *s, unsigned answer)
{
    switch (s->phase) {
    case CONVERSATION:
        if (!open_row(s, s->row + 8)) return 0;
        if (s->row == FIRST_HELLO + 24 || s->row == RETURN_HELLO + 8 || s->row == ASK_AGAIN + 8)
            s->phase = QUESTION;
        return 1;
    case QUESTION:
        if (answer == 2) return open_topic(s);
        if (answer == 3) {
            if (!open_row(s, s->followed_up ? GOODBYE : FIRST_DECLINE)) return 0;
            s->phase = FAREWELL;
            return 1;
        }
        if (answer != 1) return 0;
        s->phase = SHOPPING;
        s->caption = BFM_BAKERY_CAPTION_OFFER;
        break;
    case CHAT:
        return ask_again(s);
    case FAREWELL:
        s->phase = s->row == GOODBYE ? RETURN_TO_FIELD : LEAVING;
        break;
    default:
        return 0;
    }
    bfm_forest_message_free(s->message);
    s->message = NULL;
    s->age = 0;
    return 1;
}

int bfm_bakery_speech_step(BfmBakerySpeech *s, uint16_t pressed, unsigned voice_status)
{
    if (!s) return 0;
    s->updated = 0;
    ++s->age;
    if (s->phase == RETURN_TO_FIELD) {
        /* The counter releases its exit walk after two bubble-free fields. */
        if (s->age >= 2) s->phase = LEAVING;
        return 1;
    }
    if (!s->message) return s->phase == SHOPPING || s->phase == LEAVING;
    int direction = !!(pressed & 0x40) - !!(pressed & 0x10);
    int confirm = !!(pressed & 0x7000);
    if (s->farewell_delay) {
        confirm = --s->farewell_delay == 0;
    } else if (s->phase == FAREWELL && s->row == GOODBYE && confirm &&
               bfm_forest_message_frame(s->message)->wait_for_confirm) {
        /* Jam's final reply stays fully drawn for three fields after dismissal. */
        s->farewell_delay = 3;
        confirm = 0;
    }
    bfm_field_message_choose(s->message, direction, confirm && !(pressed & 0x4000), pressed & 0x4000);
    const BfmForestMessageFrame *frame = bfm_field_message_tick(s->message, voice_status,
                                                               confirm, pressed & 0x2000);
    if (!frame) return 0;
    s->update = *frame;
    s->updated = 1;
    return !(frame->events & BFM_FOREST_MESSAGE_CLOSED) || window_closed(s, frame->answer);
}

void bfm_bakery_speech_service_tick(BfmBakerySpeech *s, BfmBakeryCaption caption,
                                   unsigned item, unsigned sale_slot, int sale_tens)
{
    if (!bfm_bakery_speech_service(s)) return;
    s->updated = 0;
    int changed = s->caption != caption;
    if (caption == BFM_BAKERY_CAPTION_ITEM && s->caption_item != item)
        changed = 1;
    if (caption == BFM_BAKERY_CAPTION_SALE_OFFER &&
        (s->caption_slot != (sale_slot & 3u) || s->caption_tens != sale_tens))
        changed = 1;
    s->caption = caption;
    s->caption_item = item;
    s->caption_slot = sale_slot & 3u;
    s->caption_tens = sale_tens;
    /* A new caption shows its portrait before the first line appears. Buying
     * the same bread again keeps the existing acknowledgement on screen. */
    if (changed) s->age = 0;
    else if (s->age < 7) ++s->age;
}

/* The village's item directory points into the common player-data member. */
static const uint8_t *bakery_item_description(const uint8_t *scene, size_t scene_bytes,
                                              const uint8_t *main, size_t main_bytes,
                                              unsigned item, size_t *text_bytes)
{
    enum { DESCRIPTION_DIRECTORY = 0x66528, ITEM_ROWS = 120 };
    const uint32_t main_origin = UINT32_C(0x800cedf8);
    if (!text_bytes) return NULL;
    *text_bytes = 0;
    if (!scene || !main || item >= ITEM_ROWS) return NULL;
    size_t slot = DESCRIPTION_DIRECTORY + 4u * item;
    if (slot > scene_bytes || scene_bytes - slot < 4) return NULL;
    uint32_t address = u32le(scene + slot);
    if (address < main_origin) return NULL;
    size_t offset = address - main_origin;
    if (offset > main_bytes || main_bytes - offset < 4) return NULL;
    const uint8_t *text = main + offset;
    if (text[0] != 2 || text[1] || text[2]) return NULL;
    const uint8_t *end = memchr(text + 3, 0, main_bytes - offset - 3);
    if (!end) return NULL;
    *text_bytes = (size_t)(end - text) + 1;
    return text;
}

static int draw_service(const BfmBakerySpeech *s, BfmBakeryCaption caption, unsigned item,
                        unsigned sale_slot, int sale_tens, const BfmForestMessageArt *art,
                        const uint8_t *resident, size_t resident_bytes,
                        const uint8_t *main, size_t main_bytes, const uint16_t tiles[512][1024],
                        uint32_t pixels[240][320])
{
    enum { COUNTER_BOX = 0x99690, EMPTY_SLOT = 0x429f4 };
    if (!s || !resident || resident_bytes < COUNTER_BOX + 12) return 0;
    const uint8_t *text;
    uint8_t quote[48];
    size_t bytes;
    if (caption == BFM_BAKERY_CAPTION_ITEM) {
        text = bakery_item_description(resident, resident_bytes, main, main_bytes, item, &bytes);
        if (!text) return 0;
    } else if (caption == BFM_BAKERY_CAPTION_EMPTY) {
        if (!main || main_bytes < EMPTY_SLOT + 4 || main[EMPTY_SLOT+3]) return 0;
        text = main + EMPTY_SLOT;
        bytes = 4;
    } else if (caption == BFM_BAKERY_CAPTION_SALE_OFFER) {
        if (sale_slot >= 12) return 0;
        size_t offset = 0x17fc + 32 * (sale_slot & 3);
        if (offset > s->bytes || s->bytes - offset < 32) return 0;
        const uint8_t *line = s->event + offset;
        const uint8_t *end = memchr(line + 3, 0, 29);
        const uint8_t *digit = end ? memchr(line + 3, 14, (size_t)(end - line - 3)) : NULL;
        if (!digit) return 0;
        size_t prefix = (size_t)(digit - line), suffix = (size_t)(end - digit);
        memcpy(quote, line, prefix);
        int length = snprintf((char *)quote + prefix, sizeof quote - prefix, "%d", sale_tens);
        if (length < 0 || (size_t)length >= sizeof quote - prefix ||
            suffix > sizeof quote - prefix - (size_t)length) return 0;
        memcpy(quote + prefix + (size_t)length, digit + 1, suffix);
        text = quote;
        bytes = prefix + (size_t)length + suffix;
    } else {
        size_t offset;
        switch (caption) {
        case BFM_BAKERY_CAPTION_OFFER:  offset = SERVICE_TEXT; break;
        case BFM_BAKERY_CAPTION_BOUGHT: offset = 0x1740; break;
        case BFM_BAKERY_CAPTION_FULL:   offset = 0x1758; break;
        case BFM_BAKERY_CAPTION_POOR:   offset = 0x177c; break;
        case BFM_BAKERY_CAPTION_SOLD:    offset = 0x17ac; break;
        case BFM_BAKERY_CAPTION_REFUSED: offset = 0x17bc; break;
        default: return 0;
        }
        if (offset > s->bytes || s->bytes - offset < 4) return 0;
        text = s->event + offset;
        bytes = s->bytes - offset;
    }
    /* The unreflected counter portrait shows 31 columns by 39 rows of Jam's
     * room atlas. The field conversation uses the full portrait instead. */
    const int face_words[8] = {109 - 160, 31 - 120, 31, 39, (456 - 448) * 4,
                               400 - 256, 457 * 64 + 352 / 16, 16 + 448 / 64};
    uint8_t face[16];
    for (unsigned i = 0; i < 8; ++i) {
        uint16_t word = (uint16_t)face_words[i];
        face[2 * i] = (uint8_t)word;
        face[2 * i + 1] = (uint8_t)(word >> 8);
    }
    return bfm_library_question_draw(art, resident + COUNTER_BOX, face, tiles, text, bytes,
                                     s->age, pixels);
}

int bfm_bakery_speech_draw(const BfmBakerySpeech *s, BfmBakeryCaption caption, unsigned item,
                           unsigned sale_slot, int sale_tens, const BfmForestMessageArt *art,
                           const uint8_t *resident, size_t resident_bytes,
                           const uint8_t *main, size_t main_bytes, const uint16_t tiles[512][1024],
                           uint32_t pixels[240][320])
{
    if (!s || !art || !tiles || !pixels) return 0;
    if (s->phase == RETURN_TO_FIELD || bfm_bakery_speech_leaving(s)) return 1;
    if (bfm_bakery_speech_service(s))
        return draw_service(s, caption, item, sale_slot, sale_tens, art, resident, resident_bytes,
                            main, main_bytes, tiles, pixels);
    const BfmForestMessageFrame *f = bfm_bakery_speech_frame(s);
    if (!f) return 0;
    BfmForestMessageFrame shown = *f;
    if (s->age & 4u) shown.inline_prompt = 0;
    bfm_bedroom_message_art_draw_side(art, &shown, s->hero ? 187 : 170, s->hero, pixels);
    return 1;
}
