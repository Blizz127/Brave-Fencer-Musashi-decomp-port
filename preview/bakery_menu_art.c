/* SPDX-License-Identifier: MIT */
#include "bakery_menu_art.h"
#include "bedroom_score.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>
#include <string.h>

enum { PANES = 3, SERVICES = 3 };
static const uint32_t village_origin = UINT32_C(0x80128158);

struct BfmBakeryMenuArt {
    BfmShopListLayout panels[PANES];
    BfmShopLabel actions[SERVICES];
    uint8_t alphabet[2*80], marks[2*28];
};

static const uint8_t *record(const uint8_t *data, size_t bytes, uint32_t address, size_t length)
{
    if (address < village_origin || length > bytes || address-village_origin > bytes-length)
        return NULL;
    return data + (address-village_origin);
}

static int read_name(const BfmBakeryMenuArt *art, BfmBakeryName name, BfmShopLabel *out)
{
    if (!name.name || name.name_bytes % 2 || name.name_bytes / 2 > sizeof out->uv / sizeof out->uv[0])
        return 0;
    out->count = 0;
    for (size_t i = 0; i < name.name_bytes; i += 2) {
        unsigned lead = name.name[i], ch = name.name[i+1];
        const uint8_t *uv;
        if (lead == 0x82 && ch >= 0x4f && ch < 0x9f) uv = art->alphabet + 2*(ch-0x4f);
        else if (lead == 0x81 && ch >= 0x40 && ch <= 0x5b) uv = art->marks + 2*(ch-0x40);
        else return 0;
        if (uv[0] > 248 || uv[1] > 248) return 0;
        out->uv[out->count++] = u16le(uv);
    }
    return 1;
}

static int static_name(const BfmBakeryMenuArt *art, const uint8_t *village, size_t bytes,
                       uint32_t address, int action, BfmShopLabel *out)
{
    const uint8_t *text = record(village, bytes, address, 2);
    if (!text) return 0;
    if (action) {
        if (text[0] != 0x81 || text[1] != 0x41) return 0;
        text += 2;
    }
    const uint8_t *end = memchr(text, 0, bytes - (size_t)(text - village));
    return end && end != text && read_name(art, (BfmBakeryName){text, (size_t)(end-text)}, out);
}

static int read_rect(const uint8_t *row, BfmShopRect *r)
{
    *r = (BfmShopRect){160+s16le(row), 120+s16le(row+2), s16le(row+4), s16le(row+6)};
    return r->x >= 0 && r->y >= 0 && r->w > 0 && r->h > 0 &&
           r->x + r->w <= 320 && r->y + r->h <= 240;
}

static int read_panel(BfmBakeryMenuArt *art, BfmBakeryPane pane, const uint8_t *scene,
                      size_t bytes, const uint8_t *row)
{
    BfmShopListLayout *p = &art->panels[pane];
    unsigned count = pane == BFM_BAKERY_SERVICES ? SERVICES :
                     pane == BFM_BAKERY_GOODS ? BFM_BAKERY_VISIBLE_ROWS : BFM_BAKERY_BAG_SLOTS;
    const uint8_t *borders = record(scene, bytes, u32le(row+4), 8*12);
    const uint8_t *fills = record(scene, bytes, u32le(row+8), 2*16);
    const uint8_t *positions = record(scene, bytes, u32le(row+24), count*4);
    if (!borders || !fills || !positions || u16le(row+12) != 8 || u16le(row+14) != 2) return 0;
    for (unsigned i = 0; i < 8; ++i, borders += 12) {
        BfmShopBorder *edge = &p->borders[i];
        if (!read_rect(borders, &edge->rect)) return 0;
        edge->u = u16le(borders+8);
        edge->v = u16le(borders+10);
        if (edge->u > 248 || edge->v > 248) return 0;
    }
    for (unsigned i = 0; i < 2; ++i, fills += 16) {
        if (!read_rect(fills+8, &p->fills[i].rect)) return 0;
        memcpy(p->fills[i].top, fills, 3);
        memcpy(p->fills[i].bottom, fills+4, 3);
    }
    p->title_x = 160 + s16le(row+16);
    p->title_y = 120 + s16le(row+18);
    if (!static_name(art, scene, bytes, u32le(row+20), 0, &p->title)) return 0;
    for (unsigned i = 0; i < count; ++i) {
        p->row_x[i] = 160 + s16le(positions+4*i);
        p->row_y[i] = 120 + s16le(positions+4*i+2);
    }
    if (pane == BFM_BAKERY_SERVICES) {
        const uint8_t *labels = record(scene, bytes, u32le(row+28), SERVICES*4);
        if (!labels) return 0;
        for (unsigned i = 0; i < SERVICES; ++i)
            if (!static_name(art, scene, bytes, u32le(labels+4*i), 1, &art->actions[i])) return 0;
    }
    return 1;
}

BfmBakeryMenuArt *bfm_bakery_menu_art_load(const MusashiDiscMedia *disc,
                                          const uint8_t *village, size_t bytes)
{
    if (!disc || !village || bytes < 0x994f4 + 4) return NULL;
    const uint8_t *panels = record(village, bytes, u32le(village+0x994f4), PANES*32);
    if (!panels) return NULL;
    BfmBakeryMenuArt *art = calloc(1, sizeof *art);
    if (!art) return NULL;
    if (!bfm_bedroom_resident_data(disc, UINT32_C(0x800636d8), art->alphabet, sizeof art->alphabet) ||
        !bfm_bedroom_resident_data(disc, UINT32_C(0x80063818), art->marks, sizeof art->marks))
        goto fail;
    for (unsigned i = 0; i < PANES; ++i)
        if (!read_panel(art, (BfmBakeryPane)i, village, bytes, panels+32*i)) goto fail;
    return art;
fail:
    free(art);
    return NULL;
}

void bfm_bakery_menu_art_free(BfmBakeryMenuArt *art)
{
    free(art);
}

int bfm_bakery_menu_draw(const BfmBakeryMenuArt *menu, const BfmSaveArt *art,
                         const BfmBakeryMenuView *view, uint32_t pixels[240][320])
{
    if (!menu || !art || !view || !pixels || (unsigned)view->active >= PANES ||
        view->service >= SERVICES || view->good >= BFM_BAKERY_VISIBLE_ROWS ||
        view->item >= BFM_BAKERY_BAG_SLOTS || view->first_good > view->goods_count) return 0;
    BfmShopLabel goods[BFM_BAKERY_VISIBLE_ROWS] = {0}, items[BFM_BAKERY_BAG_SLOTS] = {0};
    unsigned count = 0;
    while (count < BFM_BAKERY_VISIBLE_ROWS && view->goods[count].name) {
        if (!read_name(menu, view->goods[count], &goods[count])) return 0;
        ++count;
    }
    for (unsigned i = 0; i < BFM_BAKERY_BAG_SLOTS; ++i)
        if (view->items[i].name && !read_name(menu, view->items[i], &items[i])) return 0;
    bfm_save_art_shop_list(art, pixels, &menu->panels[0], BFM_BAKERY_SERVICES,
                           view, view->service, menu->actions, NULL, SERVICES);
    bfm_save_art_shop_list(art, pixels, &menu->panels[1], BFM_BAKERY_GOODS,
                           view, view->good, goods, view->drans, count);
    bfm_save_art_shop_list(art, pixels, &menu->panels[2], BFM_BAKERY_ITEMS,
                           view, view->item, items, NULL, BFM_BAKERY_BAG_SLOTS);
    return 1;
}
