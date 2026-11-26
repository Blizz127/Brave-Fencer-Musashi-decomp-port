/* SPDX-License-Identifier: MIT */
#include "palace_dialogue.h"
#include "dialogue_contour.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

enum { CORNERS = 30, CHAR_ROWS = 12, FIRST_CHAR = 32, CHAR_COUNT = 95 };
enum { SPEECH_COUNT = 20, ROUNDED = 2, BURST = 6, RIGHT_ROUNDED = 10 };
enum { CRYSTAL_CAMERA_FIELD = 2760, SCRIBE_FIELD = 3316,
       BUTLER_REACTION_FIELD = 4762, MUSASHI_RETORT_FIELD = 7082 };
enum { PORTRAIT_W = 32, PORTRAIT_H = 40, LINE_CAPACITY = 36 };
typedef BfmDialoguePoint Point;

typedef struct {
    uint8_t character, ink, x, y;
    unsigned tick;
} Letter;

typedef struct {
    Letter letters[LINE_CAPACITY * 3];
    unsigned length, voice_ticks, style, style_tick;
} Page;

typedef struct {
    unsigned bank, cue, ready_field;
    int ready;
} VoiceRequest;

typedef struct {
    uint16_t portrait[PORTRAIT_H][PORTRAIT_W];
    uint8_t name[LINE_CAPACITY];
    size_t name_size;
    unsigned portrait_id, style, hold_ticks;
    int panel_only, wait_for_voice;
    VoiceRequest voice, continuation_voice;
    Page lead, continuation;
} Speech;

struct PalaceDialogue {
    uint8_t glyph[CHAR_COUNT][CHAR_ROWS];
    Point contour[2][CORNERS], inset[2][CORNERS];
    uint16_t tail[16][16];
    uint16_t inks[16], outline, highlight, paper;
    Speech speech[SPEECH_COUNT];
};

static unsigned little16(const uint8_t *p)
{
    return p[0] | (unsigned)p[1] << 8;
}

static unsigned little32(const uint8_t *p)
{
    return little16(p) | little16(p + 2) << 16;
}

static int signed16(const uint8_t *p)
{
    unsigned n = little16(p);
    return n < 32768 ? (int)n : (int)n - 65536;
}

static int signed8(uint8_t n)
{
    return n < 128 ? n : (int)n - 256;
}

static int line_bytes(uint8_t *out, size_t *count, const uint8_t *scene,
                      size_t limit, size_t *position, uint8_t terminator)
{
    size_t size = 0;
    while (*position < limit && scene[*position] != terminator) {
        unsigned c = scene[(*position)++];
        if (size >= LINE_CAPACITY || c < FIRST_CHAR || c >= FIRST_CHAR + CHAR_COUNT)
            return 0;
        out[size++] = (uint8_t)c;
    }
    if (!size || *position == limit)
        return 0;
    ++*position;
    *count = size;
    return 1;
}

/* Markers begin timed phrases. A wait ends the phrase without clearing
 * the page; later text can continue on the same line or after a newline. */
static int read_page(Page *page, const uint8_t *scene, size_t size, size_t *position)
{
    size_t p = *position;
    unsigned x = 0, y = 0, ink = 1, tick = 0, deadline = 0;
    int speaker_name = 0;
    if (p >= size || (scene[p] != 0x14 && scene[p] != 9)) return 0;
    while (p < size) {
        unsigned c = scene[p++];
        if (tick > UINT_MAX - 65536u) return 0;
        if (c == 0x14) {
            if (size - p < 2) return 0;
            deadline = tick + little16(scene + p);
            p += 2;
            continue;
        }
        if (c == 9) {
            if (p == size || page->style || (scene[p] != ROUNDED && scene[p] != BURST)) return 0;
            page->style = scene[p++];
            page->style_tick = tick;
            continue;
        }
        if (c == 0x15) {
            if (tick < deadline) tick = deadline;
            if (p >= size) return 0;
            if (scene[p] == 3 || scene[p] == 8) {
                page->voice_ticks = tick;
                *position = p;
                return page->length != 0;
            }
            ++tick;
            continue;
        }
        if (c == 0x18) {
            speaker_name = 1;
            continue;
        }
        if (c == 1) {
            if (p >= size || scene[p] >= 16) return 0;
            ink = scene[p++];
        } else if (c == '\n') {
            x = 0; ++tick;
            speaker_name = 0;
            if (++y >= 3) return 0;
        } else {
            if (c < FIRST_CHAR || c >= FIRST_CHAR + CHAR_COUNT ||
                x >= LINE_CAPACITY || page->length >= sizeof(page->letters) / sizeof(Letter))
                return 0;
            page->letters[page->length++] = (Letter){(uint8_t)c, (uint8_t)ink, (uint8_t)x++, (uint8_t)y, tick};
            if (!speaker_name) ++tick;
        }
    }
    return 0;
}

static int read_script(PalaceDialogue *d, const uint8_t *scene, size_t size)
{
    size_t p = 0x60dfc;
    for (unsigned i = 0; i < SPEECH_COUNT; ++i) {
        Speech *speech = &d->speech[i];
        /* The neutral court script selects the short ritual declaration. */
        if (i == 7) p = 0x610b0;
        if (p >= size) return 0;
        /* A voice request may introduce a message. A following panel can
         * change the portrait balloon while the same recording continues. */
        speech->panel_only = scene[p] != 0x16;
        if (!i && speech->panel_only) return 0;
        if (!speech->panel_only) {
            if (size - p < 5) return 0;
            speech->voice.bank = scene[p + 2] & 127u;
            speech->voice.cue = little16(scene + p + 3);
            speech->wait_for_voice = speech->voice.cue && !(scene[p + 2] & 128u);
            p += 5;
        }
        if (size - p < 4 || scene[p] != 2 ||
            (scene[p + 1] != ROUNDED && scene[p + 1] != BURST &&
             scene[p + 1] != RIGHT_ROUNDED) || scene[p + 2] > 6) return 0;
        speech->style = scene[p + 1];
        speech->portrait_id = scene[p + 2];
        p += 3;
        if (scene[p] == 0x18) {
            ++p;
            if (!line_bytes(speech->name, &speech->name_size, scene, size, &p, '\n')) return 0;
        }
        if (p < size && scene[p] == 0x19) {
            speech->wait_for_voice = speech->voice.cue != 0;
            ++p;
        } else if (speech->name_size) return 0;
        if (!read_page(&speech->lead, scene, size, &p)) return 0;
        if (p < size && scene[p] == 8) {
            ++p;
            if (p < size && scene[p] == 0x0b) {
                if (size - p < 4 || (scene[p + 1] & 128u)) return 0;
                speech->continuation_voice.bank = scene[p + 1];
                speech->continuation_voice.cue = little16(scene + p + 2);
                if (!speech->continuation_voice.cue) return 0;
                p += 4;
            }
            if (!read_page(&speech->continuation, scene, size, &p)) return 0;
        }
        if (size - p < 3 || scene[p] != 3 || scene[p + 2] != 0) return 0;
        speech->hold_ticks = scene[p + 1];
        p += 3;
        /* Each message begins on a four-byte boundary. */
        while (p < size && p % 4) {
            if (scene[p++] != 0) return 0;
        }
    }
    return 1;
}

static int read_message_art(PalaceDialogue *d, const MusashiDiscMedia *disc,
                            const uint8_t *scene, const uint16_t vram[512][1024])
{
    enum { FONT = 0x24cbc, SHAPE = 0x257fc, TAIL = 0x2030, INSET = 0x5bdd0 };
    uint8_t entry[8], shape[2 * 4 * CORNERS], tail[15 * 64 + 8];
    size_t length;
    if (!musashi_disc_media_read_root_range(disc, "MAIN.CD", 8 + 8 * 10,
                                            entry, sizeof(entry), &length))
        return 0;
    unsigned sector = little32(entry), extent = little32(entry + 4);
    if (!sector || sector > length / 2048)
        return 0;
    size_t base = (size_t)sector * 2048;
    if (extent > length - base || extent < SHAPE + sizeof(shape) ||
        !musashi_disc_media_read_root_range(disc, "MAIN.CD", base + FONT,
                                            &d->glyph[0][0], sizeof(d->glyph), &length) ||
        !musashi_disc_media_read_root_range(disc, "MAIN.CD", base + SHAPE,
                                            shape, sizeof(shape), &length) ||
        !musashi_disc_media_read_root_range(disc, "MAIN.CD", base + TAIL,
                                            tail, sizeof(tail), &length))
        return 0;
    for (unsigned t = 0; t < 2; ++t) {
        for (unsigned i = 0; i < CORNERS; ++i) {
            unsigned vertex = t * CORNERS + i;
            const uint8_t *xy = &shape[4 * vertex];
            const uint8_t *delta = &scene[INSET + 2 * vertex];
            d->contour[t][i] = (Point){signed16(xy), signed16(xy + 2)};
            d->inset[t][i] = (Point){signed8(delta[0]), signed8(delta[1])};
        }
    }
    const uint16_t *bubble_colors = &vram[447][352];
    d->paper = bubble_colors[0]; d->highlight = bubble_colors[1];
    d->outline = bubble_colors[2];
    memcpy(d->inks, &vram[446][352], sizeof(d->inks));
    for (unsigned y = 0; y < 16; ++y)
        for (unsigned x = 0; x < 16; ++x)
            d->tail[y][x] = bubble_colors[(tail[y * 64 + x/2] >> (4 * (x & 1))) & 15];
    for (unsigned i = 0; i < SPEECH_COUNT; ++i) {
        Speech *speech = &d->speech[i];
        unsigned id = speech->portrait_id;
        if (!id) continue;
        /* Musashi uses the common scene atlas. The courtiers use the
         * portraits and palettes loaded for the palace. */
        unsigned left = id == 1 ? 624u : 440u + 8u * id;
        unsigned top = id == 1 ? 416u : 400u;
        unsigned palette_y = id == 1 ? 296u : 455u + id;
        for (unsigned y = 0; y < PORTRAIT_H; ++y)
            for (unsigned x = 0; x < PORTRAIT_W; ++x) {
                uint16_t pack = vram[top + y][left + x / 4];
                speech->portrait[y][x] = vram[palette_y][352 + ((pack >> (4 * (x & 3))) & 15)];
            }
    }
    return 1;
}

PalaceDialogue *bfm_palace_dialogue_load(const MusashiDiscMedia *disc,
                                         const uint8_t *scene, size_t size,
                                         const uint16_t vram[512][1024])
{
    if (!disc || !scene || !vram || size <= 0x60e04)
        return NULL;
    PalaceDialogue *d = calloc(1, sizeof(*d));
    if (!d || !read_script(d, scene, size) || !read_message_art(d, disc, scene, vram)) {
        free(d);
        return NULL;
    }
    return d;
}

void bfm_palace_dialogue_free(PalaceDialogue *dialogue)
{
    free(dialogue);
}

int bfm_palace_dialogue_voice_started(PalaceDialogue *d, unsigned bank, unsigned cue,
                                     unsigned opening_field)
{
    if (!d || opening_field > UINT_MAX - 6) return 0;
    for (unsigned i = 0; i < SPEECH_COUNT; ++i) {
        Speech *speech = &d->speech[i];
        VoiceRequest *requests[] = {&speech->voice, &speech->continuation_voice};
        for (unsigned j = 0; j < 2; ++j) {
            VoiceRequest *request = requests[j];
            if (!request->cue || request->cue != cue || request->bank != bank) continue;
            if (!request->ready) {
                request->ready_field = opening_field;
                request->ready = 1;
            }
            return 1;
        }
    }
    return 0;
}

static uint32_t rgb(uint16_t c)
{
    return UINT32_C(0xff000000) | ((c & 31u) * 255 / 31) << 16 |
           (((c >> 5) & 31u) * 255 / 31) << 8 | ((c >> 10) & 31u) * 255 / 31;
}

static int scaled(int v, int scale)
{
    int n = v * scale;
    return n < 0 ? -((-n + 4095) / 4096) : n / 4096;
}

static void put(uint32_t pixels[240][320], int x, int y, uint32_t color)
{
    if ((unsigned)x < 320 && y >= 10 && y < 230)
        pixels[y][x] = color;
}

static void edge_line(uint32_t pixels[240][320], Point a, Point b, uint32_t color)
{
    if (a.x > b.x) { Point tmp = a; a = b; b = tmp; }
    int dx = abs(b.x - a.x), dy = -abs(b.y - a.y);
    int sx = a.x < b.x ? 1 : -1, sy = a.y < b.y ? 1 : -1;
    int err = dx + dy;
    if (dx > -dy) err -= (dx - dy - 1) / 2 - (dx - 1) / 2;
    for (;;) {
        put(pixels, a.x, a.y, color);
        if (a.x == b.x && a.y == b.y) break;
        int twice = 2 * err;
        if (twice > dy) { err += dy; a.x += sx; }
        if (twice < dx) { err += dx; a.y += sy; }
    }
}

static void draw_balloon(const PalaceDialogue *d, unsigned style, unsigned step,
                         unsigned weight, unsigned portrait_id, uint32_t pixels[240][320])
{
    Point border[CORNERS], inset[CORNERS];
    int sx = (int)step * (portrait_id ? 7232 : 6170) / 8;
    int sy = (int)step * 4096 / 6;
    int low = 240, high = 0;
    for (unsigned i = 0; i < CORNERS; ++i) {
        Point pos = d->contour[0][i];
        if (weight) {
            Point target = d->contour[1][i];
            pos.x += scaled(target.x - pos.x, (int)weight * 1024);
            pos.y += scaled(target.y - pos.y, (int)weight * 1024);
        }
        Point delta = d->inset[weight >= 3][i];
        if (style == RIGHT_ROUNDED) {
            pos.x = -pos.x;
            delta.x = -delta.x;
        }
        Point p = {160 + scaled(pos.x, sx), 44 + scaled(pos.y, sy)};
        border[i] = p;
        inset[i] = (Point){p.x + delta.x, p.y + delta.y};
        if (low > p.y) low = p.y;
        if (high < p.y) high = p.y;
    }
    if (low < 10) low = 10;
    if (high > 230) high = 230;
    uint32_t paper = rgb(d->paper);
    for (int y = low; y < high; ++y) {
        int crossings[CORNERS], count = 0;
        for (unsigned i = 0; i < CORNERS; ++i) {
            Point a = border[i], b = border[(i + 1) % CORNERS];
            if ((y < a.y) == (y < b.y)) continue;
            if (a.y > b.y) { Point swap = a; a = b; b = swap; }
            int rise = b.y - a.y;
            int64_t along = (int64_t)(y - a.y) * (b.x - a.x);
            /* Include the left pixel and exclude the right edge. */
            int x = a.x + (int)(along / rise) + (along % rise > 0);
            int j = count++;
            while (j && crossings[j-1] > x) {
                crossings[j] = crossings[j-1]; --j;
            }
            crossings[j] = x;
        }
        for (int i = 1; i < count; i += 2)
            for (int x = crossings[i-1]; x < crossings[i]; ++x)
                put(pixels, x, y, paper);
    }
    for (unsigned i = 0; i < CORNERS; ++i)
        edge_line(pixels, inset[i], inset[(i + 1) % CORNERS], rgb(d->highlight));
    for (unsigned i = 0; i < CORNERS; ++i)
        edge_line(pixels, border[i], border[(i + 1) % CORNERS], rgb(d->outline));
}

static void draw_glyph(const PalaceDialogue *d, unsigned x, unsigned y, unsigned c,
                       unsigned ink, uint32_t pixels[240][320])
{
    const uint8_t *glyph = d->glyph[c - FIRST_CHAR];
    uint32_t color = rgb(d->inks[ink]);
    for (unsigned row = 0; row < CHAR_ROWS; ++row)
        for (unsigned bit = 0; bit < 6; ++bit)
            if (glyph[row] & (128u >> bit)) pixels[y + row][x + bit] = color;
}

static void draw_page(const PalaceDialogue *d, const Page *page, unsigned x, unsigned y,
                      unsigned visible, uint32_t pixels[240][320])
{
    for (unsigned i = 0; i < page->length && page->letters[i].tick < visible; ++i) {
        Letter l = page->letters[i];
        draw_glyph(d, x + 6u * l.x, y + 14u * l.y, l.character, l.ink, pixels);
    }
}

typedef struct {
    const Speech *speech;
    uint64_t opened, body, continuation_body, continuation_request, closing;
    int voice_ready;
} SpeechEvent;

/* Changing the contour retains the current text and takes four script ticks
 * at full balloon size. The first tick also prints the next letter. */
static void apply_page_style(const Page *page, uint64_t body, unsigned field,
                             unsigned *style, unsigned *weight)
{
    uint64_t change = body + 2ull * page->style_tick;
    if (!page->style || field < change) return;
    uint64_t ticks = (field - change) / 2 + 1;
    unsigned amount = ticks < 4 ? (unsigned)ticks : 4;
    if (page->style == BURST) {
        if (*weight < amount) *weight = amount;
    } else if (*weight > 4 - amount) {
        *weight = 4 - amount;
    }
    *style = page->style;
}

static void draw_speech(const PalaceDialogue *d, const SpeechEvent *event, unsigned field,
                        uint32_t pixels[240][320])
{
    const Speech *speech = event->speech;
    uint64_t elapsed = field - event->opened;
    unsigned step = elapsed < 12 ? 2 + (unsigned)elapsed / 2 : 8;
    unsigned style = speech->style;
    unsigned weight = style == BURST ? (step < 5 ? step - 1 : 4) : 0;
    int continuation_ready = !speech->continuation_voice.cue || speech->continuation_voice.ready;
    if (event->voice_ready) apply_page_style(&speech->lead, event->body, field, &style, &weight);
    if (continuation_ready)
        apply_page_style(&speech->continuation, event->continuation_body, field, &style, &weight);
    draw_balloon(d, style, step, weight, speech->portrait_id, pixels);
    int right = style == RIGHT_ROUNDED;
    unsigned text_x = right ? 32u : (speech->portrait_id ? 72u : 52u);
    if (step == 8 && !weight) {
        unsigned width = right ? 16u : (speech->portrait_id ? 15u : 13u);
        unsigned left = right ? 144u : 160u;
        for (unsigned y = 0; y < 11; ++y) {
            for (unsigned x = 0; x < width; ++x) {
                Point uv = right ? bfm_dialogue_reflected_texel(x, y, width, 11, 15) :
                                   (Point){(int)((x * 15 + width / 2) / width), (int)(y * 15 + 5) / 11};
                uint16_t color = d->tail[uv.y][uv.x];
                if (color) pixels[76+y][left+x] = rgb(color);
            }
        }
    }
    if (elapsed < 14) return;
    unsigned left = right ? 256u : 32u;
    for (unsigned y = 0; y < PORTRAIT_H; ++y) {
        for (unsigned x = 0; x < PORTRAIT_W; ++x) {
            Point uv = right ? bfm_dialogue_reflected_texel(x, y, PORTRAIT_W, PORTRAIT_H, PORTRAIT_H - 1) :
                               (Point){(int)x, (int)y};
            uint16_t color = speech->portrait[uv.y][uv.x];
            if (color) pixels[24+y][left+x] = rgb(color);
        }
    }
    if (elapsed < 16) return;
    uint64_t clear = event->body + 2ull * speech->lead.voice_ticks + 2;
    if (!event->voice_ready || !speech->continuation.length || field < clear) {
        for (unsigned i = 0; i < speech->name_size; ++i)
            draw_glyph(d, text_x + 6u * i, 24, speech->name[i], 1, pixels);
        if (event->voice_ready && field >= event->body)
            draw_page(d, &speech->lead, text_x, speech->name_size ? 38 : 24,
                      1 + (unsigned)(field - event->body) / 2, pixels);
    } else if (continuation_ready && field >= event->continuation_body) {
        draw_page(d, &speech->continuation, text_x, 24,
                  1 + (unsigned)(field - event->continuation_body) / 2, pixels);
    }
}

/* The room waits for each message to close. It counts 31 scene ticks before
 * requesting the second speaker, then requests the third as the next event.
 * Each script request precedes the first displayed balloon by six fields. */
static int speech_event(const PalaceDialogue *d, uint64_t field, SpeechEvent *event)
{
    uint64_t opened = 1790;
    for (unsigned i = 0; i < SPEECH_COUNT; ++i) {
        const Speech *speech = &d->speech[i];
        /* An early stream start can finish the previous message before the
         * crystal-room camera opens the next exchange. */
        if (i == 3 && opened < CRYSTAL_CAMERA_FIELD) opened = CRYSTAL_CAMERA_FIELD;
        if (i == 5 && opened < SCRIBE_FIELD) opened = SCRIBE_FIELD;
        if (i == 8 && opened < BUTLER_REACTION_FIELD) opened = BUTLER_REACTION_FIELD;
        /* Musashi lands before the close camera opens his reply. */
        if (i == 16 && opened < MUSASHI_RETORT_FIELD) opened = MUSASHI_RETORT_FIELD;
        uint64_t body = opened + (speech->wait_for_voice ? 32 : 16);
        int ready = !speech->wait_for_voice || speech->voice.ready || i == 0;
        if (speech->wait_for_voice && speech->voice.ready) {
            /* A fast host stream can start before the script restores normal
             * lettering after writing the portrait and name. */
            uint64_t display = (uint64_t)speech->voice.ready_field + 6;
            body = display > opened + 20 ? display : opened + 20;
        }
        int continues = i + 1 < SPEECH_COUNT && d->speech[i + 1].panel_only;
        uint64_t lead_end = body + 2ull * speech->lead.voice_ticks;
        uint64_t continuation_body = lead_end + 4;
        uint64_t continuation_request = continuation_body - 6;
        if (speech->continuation_voice.ready) {
            uint64_t audible_page = (uint64_t)speech->continuation_voice.ready_field + 6;
            if (continuation_body < audible_page) continuation_body = audible_page;
        }
        uint64_t end = speech->continuation.length ?
            continuation_body + 2ull * speech->continuation.voice_ticks : lead_end;
        uint64_t close = end + 2 * speech->hold_ticks;
        /* The two panels of one voiced announcement share the transport.
         * The first closes a script tick earlier and the next opens without
         * the intervening speaker's wait. */
        close += continues ? 2 : 4;
        /* The court waits two extra script ticks before Fillet begins the ritual. */
        unsigned pause = i == 0 ? 31u : i == 6 ? 3u : 1u;
        uint64_t next = close + 12 + (continues ? 0 : 2 * pause);
        if (!ready || (speech->continuation_voice.cue && !speech->continuation_voice.ready) ||
            field < next) {
            *event = (SpeechEvent){speech, opened, body, continuation_body, continuation_request, close, ready};
            return 1;
        }
        opened = next;
    }
    return 0;
}

int bfm_palace_dialogue_voice_schedule(const PalaceDialogue *d, unsigned field,
                                      unsigned *bank, unsigned *cue, unsigned *issue_field)
{
    SpeechEvent event;
    uint64_t display = (uint64_t)field + 6;
    if (!d || !bank || !cue || !speech_event(d, display, &event) || display < event.opened)
        return 0;
    const VoiceRequest *request = &event.speech->voice;
    uint64_t issue = event.opened - 6;
    if (request->cue && !request->ready && event.voice_ready && display >= event.closing) return 0;
    if (!request->cue || request->ready) {
        request = &event.speech->continuation_voice;
        issue = event.continuation_request;
        if (!request->cue || request->ready || field < issue) return 0;
    }
    *bank = request->bank;
    *cue = request->cue;
    if (issue_field) *issue_field = (unsigned)issue;
    return 1;
}

int bfm_palace_dialogue_voice_due(const PalaceDialogue *d, unsigned field,
                                 unsigned *bank, unsigned *cue)
{
    return bfm_palace_dialogue_voice_schedule(d, field, bank, cue, NULL);
}

void bfm_palace_dialogue_draw(const PalaceDialogue *d, unsigned field,
                              uint32_t pixels[240][320])
{
    SpeechEvent event;
    if (!d || !pixels || !speech_event(d, field, &event) || field < event.opened) return;
    if (!event.voice_ready || (event.speech->continuation_voice.cue && !event.speech->continuation_voice.ready) ||
        field < event.closing) {
        draw_speech(d, &event, field, pixels);
    } else if (field - event.closing < 4) {
        const Speech *speech = event.speech;
        unsigned style = speech->continuation.style ? speech->continuation.style :
                         speech->lead.style ? speech->lead.style : speech->style;
        unsigned step = field - event.closing < 2 ? 6 : 4;
        draw_balloon(d, style, step, style == BURST ? step / 2 : 0, speech->portrait_id, pixels);
    }
}
