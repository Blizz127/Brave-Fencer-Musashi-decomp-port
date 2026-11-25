/* SPDX-License-Identifier: MIT */
#include "palace_dialogue.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

enum { CORNERS = 30, CHAR_ROWS = 12, FIRST_CHAR = 32, CHAR_COUNT = 95 };
enum { SPEECH_COUNT = 5, ROUNDED = 2, BURST = 6, CRYSTAL_CAMERA_FIELD = 2760 };
enum { PORTRAIT_W = 32, PORTRAIT_H = 40, LINE_CAPACITY = 36 };
typedef struct { int x, y; } Point;

typedef struct {
    uint8_t character, ink, x, y, tick;
} Letter;

typedef struct {
    Letter letters[LINE_CAPACITY * 3];
    unsigned length, voice_ticks;
} Page;

typedef struct {
    uint16_t portrait[PORTRAIT_H][PORTRAIT_W];
    uint8_t name[LINE_CAPACITY];
    size_t name_size;
    unsigned portrait_id, style, hold_ticks, bank, cue;
    int voiced;
    unsigned ready_field;
    int voice_ready;
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

/* The script supplies one voice duration per page. Inline palette changes
 * consume no character cell; a newline moves to the next row. */
static int read_page(Page *page, const uint8_t *scene, size_t size, size_t *position)
{
    size_t p = *position;
    unsigned x = 0, y = 0, ink = 1, tick = 0;
    if (p > size || size - p < 3 || scene[p++] != 0x14) return 0;
    page->voice_ticks = little16(scene + p);
    p += 2;
    while (p < size) {
        unsigned c = scene[p++];
        if (c == 0x15) {
            *position = p;
            return page->length != 0;
        }
        if (c == 1) {
            if (p >= size || scene[p] >= 16) return 0;
            ink = scene[p++];
        } else if (c == '\n') {
            x = 0; ++tick;
            if (++y >= 3) return 0;
        } else {
            if (c < FIRST_CHAR || c >= FIRST_CHAR + CHAR_COUNT ||
                x >= LINE_CAPACITY || page->length >= sizeof(page->letters) / sizeof(Letter))
                return 0;
            page->letters[page->length++] = (Letter){(uint8_t)c, (uint8_t)ink, (uint8_t)x++, (uint8_t)y, (uint8_t)tick++};
        }
    }
    return 0;
}

static int read_script(PalaceDialogue *d, const uint8_t *scene, size_t size)
{
    size_t p = 0x60dfc;
    for (unsigned i = 0; i < SPEECH_COUNT; ++i) {
        Speech *speech = &d->speech[i];
        if (p >= size || size - p < 9 || scene[p] != 0x16 || scene[p + 5] != 2 ||
            (scene[p + 6] != ROUNDED && scene[p + 6] != BURST) || scene[p + 7] > 6) return 0;
        speech->portrait_id = scene[p + 7];
        speech->style = scene[p + 6];
        speech->voiced = (scene[p + 2] & 128u) != 0;
        speech->bank = scene[p + 2] & 127u;
        speech->cue = little16(scene + p + 3);
        p += 8;
        if (scene[p] == 0x18) {
            ++p;
            if (!line_bytes(speech->name, &speech->name_size, scene, size, &p, '\n')) return 0;
        }
        if (p >= size || scene[p++] != 0x19 || !read_page(&speech->lead, scene, size, &p)) return 0;
        if (p < size && scene[p] == 8) {
            ++p;
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
        for (unsigned y = 0; y < PORTRAIT_H; ++y)
            for (unsigned x = 0; x < PORTRAIT_W; ++x) {
                uint16_t pack = vram[400 + y][440 + 8 * id + x / 4];
                speech->portrait[y][x] = vram[455 + id][352 + ((pack >> (4 * (x & 3))) & 15)];
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
        if (!speech->voiced || speech->bank != bank || speech->cue != cue) continue;
        if (!speech->voice_ready) {
            speech->ready_field = opening_field;
            speech->voice_ready = 1;
        }
        return 1;
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
                         int closing, uint32_t pixels[240][320])
{
    Point border[CORNERS], inset[CORNERS];
    unsigned weight = 0;
    if (style == BURST) weight = closing ? step / 2 : (step < 5 ? step - 1 : 4);
    int sx = (int)step * 904, sy = (int)step * 4096 / 6;
    int low = 240, high = 0;
    for (unsigned i = 0; i < CORNERS; ++i) {
        Point pos = d->contour[0][i];
        if (weight) {
            Point target = d->contour[1][i];
            pos.x += scaled(target.x - pos.x, (int)weight * 1024);
            pos.y += scaled(target.y - pos.y, (int)weight * 1024);
        }
        Point p = {160 + scaled(pos.x, sx), 44 + scaled(pos.y, sy)};
        Point delta = d->inset[weight >= 3][i];
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

static void draw_page(const PalaceDialogue *d, const Page *page, unsigned y,
                      unsigned visible, uint32_t pixels[240][320])
{
    for (unsigned i = 0; i < page->length && page->letters[i].tick < visible; ++i) {
        Letter l = page->letters[i];
        draw_glyph(d, 72u + 6u * l.x, y + 14u * l.y, l.character, l.ink, pixels);
    }
}

static void draw_speech(const PalaceDialogue *d, const Speech *speech, unsigned elapsed,
                        unsigned body, int voice_ready, uint32_t pixels[240][320])
{
    unsigned step = elapsed < 12 ? 2 + elapsed / 2 : 8;
    draw_balloon(d, speech->style, step, 0, pixels);
    if (step == 8 && speech->style != BURST)
        for (unsigned y = 0; y < 11; ++y)
            for (unsigned x = 0; x < 15; ++x) {
                uint16_t color = d->tail[(y*15+5)/11][x];
                if (color) pixels[76+y][160+x] = rgb(color);
            }
    if (elapsed < 14) return;
    for (unsigned y = 0; y < PORTRAIT_H; ++y)
        for (unsigned x = 0; x < PORTRAIT_W; ++x)
            if (speech->portrait[y][x]) pixels[24+y][32+x] = rgb(speech->portrait[y][x]);
    if (elapsed < 16) return;
    unsigned next = 2 * speech->lead.voice_ticks;
    if (!voice_ready || elapsed < body || !speech->continuation.length || elapsed - body < next + 2) {
        for (unsigned i = 0; i < speech->name_size; ++i)
            draw_glyph(d, 72u + 6u * i, 24, speech->name[i], 1, pixels);
        if (voice_ready && elapsed >= body)
            draw_page(d, &speech->lead, speech->name_size ? 38 : 24, 1 + (elapsed - body) / 2, pixels);
    } else if (elapsed - body >= next + 4) {
        draw_page(d, &speech->continuation, 24, 1 + (elapsed - body - next - 4) / 2, pixels);
    }
}

typedef struct {
    const Speech *speech;
    uint64_t opened, body, closing;
    int voice_ready;
} SpeechEvent;

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
        uint64_t body = opened + (speech->voiced ? 32 : 16);
        int ready = !speech->voiced || speech->voice_ready || i == 0;
        if (speech->voice_ready) {
            /* A fast host stream can start before the script restores normal
             * lettering after writing the portrait and name. */
            uint64_t display = (uint64_t)speech->ready_field + 6;
            body = display > opened + 20 ? display : opened + 20;
        }
        uint64_t close = body + 2 * (speech->lead.voice_ticks + speech->hold_ticks) + 4;
        if (speech->continuation.length) close += 4 + 2 * speech->continuation.voice_ticks;
        uint64_t next = close + (i == 0 ? 12 + 2 * 31 : 14);
        if (!ready || field < next) {
            *event = (SpeechEvent){speech, opened, body, close, ready};
            return 1;
        }
        opened = next;
    }
    return 0;
}

int bfm_palace_dialogue_voice_due(const PalaceDialogue *d, unsigned field,
                                 unsigned *bank, unsigned *cue)
{
    SpeechEvent event;
    uint64_t display = (uint64_t)field + 6;
    if (!d || !bank || !cue || !speech_event(d, display, &event) || display < event.opened ||
        !event.speech->voiced || event.speech->voice_ready ||
        (event.voice_ready && display >= event.closing)) return 0;
    *bank = event.speech->bank;
    *cue = event.speech->cue;
    return 1;
}

void bfm_palace_dialogue_draw(const PalaceDialogue *d, unsigned field,
                              uint32_t pixels[240][320])
{
    SpeechEvent event;
    if (!d || !pixels || !speech_event(d, field, &event) || field < event.opened) return;
    if (!event.voice_ready || field < event.closing) {
        draw_speech(d, event.speech, (unsigned)(field - event.opened),
                    (unsigned)(event.body - event.opened), event.voice_ready, pixels);
    } else if (field - event.closing < 4) {
        draw_balloon(d, event.speech->style, field - event.closing < 2 ? 6 : 4, 1, pixels);
    }
}
