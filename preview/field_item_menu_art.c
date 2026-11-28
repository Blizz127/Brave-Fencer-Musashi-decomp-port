/* SPDX-License-Identifier: MIT */
#include "field_item_menu_art.h"
#include "bakery_purchase.h"
#include "library_books.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>
#include <string.h>

/* Relative to the mounted field's menu inset. The full-bag award uses two
 * columns of six item slots and a three-row action popup. */
enum {
    FRAMES = 0x330, FILLS = 0x630, TITLE_XY = 0x730, ROW_XY = 0x894,
    ITEM_TITLE = 0x8d8, ACTION_LABELS = 0x97c, ITEM_HINTS = 0xaa4,
    BOX = 0xca4, ROW_HAND = 0xcc0, ITEM_PANEL = 2, ACTION_PANEL = 5
};

struct BfmFieldItemMenuArt {
    const uint8_t *scene, *layout, *main, *item_xy, *action_xy;
    size_t scene_bytes, main_bytes;
    BfmCastleMenuArt *font;
    BfmForestMessageArt *help;
};

static const uint8_t *room_data(const BfmFieldItemMenuArt *a, size_t pointer_offset, size_t length)
{
    uint32_t address = u32le(a->layout + pointer_offset), base = UINT32_C(0x80128158);
    if (address < base || length > a->scene_bytes || address - base > a->scene_bytes - length)
        return NULL;
    return a->scene + (address - base);
}

BfmFieldItemMenuArt *bfm_field_item_menu_art_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, size_t inset, const uint8_t *main, size_t main_bytes,
    const uint16_t tiles[512][1024])
{
    if (!disc || !scene || inset > bytes || bytes - inset < ROW_HAND + 16 ||
        !main || main_bytes < 4 || !tiles) return NULL;
    BfmFieldItemMenuArt *a = calloc(1, sizeof *a);
    if (!a) return NULL;
    a->scene = scene;
    a->layout = scene + inset;
    a->scene_bytes = bytes;
    a->main = main;
    a->main_bytes = main_bytes;
    const uint8_t *right_xy = room_data(a, ROW_XY + 4*ITEM_PANEL, 17*4);
    a->action_xy = room_data(a, ROW_XY + 4*ACTION_PANEL, 3*4);
    if (!right_xy || !a->action_xy || right_xy < a->layout || a->action_xy < a->layout) goto fail;
    a->item_xy = right_xy + 5*4;
    a->font = bfm_castle_menu_art_open(disc, scene, bytes);
    a->help = bfm_castle_message_art_load(disc, scene, bytes, tiles, inset);
    if (!a->font || !a->help) goto fail;
    return a;
fail:
    bfm_field_item_menu_art_free(a);
    return NULL;
}

void bfm_field_item_menu_art_free(BfmFieldItemMenuArt *a)
{
    if (!a) return;
    bfm_castle_menu_art_free(a->font);
    bfm_forest_message_art_free(a->help);
    free(a);
}

/* The full and narrow spaces advance seven and four pixels without ink. */
static int menu_label(const BfmCastleMenuArt *font, BfmCastleMenuLabel name,
                            int x, int y, int active, const uint16_t tiles[512][1024],
                            uint32_t pixels[240][320])
{
    if (!name.bytes) return 0;
    uint8_t word[2*48+1];
    size_t used = 0;
    for (size_t pos = 0; pos < name.size; pos += 2) {
        const uint8_t *p = name.bytes+pos;
        if (*p && name.size-pos < 2) return 0;
        int space = *p == 0x81 && (p[1] == 0x40 || p[1] == 0x41);
        if (!*p || space) {
            if (used) {
                word[used] = 0;
                BfmCastleMenuLabel run = {.bytes=word, .size=used+1, .ink=name.ink};
                if (!bfm_castle_menu_label(font, run, tiles, pixels, x, y, active)) return 0;
                x += (int)(used/2)*7;
                used = 0;
            }
            if (!*p) return 1;
            x += p[1] == 0x40 ? 7 : 4;
        } else {
            if (used+2 >= sizeof word) return 0;
            memcpy(word+used, p, 2);
            used += 2;
        }
    }
    return 0;
}

static int label_at(const BfmFieldItemMenuArt *a, BfmCastleMenuLabel name, const uint8_t *xy,
                    const uint16_t tiles[512][1024], uint32_t pixels[240][320])
{
    return menu_label(a->font, name, 160 + s16le(xy), 120 + s16le(xy+2), 1, tiles, pixels);
}

static int room_label(const BfmFieldItemMenuArt *a, size_t pointer_offset, const uint8_t *xy,
                      const uint16_t tiles[512][1024], uint32_t pixels[240][320])
{
    const uint8_t *text = room_data(a, pointer_offset, 1);
    if (!text) return 0;
    BfmCastleMenuLabel name = {.bytes=text, .size=a->scene_bytes - (size_t)(text-a->scene)};
    return label_at(a, name, xy, tiles, pixels);
}

static BfmCastleMenuLabel item_label(const BfmFieldItemMenuArt *a, unsigned item)
{
    BfmBakeryGood good;
    if (!bfm_bakery_item(a->main, a->main_bytes, item, &good)) return (BfmCastleMenuLabel){0};
    return (BfmCastleMenuLabel){.bytes=good.name, .size=good.name_bytes+1};
}

static int hint(const BfmFieldItemMenuArt *a, const BfmFieldItemMenuView *v,
                unsigned item, uint32_t pixels[240][320])
{
    uint32_t address = u32le(a->layout + ITEM_HINTS + 4*item), base = UINT32_C(0x800cedf8);
    if (address < base || address - base > a->main_bytes - 4) return 0;
    size_t offset = address - base;
    return bfm_field_menu_hint_draw_sized(a->help, a->layout+BOX, v->hint_columns, v->hint_lines,
                                          a->main+offset, a->main_bytes-offset, v->ink_age, pixels);
}

static void window(const BfmFieldItemMenuArt *a, unsigned panel, const uint16_t tiles[512][1024],
                   uint32_t pixels[240][320])
{
    bfm_castle_menu_window(a->layout + FRAMES + 96*panel, a->layout + FILLS + 32*panel,
                           1, tiles, pixels);
}

static void hand(const uint8_t *xy, int offset, const uint16_t tiles[512][1024],
                 uint32_t pixels[240][320])
{
    bfm_castle_menu_sprite(tiles, pixels, 160+s16le(xy)+offset, 120+s16le(xy+2)-4,
                           16, 16, 112, 16, 16, 16, 1, BFM_MENU_LABEL_NORMAL);
}

int bfm_field_forced_items_draw(const BfmFieldItemMenuArt *a, const BfmFieldItemMenuView *v,
    const BfmSaveRecord *save, int marked, const uint16_t tiles[512][1024],
    uint32_t pixels[240][320])
{
    if (!a || !v || !save || !tiles || !pixels || v->phase > BFM_FIELD_MENU_ACTIONS ||
        v->item_slot >= 12 || v->action_row >= 3 || marked < -1 || marked >= 12) return 0;
    unsigned item = save->variables[47 + v->item_slot];
    if (!hint(a, v, item < 125 ? item : 0, pixels) && !hint(a, v, 0, pixels)) return 0;
    window(a, ITEM_PANEL, tiles, pixels);
    if (!room_label(a, ITEM_TITLE, a->layout + TITLE_XY + 4*ITEM_PANEL, tiles, pixels)) return 0;
    for (unsigned slot = 0; slot < 12; ++slot) {
        BfmCastleMenuLabel name = item_label(a, save->variables[47 + slot]);
        if (name.bytes) label_at(a, name, a->item_xy + 4*slot, tiles, pixels);
    }
    if (marked >= 0) hand(a->item_xy + 4*marked, -5, tiles, pixels);
    const uint8_t *cursor = a->item_xy + 4*v->item_slot;
    if (v->phase == BFM_FIELD_MENU_ACTIONS) {
        window(a, ACTION_PANEL, tiles, pixels);
        BfmCastleMenuLabel name = item_label(a, item);
        if (name.bytes) label_at(a, name, a->layout + TITLE_XY + 4*ACTION_PANEL, tiles, pixels);
        for (unsigned i = 0; i < 3; ++i)
            if (!room_label(a, ACTION_LABELS + 4*i, a->action_xy + 4*i, tiles, pixels)) return 0;
        cursor = a->action_xy + 4*v->action_row;
    }
    int bob = s16le(a->layout + ROW_HAND + 2*(v->hand_age % 8));
    hand(cursor, bob - 13, tiles, pixels);
    return 1;
}
