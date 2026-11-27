/* SPDX-License-Identifier: MIT */
#include "save_menu.h"
#include "bakery_menu_art.h"
#include "inn_menu.h"
#include "chapter_completion_view.h"
#include "inn_greeting.h"
#include "scene_tiles.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { GLYPHS, LABELS, PAGE_WORDS = 64 };
struct BfmSaveArt {
    uint16_t pages[2][256][PAGE_WORDS];
    uint16_t palette[16];
};

BfmSaveArt *bfm_save_art_load(const MusashiDiscMedia *disc)
{
    uint16_t (*words)[1024] = calloc(512, sizeof *words);
    BfmSaveArt *art = NULL;
    if (words && bfm_scene_read_tiles(disc, "MAIN.CD", 12, words)) {
        art = malloc(sizeof *art);
        if (art) {
            memcpy(art->palette, &words[257][352], sizeof art->palette);
            for (unsigned y = 0; y < 256; ++y) {
                memcpy(art->pages[GLYPHS][y], &words[256 + y][320], sizeof art->pages[0][0]);
                memcpy(art->pages[LABELS][y], &words[y][960], sizeof art->pages[0][0]);
            }
        }
    }
    free(words);
    return art;
}

typedef struct {
    const BfmSaveArt *art;
    uint32_t (*pixels)[320];
    unsigned over_field;
    const uint16_t *palette;
} Canvas;

static uint32_t rgb(unsigned r, unsigned g, unsigned b)
{
    return 0xff000000u | (r * 255u / 31u << 16) | (g * 255u / 31u << 8) | (b * 255u / 31u);
}

/* The two mounted four-bit pages use the same 16-color CLUT. */
static void blit(Canvas *c, int page, int x, int y, int w, int h,
                 int u, int v, int uw, int vh, unsigned light)
{
    for (int j = 0; j < h; ++j) {
        const uint16_t *src = c->art->pages[page][v + j * vh / h];
        for (int i = 0; i < w; ++i) {
            unsigned tx = (unsigned)(u + i * uw / w);
            unsigned index = (src[tx / 4] >> ((tx % 4) * 4)) & 15u;
            unsigned texel = c->palette[index];
            if (texel && (unsigned)(x + i) < 320 && (unsigned)(y + j) < 240)
                c->pixels[y + j][x + i] = rgb((texel & 31u) * light / 128,
                                               ((texel >> 5) & 31u) * light / 128,
                                               ((texel >> 10) & 31u) * light / 128);
        }
    }
}

static void label(Canvas *c, int x, int y, int u, int v, int w, int h, unsigned light)
{
    blit(c, LABELS, x, y, w, h, u, v, w, h, light);
}

static void text(Canvas *c, int x, int y, const char *s, unsigned light)
{
    for (; *s; ++s, x += 7) {
        unsigned ch = (unsigned char)*s, tile;
        if (ch >= 'a' && ch <= 'z') tile = ch - 'a' + 1u + (ch >= 'o');
        else if (ch >= 'A' && ch <= 'Z') tile = ch - 'A' + 161u + (ch >= 'O');
        else if (ch >= '0' && ch <= '9') tile = ch - '0' + 129;
        else {
            switch (ch) {
            case ':': tile = 122; break;
            case '/': tile = 123; break;
            case '-': tile = 143; break;
            case '.': tile = 188; break;
            case '$': tile = 128; break;
            case ';': tile = 140; break; /* compact colon on the HP/BP lines */
            default: continue;
            }
        }
        blit(c, GLYPHS, x, y, 8, 8, (int)(tile % 16) * 8, (int)(tile / 16) * 8, 8, 8, light);
    }
}

static const int dither[4][4] = {
    {-4, 0,-3, 1}, {2,-2, 3,-1}, {-3, 1,-4, 0}, {3,-1, 2,-2}
};

static unsigned quantize(int value, int adjustment, int dim)
{
    int level = ((value >> dim) + adjustment) / 8;
    if (level < 0) level = 0;
    if (level > 31) level = 31;
    return (unsigned)level;
}

enum PanelBand { BODY, HEADING, INN_HEADING };

static void shade(Canvas *c, int x, int y, int w, int h, enum PanelBand band, int dim)
{
    for (int j = 0; j < h; ++j) {
        for (int i = 0; i < w; ++i) {
            int d = dither[(y+j) % 4][(x+i) % 4];
            int numerator = (band == BODY ? 120 : 40) * j * w - 40 * i * h;
            int base = band == BODY ? 112 : (band == HEADING ? 208 : 192);
            int blue = (base * w * h + numerator + w * h / 2) / (w * h);
            unsigned red = quantize(band == HEADING ? 8 : 0, d, dim);
            unsigned green = quantize(band == HEADING ? 16 : 8, d, dim);
            unsigned b = quantize(blue, d, dim);
            if (dim && c->over_field) {
                uint32_t field = c->pixels[y+j][x+i];
                red += (field >> 20) & 15u;
                green += (field >> 12) & 15u;
                b += (field >> 4) & 15u;
            }
            c->pixels[y+j][x+i] = rgb(red, green, b);
        }
    }
}

static void panel_frame(Canvas *c, int x, int y, int w, int h, unsigned light)
{
    for (int side = 0; side <= 1; ++side) {
        int dx = side * (w-8), dy = side * (h-8);
        blit(c, GLYPHS, x+dx, y+8, 8, h-16, 32, 72, 8, 7, light);
        blit(c, GLYPHS, x+8, y+dy, w-16, 8, 40, 72, 7, 8, light);
        blit(c, GLYPHS, x+dx, y, 8, 8, side * 8, 72, 8, 8, light);
        blit(c, GLYPHS, x+dx, y+h-8, 8, 8, 16 + side * 8, 72, 8, 8, light);
    }
}

static void panel(Canvas *c, int x, int y, int w, int h, int heading_height)
{
    shade(c, x+4, y+6, w-8, h-9, BODY, !heading_height);
    if (heading_height) shade(c, x+4, y+4, w-8, heading_height, HEADING, 0);
    panel_frame(c, x, y, w, h, heading_height ? 128 : 88);
}

static void draw_row(Canvas *c, const BfmSaveMenu *menu, unsigned i)
{
    int x = 8 + 151 * (int)(i % 2), y = 48 + 90 * (int)(i / 2);
    unsigned light = i == menu->selected ? 128 : 88;
    const BfmSaveRow *row = &menu->rows[i];
    char line[64];
    panel(c, x, y, 153, 92, i == menu->selected ? 27 : 0);
    label(c, x+19, y+8, 36*(int)(i%2), 12*(int)(i/2), 35, 12, light);
    unsigned badge = row->state == BFM_SAVE_EMPTY ? 1 : row->chapter;
    if (badge) label(c, x+96, y+8, 72+18*(int)(badge-1), 0, 17, 12, light);
    if (row->state == BFM_SAVE_DAMAGED) {
        text(c, x+24, y+56, "Unreadable", light);
        return;
    }
    snprintf(line, sizeof line, "Playtime %02u:%02u", row->hours, row->minutes);
    text(c, x+24, y+22, line, light);
    if (row->state == BFM_SAVE_EMPTY) {
        text(c, x+45, y+56, "No Data", light);
        return;
    }
    static const char *const weekdays[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    snprintf(line, sizeof line, "Day:%3u %s %02u:%02u", row->day, weekdays[(row->day-1)%7],
             row->hour, row->minute);
    text(c, x+17, y+36, line, light);
    snprintf(line, sizeof line, "HP;%3u/%u", row->hp, row->max_hp);
    text(c, x+17, y+46, line, light);
    snprintf(line, sizeof line, "BP;%3u/%u", row->bp, row->max_bp);
    text(c, x+17, y+56, line, light);
    snprintf(line, sizeof line, "$%6llu", (unsigned long long)row->money);
    text(c, x+17, y+66, line, light);
    snprintf(line, sizeof line, "B.Field%3u/35", row->rescued);
    text(c, x+17, y+76, line, light);
}

static void file_cursor(Canvas *c, unsigned selected, unsigned fields)
{
    static const int motion[] = {0, 0, 2, 4, 4, 3, 2, 1};
    int x = 8 + 151 * (int)(selected % 2);
    int y = 54 + 90 * (int)(selected / 2);
    blit(c, GLYPHS, x + motion[(fields / 2) % 8], y, 16, 16, 112, 16, 16, 16, 128);
}

void bfm_save_menu_draw(const BfmSaveArt *art, const BfmSaveMenu *menu, unsigned fields,
                        uint32_t pixels[240][320])
{
    Canvas c = {art, pixels, 0, art->palette};
    for (int y = 0; y < 240; ++y)
        for (int x = 0; x < 320; ++x) pixels[y][x] = 0xff000000u;
    panel(&c, 8, 12, 304, 38, 29);
    if (menu->issue) {
        text(&c, 22, 18, menu->issue, 128);
        text(&c, 22, 31, "Press T or Triangle to return.", 128);
        return;
    }
    if (menu->page == BFM_SAVE_CARD_READING) {
        label(&c, 118, 18, 0, 24, 75, 12, 128);
        label(&c, 40, 31, 0, 36, 237, 12, 128);
        return;
    }
    if (menu->page == BFM_SAVE_NO_DATA) {
        label(&c, 94, 18, 0, 120, 124, 12, 128);
        label(&c, 76, 31, 0, 132, 162, 12, 128);
        return;
    }
    for (unsigned i = 0; i < 4; ++i) draw_row(&c, menu, i);
    if (menu->page == BFM_SAVE_BOUNDARY) {
        text(&c, 22, 18, "This preview cannot resume this save.", 128);
        text(&c, 22, 31, "Press T or Triangle to return to files.", 128);
    } else if (menu->page == BFM_SAVE_CONFIRM || menu->page == BFM_SAVE_LOADED) {
        label(&c, 106, 18, 0, 180, 101, 12, 128);
        label(&c, 118, 31, 114, 72, 83, 12, 128);
    } else {
        label(&c, 58, 18, 0, 156, 203, 12, 128);
        label(&c, 46, 31, 0, 168, 221, 12, 128);
    }
    if (menu->page == BFM_SAVE_LOADED) {
        panel(&c, 32, 118, 256, 38, 29);
        label(&c, 122, 131, 76, 216, 74, 12, 128);
        return;
    }
    file_cursor(&c, menu->selected, fields);
}

int bfm_inn_menu_dialogue(const BfmInnMenu *menu)
{
    switch (menu->page) {
    case BFM_INN_SERVICE:
    case BFM_INN_SERVICE_ERROR:
        return BFM_INN_NORMAL_DIALOGUE;
    case BFM_INN_ROOM_ERROR:
        if (!menu->notice) return BFM_INN_LOW_FUNDS;
        /* Keep the selected room visible behind the preview notice. */
        break;
    case BFM_INN_ROOMS:
        break;
    case BFM_INN_REST:
        return BFM_INN_PAID_DIRECTIONS;
    default:
        return BFM_INN_HIDE_DIALOGUE;
    }
    return menu->room < 6 ? BFM_INN_FIRST_ROOM_DESCRIPTION + (int)menu->room :
                            BFM_INN_NORMAL_DIALOGUE;
}

static void inn_service_list(Canvas *c, unsigned selected, int active)
{
    static const char *const services[] = {"Sleep", "Save", "Chat", "Quit"};
    unsigned light = active ? 128 : 88;
    panel(c, 4, 12, 88, 75, active ? 13 : 0);
    text(c, 34, 20, "INN", light);
    for (unsigned i = 0; i < 4; ++i)
        text(c, 25, 33 + 13*(int)i, services[i], light);
    blit(c, GLYPHS, 13, 33 + 13*(int)selected, 8, 8, 48, 72, 8, 8, light);
}

/* The early village visit shows four offered rooms followed by two guests. */
static void inn_room_list(Canvas *c, const BfmInnMenu *menu, int active)
{
    unsigned light = active ? 128 : 88;
    shade(c, 160, 91, 152, 13, INN_HEADING, !active);
    shade(c, 160, 104, 152, 82, BODY, !active);
    panel_frame(c, 156, 87, 160, 102, light);
    text(c, 198, 95, "CUSTOMERS", light);
    for (unsigned i = 0; i < 6; ++i) {
        int y = 108 + 13 * (int)i;
        char line[12];
        snprintf(line, sizeof line, "%u", 201u+i);
        text(c, 171, y, line, light);
        unsigned cost = menu->prices[i];
        if (cost) {
            text(c, 199, y, "Vacancy", light);
            snprintf(line, sizeof line, "$%u", cost);
            text(c, 290 - 7*(int)strlen(line), y, line, light);
        } else {
            text(c, 199, y, i == 4 ? "Wanda" : "Macho", light);
            text(c, 276, y, "-", light);
        }
    }
    blit(c, GLYPHS, 163, 108 + 13*(int)menu->room, 8, 8, 48, 72, 8, 8, light);
}

static void shop_label(Canvas *c, int x, int y, const BfmShopLabel *label, unsigned light)
{
    for (unsigned i = label->count; i > 0; --i) {
        unsigned uv = label->uv[i-1];
        blit(c, GLYPHS, x + 7*(int)(i-1), y, 8, 8, uv & 255u, uv >> 8, 8, 8, light);
    }
}

static void shop_fill(Canvas *c, const BfmShopFill *fill, int active)
{
    BfmShopRect r = fill->rect;
    int area = r.w*r.h;
    for (int y = 0; y < r.h; ++y) {
        for (int x = 0; x < r.w; ++x) {
            uint32_t *pixel = &c->pixels[r.y+y][r.x+x];
            int d = dither[(r.y+y)%4][(r.x+x)%4];
            unsigned color[3];
            for (unsigned channel = 0; channel < 3; ++channel) {
                int across = channel == 2 ? 40 : 0;
                int delta = (int)fill->bottom[channel] - fill->top[channel];
                int level = (fill->top[channel]*area + (delta+across)*y*r.w -
                             across*x*r.h + area/2)/area;
                if (!active) {
                    unsigned behind = (*pixel >> (19-8*channel)) & 31u;
                    level = (int)behind * 4 + level/2;
                }
                color[channel] = quantize(level, d, 0);
            }
            *pixel = rgb(color[0], color[1], color[2]);
        }
    }
}

void bfm_save_art_shop_list(const BfmSaveArt *art, uint32_t pixels[240][320],
                            const BfmShopListLayout *box, BfmBakeryPane pane, const BfmBakeryMenuView *view,
                            unsigned cursor, const BfmShopLabel *names,
                            const unsigned *prices, unsigned count)
{
    Canvas c = {art, pixels, 1, art->palette};
    int active = pane == view->active;
    unsigned light = active ? 128 : 88;
    shop_fill(&c, &box->fills[1], active);
    if (active) shop_fill(&c, &box->fills[0], 1);
    for (unsigned i = 0; i < 8; ++i) {
        const BfmShopBorder *edge = &box->borders[i];
        BfmShopRect r = edge->rect;
        blit(&c, GLYPHS, r.x, r.y, r.w, r.h, (int)edge->u, (int)edge->v,
             r.w > 8 ? 7 : 8, r.h > 8 ? 7 : 8, light);
    }
    shop_label(&c, box->title_x, box->title_y, &box->title, light);
    for (unsigned i = 0; i < count; ++i) {
        int x = box->row_x[i], y = box->row_y[i];
        shop_label(&c, x + (pane == BFM_BAKERY_SERVICES ? 4 : 0), y, &names[i], light);
        if (prices) {
            char amount[16];
            snprintf(amount, sizeof amount, "$%u", prices[i]);
            text(&c, x + 102 - 7*(int)strlen(amount), y, amount, light);
        }
    }
    blit(&c, GLYPHS, box->row_x[cursor]-8, box->row_y[cursor], 8, 8, 48, 72, 8, 8, light);
    if (active && pane == BFM_BAKERY_GOODS && (view->age & 8u)) {
        int x = box->fills[0].rect.x + box->fills[0].rect.w - 12;
        if (view->first_good)
            blit(&c, GLYPHS, x, box->row_y[0]-2, 8, 8, 120, 48, 8, 8, light);
        if (view->goods_count-view->first_good > BFM_BAKERY_VISIBLE_ROWS)
            blit(&c, GLYPHS, x, box->row_y[BFM_BAKERY_VISIBLE_ROWS-1]+1,
                 8, 8, 120, 56, 8, 8, light);
    }
}

static void inn_room_fade(uint32_t pixels[240][320], unsigned age)
{
    /* Ages are 30 Hz room updates. The picture loses levels from each
     * channel, stays black, then brings the counter back into view. */
    if (age <= 10 || age >= 105) return;
    unsigned levels = age <= 74 ? age - 10 : 105 - age;
    if (levels > 31) levels = 31;
    for (int y = 0; y < 240; ++y) {
        for (int x = 0; x < 320; ++x) {
            uint32_t src = pixels[y][x];
            unsigned r = (src >> 19) & 31, g = (src >> 11) & 31, b = (src >> 3) & 31;
            pixels[y][x] = rgb(r > levels ? r-levels : 0, g > levels ? g-levels : 0,
                               b > levels ? b-levels : 0);
        }
    }
}

static void save_picker_heading(Canvas *c, int selecting)
{
    if (selecting) {
        label(c, 46, 18, 0, 156, 172, 12, 128);
        label(c, 219, 18, 179, 132, 50, 12, 128);
        label(c, 94, 31, 126, 24, 126, 12, 128);
    } else {
        label(c, 106, 18, 0, 180, 102, 12, 128);
        label(c, 118, 31, 114, 72, 85, 12, 128);
    }
}

void bfm_inn_menu_draw(const BfmInnMenu *menu, const BfmSaveArt *art,
                       unsigned fields, uint32_t pixels[240][320])
{
    Canvas c = {art, pixels, 1, menu->field_palette ? menu->field_palette : art->palette};
    if (bfm_inn_menu_dialogue(menu) != BFM_INN_HIDE_DIALOGUE) {
        int services = menu->page == BFM_INN_SERVICE || menu->page == BFM_INN_SERVICE_ERROR;
        inn_service_list(&c, menu->service, services);
        inn_room_list(&c, menu, !services);
        if (menu->page == BFM_INN_REST) inn_room_fade(pixels, menu->rest_age);
        if ((menu->page == BFM_INN_SERVICE_ERROR || menu->page == BFM_INN_ROOM_ERROR) &&
            menu->notice) {
            panel(&c, 24, 116, 272, 38, 29);
            text(&c, 32, 130, menu->notice, 128);
        }
        return;
    }
    panel(&c, 8, 12, 304, 38, 29);
    for (unsigned i = 0; i < 4; ++i) draw_row(&c, &menu->card, i);
    if (menu->overwriting && (menu->page == BFM_INN_CONFIRM || menu->page == BFM_INN_WRITING ||
                               menu->page == BFM_INN_SAVED)) {
        label(&c, 100, 18, 108, 180, 113, 12, 128);
        label(&c, 94, 31, 126, 24, 125, 12, 128);
    } else {
        save_picker_heading(&c, menu->page == BFM_INN_FILES || menu->page == BFM_INN_FILE_ERROR);
    }
    switch (menu->page) {
    case BFM_INN_WRITING:
    case BFM_INN_SAVED:
        panel(&c, 32, 118, 256, 38, 29);
        if (menu->page == BFM_INN_SAVED) {
            label(&c, 124, 131, 72, 204, 65, 12, 128);
        } else {
            label(&c, 124, 124, 0, 204, 71, 12, 128);
            label(&c, 40, 137, 0, 36, 239, 12, 128);
        }
        break;
    case BFM_INN_FILE_ERROR:
        panel(&c, 24, 116, 272, 38, 29);
        text(&c, 32, 130, menu->notice, 128);
        break;
    default:
        file_cursor(&c, menu->card.selected, fields);
        break;
    }
}

int bfm_chapter_completion_draw(const BfmChapterCompletion *state, const BfmSaveArt *art,
                                const BfmChapterCard *title, unsigned fields,
                                uint32_t pixels[240][320])
{
    if (!state || !pixels || state->page == BFM_CHAPTER_INACTIVE ||
        state->page == BFM_CHAPTER_BEDROOM || state->page == BFM_CHAPTER_COMPLETION_FINISHED)
        return 0;
    if (state->page == BFM_CHAPTER_TWO_TITLE) {
        /* The first field follows the accepted resume prompt. The fade
         * starts earlier on the shared crest clock than on a forest entry. */
        unsigned elapsed = fields < BFM_CHAPTER_FOREST_FIELD - 56u ?
                           fields + 56u : BFM_CHAPTER_FOREST_FIELD;
        return bfm_chapter_card_draw(title, elapsed, 1, &pixels[0][0]) >= 0;
    }
    if (!art) return 0;
    Canvas c = {art, pixels, 0, art->palette};
    for (unsigned y = 0; y < 240; ++y)
        for (unsigned x = 0; x < 320; ++x) pixels[y][x] = 0xff000000u;
    panel(&c, 8, 12, 304, 38, 29);
    switch (state->page) {
    case BFM_CHAPTER_SAVE_PROMPT:
        label(&c, 88, 18, 0, 144, 139, 12, 128);
        label(&c, 118, 31, 114, 72, 85, 12, 128);
        break;
    case BFM_CHAPTER_RESUME:
        label(&c, 70, 18, 0, 144, 107, 12, 128);
        label(&c, 178, 18, 180, 144, 71, 12, 128);
        label(&c, 118, 31, 114, 72, 83, 12, 128);
        break;
    case BFM_CHAPTER_SAVE_READING:
        label(&c, 118, 18, 0, 24, 76, 12, 128);
        label(&c, 40, 31, 0, 36, 238, 12, 128);
        break;
    case BFM_CHAPTER_SAVE_ERROR:
        if (state->issue) text(&c, 22, 25, state->issue, 128);
        break;
    case BFM_CHAPTER_SAVE_FILES:
    case BFM_CHAPTER_SAVE_CONFIRM:
    case BFM_CHAPTER_SAVE_WRITING:
    case BFM_CHAPTER_SAVED:
        save_picker_heading(&c, state->page == BFM_CHAPTER_SAVE_FILES);
        for (unsigned i = 0; i < 4; ++i) draw_row(&c, &state->files, i);
        if (state->page == BFM_CHAPTER_SAVE_WRITING || state->page == BFM_CHAPTER_SAVED) {
            panel(&c, 32, 118, 256, 38, 29);
            if (state->page == BFM_CHAPTER_SAVE_WRITING) {
                label(&c, 124, 124, 0, 204, 71, 12, 128);
                label(&c, 40, 137, 0, 36, 238, 12, 128);
            } else {
                label(&c, 124, 131, 72, 204, 65, 12, 128);
            }
        } else {
            file_cursor(&c, state->files.selected, fields);
        }
        break;
    default:
        return 0;
    }
    return 1;
}
