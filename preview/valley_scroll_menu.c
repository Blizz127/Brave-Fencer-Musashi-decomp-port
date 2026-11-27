/* SPDX-License-Identifier: MIT */
#include "valley_scroll_menu.h"
#include "library_books.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>
#include <string.h>

/* US SC03/31 owns this field's menu layout and labels. MAIN/10 supplies the
 * English help programs addressed by the room's scroll description table. */
enum {
    SCENE_BASE = 0x80128158u, MAIN_BASE = 0x800cedf8u,
    INSET = 0x5a8e0, FRAMES = 0x5ac10, FILLS = 0x5af10, TITLE_XY = 0x5b010,
    ROW_XY_TABLE = 0x5b174, TITLE_TABLE = 0x5b194, ROW_TABLE = 0x5b200,
    DESCRIPTION_TABLE = 0x5b300, BOX = 0x5b584, GUARDIAN_HINT = 0x44384,
    EARTH_FLAG = 22
};
enum { CLOSED, SWORD, SCROLLS };
enum { NORMAL, EARTH, CHOICES };

struct BfmValleyScrollMenu {
    BfmCastleMenuArt *art;
    BfmForestMessageArt *help_art;
    const uint8_t *scene, *main;
    size_t main_bytes;
    BfmCastleMenuLabel title, name[CHOICES];
    const uint8_t *row_xy;
    size_t hint[CHOICES];
    unsigned page, row, rows, equipped, ink_age, hand_age;
    int guardian;
};

static const uint8_t *resource(const uint8_t *payload, size_t size, uint32_t address,
                               uint32_t base, size_t count)
{
    if (address < base || count > size || address-base > size-count) return NULL;
    return payload + (address-base);
}

static int read_label(const uint8_t *scene, size_t size, size_t table, BfmCastleMenuLabel *out)
{
    const uint8_t *name = resource(scene, size, u32le(scene+table), SCENE_BASE, 1);
    if (!name) return 0;
    const uint8_t *end = memchr(name, 0, size - (size_t)(name-scene));
    if (!end) return 0;
    *out = (BfmCastleMenuLabel){.bytes=name, .size=(size_t)(end-name)+1};
    return 1;
}

BfmValleyScrollMenu *bfm_valley_scroll_menu_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, const uint8_t *main, size_t main_bytes,
    const uint16_t tiles[512][1024])
{
    if (!disc || !scene || bytes < BOX+12 || !main || main_bytes < GUARDIAN_HINT+3 || !tiles)
        return NULL;
    BfmValleyScrollMenu *m = calloc(1, sizeof *m);
    if (!m) return NULL;
    m->scene = scene;
    m->main = main;
    m->main_bytes = main_bytes;
    m->row_xy = resource(scene, bytes, u32le(scene+ROW_XY_TABLE), SCENE_BASE, 4*CHOICES);
    if (!m->row_xy || !read_label(scene, bytes, TITLE_TABLE, &m->title)) goto fail;
    for (unsigned i = 0; i < CHOICES; ++i) {
        const uint8_t *hint = resource(main, main_bytes, u32le(scene+DESCRIPTION_TABLE+4*i),
                                       MAIN_BASE, 3);
        if (!hint || !read_label(scene, bytes, ROW_TABLE+4*i, &m->name[i])) goto fail;
        m->hint[i] = (size_t)(hint-main);
    }
    m->art = bfm_castle_menu_art_open(disc, scene, bytes);
    m->help_art = bfm_castle_message_art_load(disc, scene, bytes, tiles, INSET);
    if (!m->art || !m->help_art) goto fail;
    return m;
fail:
    bfm_valley_scroll_menu_free(m);
    return NULL;
}

void bfm_valley_scroll_menu_free(BfmValleyScrollMenu *m)
{
    if (!m) return;
    bfm_castle_menu_art_free(m->art);
    bfm_forest_message_art_free(m->help_art);
    free(m);
}

int bfm_valley_scroll_menu_active(const BfmValleyScrollMenu *m)
{
    return m && m->page != CLOSED;
}

int bfm_valley_scroll_menu_input(BfmValleyScrollMenu *m, int start_pressed,
                                 BfmFieldInput input, BfmSaveRecord *save)
{
    if (!m || !save) return 0;
    uint8_t *weapon = &save->progress[0x48];
    if (m->page == CLOSED) {
        if (!start_pressed || !(*weapon & 0x7f) || (*weapon & 0x7f) > CHOICES) return 0;
        m->page = SWORD;
        m->row = NORMAL;
        m->equipped = (*weapon & 0x7f)-1;
        m->rows = save->flags[EARTH_FLAG/8] & (1u << (EARTH_FLAG%8)) ? CHOICES : 1;
        m->guardian = u32le(save->progress+8) == 150;
        m->hand_age = m->ink_age = 0;
        return 1;
    }
    ++m->hand_age;
    if (m->ink_age < 6) ++m->ink_age;
    if (start_pressed || (input.triangle_pressed && m->page == SWORD)) {
        m->page = CLOSED;
    } else if (m->page == SWORD && input.cross_pressed) {
        m->page = SCROLLS;
        m->row = NORMAL;
        m->ink_age = 0;
    } else if (m->page == SCROLLS) {
        if (input.triangle_pressed) {
            m->page = SWORD;
            m->ink_age = 0;
        } else if (input.cross_pressed) {
            m->equipped = m->row;
            *weapon = (uint8_t)((*weapon & 0x80) | (m->row+1));
        } else {
            unsigned direction = input.pressed_directions & 0x50u;
            if ((direction == 0x10u || direction == 0x40u) && m->rows == CHOICES) {
                m->row ^= 1;
                m->ink_age = 0;
            }
        }
    }
    return 1;
}

int bfm_valley_scroll_menu_draw(const BfmValleyScrollMenu *m,
    const uint16_t tiles[512][1024], uint32_t pixels[240][320])
{
    if (!m || !tiles || !pixels) return 0;
    if (m->page == CLOSED) return 1;
    int x = 160 + s16le(m->scene+TITLE_XY), y = 120 + s16le(m->scene+TITLE_XY+2);
    int cursor_x = x, cursor_y = y;
    bfm_castle_menu_window(m->scene+FRAMES, m->scene+FILLS, 1, tiles, pixels);
    if (!bfm_castle_menu_label(m->art, m->title, tiles, pixels, x, y, 1)) return 0;
    for (unsigned i = 0; i < m->rows; ++i) {
        x = 160 + s16le(m->row_xy+4*i);
        y = 120 + s16le(m->row_xy+4*i+2);
        BfmCastleMenuLabel name = m->name[i];
        if (i == m->equipped) name.ink = BFM_MENU_LABEL_EQUIPPED;
        if (!bfm_castle_menu_label(m->art, name, tiles, pixels, x, y, 1)) return 0;
        if (m->page == SCROLLS && m->row == i) {
            cursor_x = x;
            cursor_y = y;
        }
    }
    static const int hand_dx[] = {0,0,2,4,4,3,2,1};
    bfm_castle_menu_sprite(tiles, pixels, cursor_x-16+hand_dx[m->hand_age%8], cursor_y-3,
                           16, 16, 112, 16, 16, 16, 1, BFM_MENU_LABEL_NORMAL);
    size_t hint = m->page == SWORD && m->guardian ? GUARDIAN_HINT : m->hint[m->row];
    return bfm_field_menu_hint_draw(m->help_art, m->scene+BOX, tiles, m->main+hint,
                                   m->main_bytes-hint, m->ink_age, pixels);
}
