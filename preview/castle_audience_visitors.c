/* SPDX-License-Identifier: MIT */
#include "castle_audience_visitors.h"
#include "library_books.h"
#include "../pc_port/forest_player_bytes.h"
#include <string.h>

enum {
    QUESTION, CHOICES, DECIDE, FINISHED,
    RESCUED_MENU, GUEST_LIST, GUEST_CALLED
};

enum {
    VISITOR_COUNT = 40, RESCUED_FIRST = 5, VISIBLE_ROWS = 6,
    CASTLE_VARIABLES = 0x5b8e8, GUEST_NAMES = 0x405d0,
    QUESTION_SCRIPT = 0xc1108, GOODBYE_SCRIPT = 0xc10e8,
    NUMERIC_PAGE = 0x5e4d8, NO_RESCUES_PAGE = 0x5e524,
    ALL_RESCUED_PAGE = 0x5e578,
    RESCUED_PANEL = 0xc0c68, LIST_PANEL = 0xc0c88
};
static const uint32_t scene_base = UINT32_C(0x80128158);
static const uint32_t main_base = UINT32_C(0x800cedf8);

static const uint8_t *span(const uint8_t *data, size_t size, size_t offset, size_t len)
{
    return data && offset <= size && len <= size-offset ? data+offset : NULL;
}

static unsigned saved_people(const BfmCastleAudienceVisitors *v)
{
    unsigned count = 0;
    for (unsigned i=RESCUED_FIRST; i<VISITOR_COUNT; ++i) {
        if (v->availability[i]) ++count;
    }
    return count;
}

static int open_cue(BfmCastleAudienceVisitors *v, size_t script_offset, BfmForestMessage **opened)
{
    const uint8_t *cue = span(v->scene, v->bytes, script_offset, 8);
    if (!cue || u32le(cue) > 1) return 0;
    uint32_t address = u32le(cue+4);
    if (address < scene_base || address-scene_base >= v->bytes) return 0;
    size_t at = address-scene_base;
    if (v->scene[at] != 5) return 0;
    v->hero = u32le(cue) == 1;
    *opened = bfm_field_message_borrow(v->scene+at, v->bytes-at);
    return *opened != NULL;
}

static BfmForestMessage *count_reply(const BfmCastleAudienceVisitors *v)
{
    unsigned count = saved_people(v);
    size_t off = count == 0 ? NO_RESCUES_PAGE : count == VISITOR_COUNT-RESCUED_FIRST ?
        ALL_RESCUED_PAGE : NUMERIC_PAGE;
    const uint8_t *page = span(v->scene, v->bytes, off, 4);
    if (!page || page[0] != 5) return NULL;
    if (off != NUMERIC_PAGE) return bfm_field_message_borrow(page, v->bytes-off);
    /* This resident field message inserts the decimal rescue count. Keep the
     * two format and colour controls on either side of that slot intact. */
    size_t extent = NO_RESCUES_PAGE - NUMERIC_PAGE;
    if (!span(v->scene, v->bytes, off, extent)) return NULL;
    const uint8_t *slot = memchr(page, 14, extent);
    const uint8_t *end = memchr(page, 0, extent);
    if (!slot || !end || end <= slot) return NULL;
    uint8_t expanded[NO_RESCUES_PAGE-NUMERIC_PAGE+1];
    size_t prefix = (size_t)(slot-page);
    memcpy(expanded, page, prefix);
    size_t pos = prefix;
    if (count >= 10) expanded[pos++] = (uint8_t)('0'+count/10);
    expanded[pos++] = (uint8_t)('0'+count%10);
    size_t suffix = (size_t)(end-slot);
    memcpy(expanded+pos,slot+1,suffix);
    return bfm_field_message_open(expanded,pos+suffix);
}

int bfm_castle_audience_visitors_begin(BfmCastleAudienceVisitors *v, const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, const uint8_t *main, size_t main_bytes)
{
    if (!v || !disc || !span(scene,bytes,CASTLE_VARIABLES,VISITOR_COUNT) ||
        !span(scene,bytes,0xc0e40,32) || !span(main,main_bytes,GUEST_NAMES,4*VISITOR_COUNT)) return 0;
    BfmCastleMenuArt *art = bfm_castle_menu_art_open(disc,scene,bytes);
    if (!art) return 0;
    *v = (BfmCastleAudienceVisitors){.scene=scene,.bytes=bytes,.main=main,
        .main_bytes=main_bytes,.menu_art=art,.phase=QUESTION};
    return 1;
}

void bfm_castle_audience_visitors_free(BfmCastleAudienceVisitors *v)
{
    if (v) {
        bfm_castle_menu_art_free(v->menu_art);
        *v = (BfmCastleAudienceVisitors){0};
    }
}

void bfm_castle_audience_visitors_return(BfmCastleAudienceVisitors *v)
{
    if (v && v->phase == GUEST_CALLED)
        v->phase = QUESTION;
}

void bfm_castle_audience_visitors_closed(BfmCastleAudienceVisitors *v, unsigned answer)
{
    if (v && v->waiting) {
        v->waiting = 0;
        v->answer = answer;
    }
}

enum { HOVER, ANSWER };

/* The game keeps all names visible. The saved byte for each guest controls
 * the label intensity and whether Ribson can call them into the room. */
static void read_roster(BfmCastleAudienceVisitors *v, const BfmSaveRecord *save)
{
    const uint8_t *indices = v->scene+CASTLE_VARIABLES;
    for (unsigned row = 0; row < VISITOR_COUNT; ++row)
        v->availability[row] = save->variables[indices[row]];
    v->story = u32le(save->progress+8);
    for (unsigned i=0; i<8; ++i) {
        const uint8_t *rule = v->scene+0xc0e20+4*i;
        unsigned flag = u16le(rule+2);
        v->known_region[i] = v->story >= u16le(rule) ||
            (flag && flag < 8*sizeof save->flags && (save->flags[flag/8] & (1u << (flag%8))));
    }
}

static void move_cursor(BfmCastleAudienceVisitors *v, int direction)
{
    if (!direction) return;
    unsigned before = v->phase == RESCUED_MENU ? v->menu_row : v->guest_row;
    if (v->phase == RESCUED_MENU) v->menu_row ^= 1u;
    else {
        if (direction < 0 && v->guest_row > 0) --v->guest_row;
        if (direction > 0 && v->guest_row+1 < VISITOR_COUNT) ++v->guest_row;
        if (v->guest_row < v->scroll) v->scroll = v->guest_row;
        if (v->guest_row >= v->scroll + VISIBLE_ROWS) v->scroll = v->guest_row+1-VISIBLE_ROWS;
    }
    if ((v->phase == RESCUED_MENU ? v->menu_row : v->guest_row) != before) {
        v->sound = 0x45a;
        v->ink = 0;
    }
    v->prompt = HOVER;
}

static int guest_receives_visit(const BfmCastleAudienceVisitors *v)
{
    return v->availability[v->guest_row] && v->guest_row != 0 && v->guest_row != 3 &&
        v->guest_row != 5 && !(v->story >= 160 && v->story < 200 &&
                                v->guest_row >= 10 && v->guest_row <= 13);
}

static BfmCastleVisitResult menu_input(BfmCastleAudienceVisitors *v,
    int direction, int held, int accept, int back)
{
    ++v->clock;
    if (direction || back || accept) v->prompt = HOVER;
    if (v->ink < 6) ++v->ink;
    if (back) {
        int in_list = v->phase == GUEST_LIST;
        v->phase = in_list ? RESCUED_MENU : QUESTION;
        v->sound = in_list ? 0x473 : 0x474;
        v->ink = 0;
        return BFM_CASTLE_VISIT_ACTIVE;
    }
    if (!accept) {
        if (held && v->held == held && !direction && v->repeat && !--v->repeat) {
            direction = held;
            v->repeat = 3;
        } else if (direction || held != v->held) v->repeat = 8;
        v->held = held;
        move_cursor(v, direction);
        return BFM_CASTLE_VISIT_ACTIVE;
    }
    if (v->phase == RESCUED_MENU) {
        v->ink = 0;
        if (v->menu_row) {
            v->sound = 0x474;
            v->phase = QUESTION;
        } else {
            v->phase = GUEST_LIST;
            v->sound = 0x45d;
            v->guest_row = v->scroll = 0;
            v->prompt = HOVER;
        }
    } else if (guest_receives_visit(v)) {
        v->phase = GUEST_CALLED;
        v->sound = 0x474;
        v->guest = v->guest_row;
        return BFM_CASTLE_VISIT_GUEST;
    } else if (v->availability[v->guest_row]) {
        v->prompt = ANSWER;
        v->ink = 0;
    } else {
        v->sound = 0x45f;
    }
    return BFM_CASTLE_VISIT_ACTIVE;
}

BfmCastleVisitResult bfm_castle_audience_visitors_step(BfmCastleAudienceVisitors *v,
    const BfmSaveRecord *save, int direction, int held, int accept, int back, BfmForestMessage **opened)
{
    if (!v || !v->menu_art || !save || !opened) return BFM_CASTLE_VISIT_ERROR;
    *opened = NULL;
    v->sound = 0;
    if (v->waiting) return BFM_CASTLE_VISIT_ACTIVE;
    switch (v->phase) {
    case QUESTION:
        if (!open_cue(v, QUESTION_SCRIPT+8, opened)) return BFM_CASTLE_VISIT_ERROR;
        v->phase = CHOICES;
        break;
    case CHOICES:
        if (!open_cue(v, QUESTION_SCRIPT+24, opened)) return BFM_CASTLE_VISIT_ERROR;
        v->phase = DECIDE;
        break;
    case DECIDE:
        read_roster(v, save);
        switch (v->answer) {
        case 1:
            v->phase = RESCUED_MENU;
            v->menu_row = v->ink = v->clock = v->repeat = 0;
            v->held = 0;
            return BFM_CASTLE_VISIT_ACTIVE;
        case 2:
            v->hero = 0;
            *opened = count_reply(v);
            if (!*opened) return BFM_CASTLE_VISIT_ERROR;
            v->phase = QUESTION;
            break;
        case 3:
            if (!open_cue(v, GOODBYE_SCRIPT+8, opened)) return BFM_CASTLE_VISIT_ERROR;
            v->phase = FINISHED;
            break;
        default:
            return BFM_CASTLE_VISIT_ERROR;
        }
        break;
    case RESCUED_MENU: case GUEST_LIST:
        return menu_input(v, direction, held, accept, back);
    case GUEST_CALLED:
        return BFM_CASTLE_VISIT_GUEST;
    case FINISHED:
        return BFM_CASTLE_VISIT_FAREWELL;
    default:
        return BFM_CASTLE_VISIT_ERROR;
    }
    v->waiting = 1;
    return BFM_CASTLE_VISIT_ACTIVE;
}

static int guest_label(const BfmCastleAudienceVisitors *v, unsigned row, BfmCastleMenuLabel *label)
{
    const uint8_t *entry = v->main+GUEST_NAMES+4*row;
    uint32_t address = u32le(entry);
    if (address < main_base || address-main_base >= v->main_bytes) return 0;
    size_t offset = address-main_base;
    *label = (BfmCastleMenuLabel){v->main+offset, v->main_bytes-offset, !v->availability[row]};
    return 1;
}

/* The Seer's location hint inserts one field-format place name. The first
 * three bytes of the question are its two-byte format operands, not text. */
static int seer_hint(const BfmCastleAudienceVisitors *v, uint8_t *out, size_t capacity)
{
    unsigned region = v->scene[0xc0df8+v->guest_row];
    if (region >= 8 || !v->known_region[region]) return 0;
    uint32_t address = u32le(v->scene+0xc0e40+4*region);
    if (address < scene_base || address-scene_base >= v->bytes) return -1;
    size_t at = address - scene_base;
    const uint8_t *location = v->scene+at;
    const uint8_t *location_end = memchr(location,0,v->bytes-at);
    const size_t limit = 0xc6fe8-0xc6fb0;
    const uint8_t *question = span(v->scene,v->bytes,0xc6fb0,limit);
    if (!location_end || !question) return -1;
    const uint8_t *slot = memchr(question+3,15,limit-3);
    const uint8_t *end = memchr(question+3,0,limit-3);
    if (!slot || !end || end <= slot) return -1;
    size_t head = (size_t)(slot-question), tail = (size_t)(end-slot);
    size_t name = (size_t)(location_end-location);
    if (head+tail > capacity || name > capacity-head-tail) return -1;
    memcpy(out,question,head);
    memcpy(out+head,location,name);
    memcpy(out+head+name,slot+1,tail);
    return (int)(head+name+tail);
}

int bfm_castle_audience_visitors_draw(const BfmCastleAudienceVisitors *v,
    const BfmForestMessageArt *message_art, const uint16_t tiles[512][1024],
    uint32_t pixels[240][320])
{
    if (!v || (v->phase != RESCUED_MENU && v->phase != GUEST_LIST)) return 1;
    int list = v->phase == GUEST_LIST;
    if (!bfm_castle_menu_art_draw_panel(v->menu_art, scene_base+RESCUED_PANEL,
            NULL, 2, v->menu_row, !list, 0, 0, tiles, pixels)) return 0;
    if (list) {
        BfmCastleMenuLabel names[VISIBLE_ROWS];
        for (unsigned i=0; i<VISIBLE_ROWS; ++i) {
            if (!guest_label(v,v->scroll+i,&names[i])) return 0;
        }
        int show_arrows = (v->clock & 8u) != 0;
        if (!bfm_castle_menu_art_draw_panel(v->menu_art, scene_base+LIST_PANEL, names, VISIBLE_ROWS,
                v->guest_row-v->scroll, 1, show_arrows && v->scroll != 0,
                show_arrows && v->scroll+VISIBLE_ROWS < VISITOR_COUNT, tiles, pixels)) return 0;
    }
    const uint8_t *source = v->scene;
    size_t size = v->bytes, off = 0xc6eb8;
    int hero = 0;
    uint8_t hint[192];
    if (list && v->prompt == HOVER) {
        hero = 1;
        if (v->guest_row == 0) off = 0xc7010;
        else if (!v->availability[v->guest_row] && v->availability[6]) {
            int length = seer_hint(v,hint,sizeof hint);
            if (length < 0) return 0;
            hero = 0;
            off = 0xc6fe8;
            if (length) {
                source = hint;
                size = (size_t)length;
                off = 0;
            }
        } else {
            source = v->main;
            size = v->main_bytes;
            unsigned state = v->availability[v->guest_row];
            off = !state ? 0x43d60 : state == 1 ? 0x43d98 : 0x43db0;
        }
    } else if (list && v->prompt == ANSWER) {
        switch (v->guest_row) {
        case 0: off = 0xc6ed8; break;
        case 3: off = 0xc6efc; break;
        case 5: off = 0xc6f3c; break;
        default: off = 0xc7034; hero = 1; break;
        }
    }
    const uint8_t *box = span(v->scene, v->bytes, 0xc0f0c, 12);
    const uint8_t *portrait = span(v->scene, v->bytes, hero ? 0xc07c8 : 0xc07b4, 20);
    const uint8_t *text = span(source, size, off, 3);
    return text && box && portrait && bfm_library_question_draw(message_art, box, portrait,
        tiles, text, size-off, v->ink, pixels);
}
