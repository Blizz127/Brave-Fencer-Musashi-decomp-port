/* SPDX-License-Identifier: MIT */
#include "forest_hud.h"

#include <stdlib.h>
#include <string.h>

enum { SCENE_BASE = 0x128158, FOREST_PANELS = 0x18a1dc - SCENE_BASE,
       BEDROOM_PANELS = 0x185a70 - SCENE_BASE, ABILITY_NAMES = 0x60,
       VITALITY_ROWS = 0xdc, DIAL_ROWS = 0xf0, WEEKDAY_COLORS = 0x124,
       ABILITY_COUNT = 31, MAX_NAME = 24 };
enum { EXE_HEADER = 2048, ALPHABET = EXE_HEADER + 0x636d8 - 0x10000,
       PUNCTUATION = EXE_HEADER + 0x63818 - 0x10000 };
typedef struct {
    int x, y, w, h;
    unsigned uv, palette;
} Sprite;
typedef struct {
    uint16_t glyph[MAX_NAME];
    unsigned count;
} Label;
typedef struct {
    int16_t hp, bp;
} DisplayPoints;
struct BfmForestHud {
    const uint16_t (*tiles)[1024];
    Sprite panel[5];
    Label ability[ABILITY_COUNT];
    uint8_t vitality[20], dial[20], weekday[7];
    uint16_t orb[7], digit[10];
    unsigned inset, remaining, hiding, modal;
    unsigned field_gauges, fill, green, charge_color;
    DisplayPoints points[3];
    int points_ready;
};
typedef struct {
    const BfmForestHud *hud;
    uint32_t (*pixels)[320];
} Canvas;

static unsigned le16(const uint8_t *p)
{
    return p[0] | (unsigned)p[1] << 8;
}
static uint32_t le32(const uint8_t *p)
{
    return le16(p) | (uint32_t)le16(p+2) << 16;
}
static int signed16(const uint8_t *p)
{
    unsigned v = le16(p);
    return v < 32768 ? (int)v : (int)v - 65536;
}

/* The room names use the game's two-byte font encoding. Read the resident
 * glyph-coordinate maps from the executable on the same mounted disc. */
static int read_labels(BfmForestHud *hud, const MusashiDiscMedia *disc,
                       const uint8_t *scene, size_t bytes, const uint8_t *names)
{
    uint8_t alphabet[2 * 80], punctuation[2 * 28];
    size_t size;
    if (!musashi_disc_media_read_root_range(disc, "SLUS_007.26", ALPHABET,
                                            alphabet, sizeof(alphabet), &size) ||
        !musashi_disc_media_read_root_range(disc, "SLUS_007.26", PUNCTUATION,
                                            punctuation, sizeof(punctuation), &size))
        return 0;
    for (unsigned i = 0; i < 10; ++i)
        hud->digit[i] = (uint16_t)le16(alphabet + 2*i);
    for (unsigned i = 0; i < ABILITY_COUNT; ++i) {
        uint32_t ptr = le32(names + 4*i);
        if (ptr < UINT32_C(0x80000000) + SCENE_BASE) return 0;
        size_t offset = ptr - (UINT32_C(0x80000000) + SCENE_BASE);
        Label *label = &hud->ability[i];
        while (offset < bytes && scene[offset]) {
            if (bytes - offset < 2 || label->count == MAX_NAME) return 0;
            unsigned kind = scene[offset], ch = scene[offset+1];
            const uint8_t *uv;
            if (kind == 0x82 && ch >= 0x4f && ch < 0x9f)
                uv = alphabet + 2*(ch - 0x4f);
            else if (kind == 0x81 && ch >= 0x40 && ch <= 0x5b)
                uv = punctuation + 2*(ch - 0x40);
            else
                return 0;
            label->glyph[label->count++] = (uint16_t)le16(uv);
            offset += 2;
        }
        if (offset >= bytes) return 0;
    }
    return 1;
}

static BfmForestHud *load_hud(const MusashiDiscMedia *disc, const uint8_t *scene,
                              size_t bytes, size_t offset, const uint16_t tiles[512][1024])
{
    if (!disc || !scene || !tiles || offset > bytes || bytes - offset < WEEKDAY_COLORS + 7)
        return NULL;
    const uint8_t *records = scene + offset;
    BfmForestHud *hud = calloc(1, sizeof(*hud));
    if (!hud) return NULL;
    hud->tiles = tiles;
    hud->inset = 55;
    for (unsigned i = 0; i < 5; ++i) {
        const uint8_t *p = records + i*16;
        if (p[3] != 0x64 || !le16(p+12) || le16(p+12)>320 || !le16(p+14) || le16(p+14)>240)
            goto fail;
        hud->panel[i] = (Sprite){160+signed16(p+4), 120+signed16(p+6),
                                 (int)le16(p+12), (int)le16(p+14), le16(p+8), le16(p+10)};
    }
    for (unsigned i = 0; i < 7; ++i)
        hud->orb[i] = (uint16_t)le16(records + 80 + i*2);
    memcpy(hud->vitality, records+VITALITY_ROWS, sizeof(hud->vitality));
    memcpy(hud->dial, records+DIAL_ROWS, sizeof(hud->dial));
    memcpy(hud->weekday, records+WEEKDAY_COLORS, sizeof(hud->weekday));
    if (read_labels(hud, disc, scene, bytes, records+ABILITY_NAMES)) return hud;
fail:
    free(hud);
    return NULL;
}
BfmForestHud *bfm_forest_hud_load(const MusashiDiscMedia *disc, const uint8_t *scene,
                                 size_t bytes, const BfmForestTerrain *terrain)
{
    return load_hud(disc, scene, bytes, FOREST_PANELS, bfm_forest_terrain_tiles(terrain));
}

BfmForestHud *bfm_bedroom_hud_load(const MusashiDiscMedia *disc, const uint8_t *scene,
                                  size_t bytes, const uint16_t tiles[512][1024])
{
    return load_hud(disc, scene, bytes, BEDROOM_PANELS, tiles);
}

BfmForestHud *bfm_village_hud_load(const MusashiDiscMedia *disc, const uint8_t *scene,
                                  size_t bytes, const uint16_t tiles[512][1024])
{
    return load_hud(disc, scene, bytes, 0x68a74, tiles);
}

void bfm_forest_hud_free(BfmForestHud *hud)
{
    free(hud);
}
void bfm_forest_hud_show(BfmForestHud *hud)
{
    if (hud && hud->inset == 55 && !hud->remaining) {
        hud->hiding = 0;
        hud->remaining = 10;
    }
}
void bfm_bedroom_hud_hide(BfmForestHud *hud)
{
    if (hud && !hud->inset && !hud->remaining) {
        hud->hiding = 1;
        hud->remaining = 10;
    }
}
void bfm_castle_hud_command(BfmForestHud *hud)
{
    if (hud) hud->modal = 1;
}
void bfm_castle_hud_command_closed(BfmForestHud *hud)
{
    if (hud) hud->modal = 0;
}

void bfm_forest_hud_tick(BfmForestHud *hud)
{
    if (hud && hud->remaining) {
        if (hud->hiding) hud->inset += hud->remaining;
        else hud->inset -= hud->remaining;
        --hud->remaining;
    }
}

static int16_t approach_points(int displayed, int current)
{
    int difference = current - displayed;
    int step = difference > 10 || difference < -10 ? 2 : 1;
    if (difference > 0) return (int16_t)(displayed + step);
    if (difference < 0) return (int16_t)(displayed - step);
    return (int16_t)displayed;
}

void bfm_forest_hud_submit_points(BfmForestHud *hud, const BfmForestStatus *status)
{
    if (!hud || !status) return;
    BfmForestStatusValues v = bfm_forest_status_values(status);
    if (!hud->points_ready) {
        DisplayPoints initial = {v.hp, v.bp};
        for (unsigned i = 0; i < 3; ++i) hud->points[i] = initial;
        hud->points_ready = 1;
        return;
    }
    DisplayPoints previous = hud->points[2];
    hud->points[0] = hud->points[1];
    hud->points[1] = previous;
    hud->points[2] = (DisplayPoints){approach_points(previous.hp, v.hp),
                                     approach_points(previous.bp, v.bp)};
}

static uint32_t rgb(unsigned color)
{
    return UINT32_C(0xff000000) | ((color & 31u)*255/31) << 16 |
           (((color>>5)&31u)*255/31) << 8 | (((color>>10)&31u)*255/31);
}

static void sprite(Canvas *c, const Sprite *s, unsigned xpage)
{
    const uint16_t (*tiles)[1024] = c->hud->tiles;
    unsigned clut_y = (s->palette >> 6) & 511, clut_x = (s->palette & 63)*16;
    for (int iy = 0; iy < s->h; ++iy) {
        int y = s->y + iy;
        if (y < 10 || y >= 230) continue;
        unsigned v = ((s->uv >> 8) + (unsigned)iy) & 255;
        for (int ix = 0; ix < s->w; ++ix) {
            int x = s->x + ix;
            if ((unsigned)x >= 320) continue;
            unsigned u = ((s->uv & 255) + (unsigned)ix) & 255;
            unsigned packed = tiles[256+v][xpage+u/4];
            unsigned color = tiles[clut_y][clut_x + ((packed >> (4*(u&3)))&15)];
            if (color) c->pixels[y][x] = rgb(color);
        }
    }
}
static void glyph(Canvas *c, int x, int y, unsigned uv, unsigned palette)
{
    Sprite s = {x,y,8,8,uv,palette};
    sprite(c,&s,320);
}
static void number(Canvas *c, int x, int y, unsigned value, unsigned slots, unsigned palette)
{
    /* Numbers are right-justified inside their fixed number of cells. */
    do {
        if (!slots) break;
        --slots;
        glyph(c, x + (int)slots*8, y, c->hud->digit[value%10], palette);
        value /= 10;
    } while (value);
}
static void rectangle(Canvas *c, int x, int y, int w, int h, uint32_t color)
{
    int right = x+w < 320 ? x+w : 320, bottom = y+h < 230 ? y+h : 230;
    for (int iy = y < 10 ? 10 : y; iy < bottom; ++iy)
        for (int ix = x < 0 ? 0 : x; ix < right; ++ix)
            c->pixels[iy][ix] = color;
}

void bfm_forest_hud_gauges(BfmForestHud *hud, unsigned charge, int absorption)
{
    if (!hud) return;
    unsigned value = absorption >= 0 ? (unsigned)absorption : charge;
    if (value > 128) value = 128;
    hud->field_gauges = 1;
    if (absorption >= 0 && !hud->green) hud->fill = 0;
    if (absorption >= 0) hud->green = 1;
    if (hud->fill > value) {
        unsigned difference = hud->fill - value;
        unsigned step = value ? 3 : 8;
        hud->fill -= difference < step ? difference : step;
    } else {
        hud->fill = value;
    }
    if (!hud->fill && absorption < 0) hud->green = 0;
    if (charge == 128 && !hud->green) hud->charge_color = hud->charge_color % 3 + 1;
    else hud->charge_color = 0;
}

static void ornaments(Canvas *c, const BfmForestStatusValues *v, int inset)
{
    Sprite s[5];
    memcpy(s, c->hud->panel, sizeof(s));
    s[0].uv = (s[0].uv & 255) | (v->orb ? 0u : 160u << 8);
    if (v->orb & 0x80)
        s[1].uv = c->hud->orb[6];
    else if (v->orb < 8)
        s[1].uv = c->hud->orb[v->orb ? v->orb-1 : 0];
    if (v->face_gauge && v->face_gauge/16 < sizeof(c->hud->dial))
        s[3].uv = (s[3].uv & 255) | (unsigned)c->hud->dial[v->face_gauge/16] << 8;
    else if (!v->face_gauge)
        s[3].uv = (s[3].uv & 255) | 160u << 8;
    if (v->bp >= 0)
        s[4].uv = (s[4].uv & 255) | (v->bp < 100 ? (unsigned)c->hud->vitality[v->bp/5] << 8 : 0);
    /* The field HUD places the meter 256 pixels lower in a modal menu. */
    int meter_y = c->hud->modal ? 256 : 0;
    /* RGB555. Full stance charge cycles the three bright primaries. */
    static const unsigned charge_tints[] = {0x085f, 0x18df, 0x1be6, 0x7cc6};
    unsigned tint = c->hud->green ? 0x0be2 : charge_tints[c->hud->charge_color];
    int charge = c->hud->field_gauges ? (int)(c->hud->fill * 9 / 16) : v->charge * 3 / 8;
    int charge_active = c->hud->field_gauges ? (charge || c->hud->green) : v->charge;
    rectangle(c,8-inset,meter_y+129+inset,8,72,rgb(charge_active ? 0x5108 : 0x2108));
    rectangle(c,8-inset,meter_y+201+inset-charge,8,charge,rgb(tint));
    /* Frames overlap the orbs and the charge well in this layer. */
    for (int i = 4; i >= 0; --i) {
        s[i].x += i < 3 ? -inset : inset;
        s[i].y += inset + (i == 2 ? meter_y : 0);
        sprite(c,&s[i],384);
    }
}

static void points(Canvas *c, int x, int y, unsigned current, unsigned max,
                   int bp)
{
    unsigned palette = bp ? 0x6e16 : 0x6b56;
    int bar_y = bp ? y-4 : y+10;
    /* Both bars use a common pixel scale, so increasing the maximum extends
     * the HUD outline as well as the available fill. */
    int total = (int)(max * 20710u / 65536), fill = (int)(current * 20710u / 65536);
    rectangle(c,x,bar_y-1,total+2,4,rgb(0));
    rectangle(c,x+1,bar_y,total,2,rgb(bp ? 0x7cc2 : 31));
    rectangle(c,x+1,bar_y,fill,2,rgb(bp ? 0x2be0 : 0x037f));
    glyph(c,x,y,bp ? 0x5010 : 0x5040,palette);
    glyph(c,x+8,y,0x5808,palette);
    glyph(c,x+16,y,0x4060,palette);
    glyph(c,x+48,y,0x3858,palette);
    number(c,x+24,y,current,3,palette);
    number(c,x+56,y,max,3,palette);
}

static void calendar(Canvas *c, const BfmForestStatusValues *v, int x, int y, unsigned clock_palette)
{
    if (v->weekday >= 7) return;
    unsigned palette = 0x4016 | (unsigned)c->hud->weekday[v->weekday] << 6;
    glyph(c,x-3,y+1,0x1800,palette);
    glyph(c,x+5,y+1,0x1808,palette);
    number(c,x+14,y,v->day < 1000 ? v->day : 999,3,palette);
    glyph(c,x+42,y,0x1000 + v->weekday*16,palette);
    glyph(c,x+50,y,0x1008 + v->weekday*16,palette);
    unsigned quarter = v->minutes_of_day / 15;
    number(c,x+61,y,quarter/4,2,clock_palette);
    glyph(c,x+76,y,0x3850,clock_palette);
    unsigned minutes = quarter % 4 * 15;
    glyph(c,x+83,y,c->hud->digit[minutes/10],clock_palette);
    glyph(c,x+91,y,c->hud->digit[minutes%10],clock_palette);
}

void bfm_forest_hud_draw(const BfmForestHud *hud, const BfmForestStatus *status,
                         uint32_t pixels[240][320])
{
    if (!hud || !status || !pixels || hud->inset == 55) return;
    BfmForestStatusValues v = bfm_forest_status_values(status);
    if (hud->points_ready) {
        v.hp = hud->points[0].hp;
        v.bp = hud->points[0].bp;
    }
    Canvas c = {hud,pixels};
    int inset = (int)hud->inset, x = 217+inset, y = 201+inset;
    ornaments(&c,&v,inset);
    points(&c,47-inset,y,v.hp < 0 ? 0 : (unsigned)v.hp,v.max_hp,0);
    points(&c,47-inset,y+20,v.bp < 0 ? 0 : (unsigned)v.bp,v.max_bp,1);
    glyph(&c,x,y,0x4000,0x6b56);
    number(&c,x+8,y,v.wallet,5,0x6b56);
    glyph(&c,x+48,y,hud->digit[0],0x6b56);
    unsigned story = le16(status->record + 8);
    calendar(&c,&v,x,y+20,story - 10u < 1200u ? 0x6d16 : 0x4016);
    if (v.ability_label < ABILITY_COUNT) {
        const Label *label = &hud->ability[v.ability_label];
        for (unsigned i = 0; i < label->count; ++i) {
            unsigned uv = label->glyph[i];
            glyph(&c,x,y+10,uv,0x4056);
            x += uv < 0x0861 ? 7 : 8;
        }
    }
}
