/* SPDX-License-Identifier: MIT */
#include "woods_box_view.h"
#include "woods_box_water.h"
#include "woods_water_effect.h"
#include "woods_pole_view.h"
#include "field_waypoint.h"
#include "scene_tiles.h"
#include "village_sky.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static const uint32_t base = UINT32_C(0x80128158), room = UINT32_C(0x801961e8);
enum { PALETTE_TRANSFERS = 20, PHASES = 5 };

typedef struct { unsigned sx, sy, x, y, width; } PaletteTransfer;
struct BfmWoodsBoxView {
    BfmVillageField field;
    BfmWoodsBoxWater water;
    BfmForestWaterEffect splash;
    BfmVillageCamera camera;
    BfmWoodsPoleView pole;
    const uint8_t *scene, *quarter, *gains, *lamp_colors, *sky_colors;
    size_t bytes;
    uint16_t (*tiles)[1024];
    PaletteTransfer transfers[PALETTE_TRANSFERS];
    BfmBedroomLight lamps[3];
    uint8_t gain[3], sky[3];
    unsigned phase, wait, blending, near_zoom;
    int pitch, height_offset;
    int hide_body, hide_shadow, camera_effect_y;
};

static unsigned read16(const uint8_t *p) { return p[0] | (unsigned)p[1] << 8; }
static uint32_t read32(const uint8_t *p) { return read16(p) | (uint32_t)read16(p+2) << 16; }

static const uint8_t *at(const BfmWoodsBoxView *v, uint32_t address, size_t length)
{
    if (address < base || (size_t)(address - base) > v->bytes) return NULL;
    size_t offset = address - base;
    return length <= v->bytes - offset ? v->scene + offset : NULL;
}

static int palette_directory(BfmWoodsBoxView *v, const uint8_t *descriptor)
{
    if (read32(descriptor + 0xa4) != 0x11) return 0;
    const uint8_t *table = at(v,read32(descriptor + 0xa0),2 + PALETTE_TRANSFERS*16);
    if (!table || read16(table) != PALETTE_TRANSFERS) return 0;
    for (unsigned i = 0; i < PALETTE_TRANSFERS; ++i) {
        const uint8_t *entry = table+2+16*i;
        unsigned u = read16(entry), y = read16(entry+2), page = read16(entry+4);
        PaletteTransfer t = {
            .sx = (page & 15u)*64 + u, .sy = (page & 16u)*16 + y,
            .x = read16(entry+8), .y = read16(entry+10), .width = read16(entry+12)
        };
        if (((page >> 7) & 3u) != 2 || u > 255 || y > 255 ||
            !t.width || t.width > 1024 || t.sx > 1024-t.width || t.x > 1024-t.width ||
            t.sy >= 512 || t.y >= 512 || read16(entry+14) != 1) return 0;
        v->transfers[i] = t;
    }
    return 1;
}

static int toward(int from, int to, int maximum)
{
    int d = to - from;
    return from + (d > maximum ? maximum : d < -maximum ? -maximum : d);
}

static void palettes(BfmWoodsBoxView *v)
{
    for (unsigned i = 0; i < PALETTE_TRANSFERS; ++i) {
        const PaletteTransfer *t = &v->transfers[i];
        const uint16_t *src = &v->tiles[t->sy][t->sx];
        uint16_t *dest = &v->tiles[t->y][t->x];
        for (unsigned x = 0; x < t->width; ++x) {
            unsigned original = src[x], color = original & 0x8000u;
            if (!original) continue;
            for (unsigned c = 0; c < 3; ++c) {
                unsigned value = (((original >> (5*c)) & 31u)*v->gain[c]) >> 7;
                color |= (value < 32 ? value : 31u) << (5*c);
            }
            dest[x] = (uint16_t)color;
        }
    }
}

static void light_step(BfmWoodsBoxView *v, int entering)
{
    unsigned pending = 0, recolor = 0;
    for (unsigned c = 0; c < 3; ++c) {
        int incoming = (int)read32(v->gains + (v->phase*3+c)*4);
        int goal = (int)read32(v->sky_colors + (v->phase*3+c)*4);
        uint8_t new_gain = (uint8_t)(entering ? incoming : toward(v->gain[c],incoming,4));
        recolor |= new_gain != v->gain[c];
        v->gain[c] = new_gain;
        v->sky[c] = (uint8_t)(entering ? goal : toward(v->sky[c],goal,1));
        pending |= new_gain != incoming || v->sky[c] != goal;
        for (unsigned i = 0; i < 3; ++i) {
            int channel = v->lamp_colors[v->phase*9+i*3+c];
            uint8_t *lamp = &v->lamps[i].rgb[c];
            *lamp = (uint8_t)(entering ? channel : toward(*lamp,channel,8));
            pending |= *lamp != channel;
        }
    }
    if (entering || recolor) palettes(v);
    v->blending = pending;
}

BfmWoodsBoxView *bfm_woods_box_view_open(const MusashiDiscMedia *disc, const uint8_t *scene,
                                         size_t bytes, unsigned phase, uint16_t tiles[512][1024])
{
    if (!disc || !scene || !tiles || phase >= PHASES) return NULL;
    BfmWoodsBoxView *v = calloc(1,sizeof *v);
    if (!v) return NULL;
    v->scene = scene;
    v->bytes = bytes;
    v->phase = phase;
    v->quarter = at(v,0x801d66c8u,2048);
    v->gains = at(v,0x801914c8u,PHASES*12);
    v->lamp_colors = at(v,0x80191660u,PHASES*9);
    v->sky_colors = at(v,0x80191698u,PHASES*12);
    const uint8_t *descriptor = at(v,room,0xa8);
    /* Twinpeak's column and ring are adjacent scene-local sprite cells. */
    const uint8_t *splash = at(v,UINT32_C(0x8019471c),24);
    if (!descriptor || !v->quarter || !v->gains || !v->lamp_colors || !v->sky_colors ||
        !bfm_woods_box_water_load(&v->water,scene,bytes) ||
        !bfm_forest_water_effect_load_cells(&v->splash,splash,24) ||
        !palette_directory(v,descriptor) || !bfm_woods_pole_view_load(&v->pole,scene,bytes)) goto fail;
    for (unsigned i = 0; i < PHASES*3; ++i)
        if (read32(v->gains+i*4)>255 || read32(v->sky_colors+i*4)>16) goto fail;
    if (!bfm_village_field_load_room(&v->field,disc,scene,bytes,room,0x80192b5cu)) goto fail;

    /* Commit both sparse uploads and the destination palettes as a unit. */
    uint16_t (*staged)[1024] = malloc(512*sizeof *staged);
    if (!staged) goto fail;
    memcpy(staged,tiles,512*sizeof *staged);
    if (!bfm_scene_read_tiles(disc,"MAIN.CD",10,staged) ||
        !bfm_scene_read_tiles(disc,"SC02.CD",11,staged)) {
        free(staged);
        goto fail;
    }
    static const BfmBedroomLight defaults[3] = {
        {{0,30,0},{0}}, {{0,7,20},{0}}, {{0,-18,0},{0}}
    };
    memcpy(v->lamps,defaults,sizeof defaults);
    v->tiles = staged;
    light_step(v,1);
    memcpy(tiles,staged,512*sizeof *staged);
    v->tiles = tiles;
    free(staged);
    return v;
fail:
    bfm_woods_box_view_free(v);
    return NULL;
}

void bfm_woods_box_view_free(BfmWoodsBoxView *v)
{
    if (v) {
        bfm_village_field_clear(&v->field);
        free(v);
    }
}

const BfmVillageCamera *bfm_woods_box_view_camera(const BfmWoodsBoxView *v)
{
    return v ? &v->camera : NULL;
}

void bfm_woods_box_view_shake(BfmWoodsBoxView *v, int offset)
{
    if (v) v->camera_effect_y = offset;
}

void bfm_woods_box_view_present(BfmWoodsBoxView *v, const BfmVillageCamera *camera)
{
    if (v && camera) v->camera = *camera;
}

BfmVillageField *bfm_woods_box_view_field(BfmWoodsBoxView *v)
{
    return v ? &v->field : NULL;
}

int bfm_woods_box_view_water_step(BfmWoodsBoxView *v, const int16_t splash_at[3],
                                  int hide_body, int hide_shadow, BfmGameplayRun *run)
{
    if (!v || (splash_at && !run)) return 0;
    bfm_forest_water_effect_tick(&v->splash);
    if (splash_at && !bfm_forest_water_effect_begin(&v->splash,splash_at,v->camera.eye,run)) return 0;
    v->hide_body = hide_body;
    v->hide_shadow = hide_shadow;
    return 1;
}

void bfm_woods_box_view_light(BfmWoodsBoxView *v, unsigned phase)
{
    if (!v || phase >= PHASES) return;
    if (phase != v->phase) {
        v->phase = phase;
        v->blending = 1;
        v->wait = 0;
    }
    if (!v->blending) return;
    if (v->wait) {
        --v->wait;
        return;
    }
    light_step(v,0);
    v->wait = 1;
}

static int quarter_move(int from, int to, int divisor)
{
    int d = to-from;
    while (divisor > 1 && d/divisor == 0) divisor /= 2;
    return from + d/divisor;
}

static int sine(const BfmWoodsBoxView *v, unsigned angle)
{
    unsigned i = angle & 1023;
    if (angle & 1024) i = 1023-i;
    int magnitude = (int)read16(v->quarter+2*i);
    return angle & 2048 ? -magnitude : magnitude;
}

static int product(int a, int b)
{
    int p = a*b;
    return p >= 0 ? p/32768 : -((-p+32767)/32768);
}

/* The room selects its zoom bands using a squared radius rounded down to
 * a base-four mantissa byte. The boundary is visible on the first pole approach. */
static int radial_distance(int x, int z)
{
    unsigned squared = (unsigned)(x*x) + (unsigned)(z*z);
    unsigned scale = 1;
    while (squared > 255) {
        squared >>= 2;
        scale *= 2;
    }
    unsigned root = (unsigned)(sqrt((double)squared) * 512.0);
    return (int)(root * scale / 512);
}

static void ordinary_camera(BfmWoodsBoxView *v, const int16_t foot[3],
                            int entering, int shoulder_pressed)
{
    if (!v || !foot) return;
    BfmVillageCamera *cam = &v->camera;
    if (entering) {
        *cam = (BfmVillageCamera){.focal=400,.radius=1200,.following=1};
        /* A callback change resets tracking. The shoulder zoom belongs to
         * the whole room visit, including the return from a staged camera. */
        v->pitch=455; v->height_offset=0;
    }
    if (shoulder_pressed) v->near_zoom ^= 1;
    int yaw = bfm_waypoint_heading(foot[0],foot[2]);
    int radial = radial_distance(foot[0],foot[2]);
    int pitch=455, radius=v->near_zoom || radial>2432 ? 900 : 1200, offset_y=0;
    int x=foot[0], y=foot[1], z=foot[2];
    int sin_yaw=sine(v,(unsigned)yaw), cos_yaw=sine(v,(unsigned)yaw+1024);
    if (radial>2560) {
        x=product(2560,sin_yaw); y=product(y,32760); z=product(2560,cos_yaw);
    }
    if (radial<512) {
        pitch=227; offset_y=-192;
    } else if ((yaw<528 || yaw>2816) && radial>960 && y>=-767) {
        radius=1200;
        pitch=yaw<=3328 ? 682 : radial<1280 ? 796 : 910;
        if (yaw>=3329 && yaw<=3583 && radial>=1153 && radial<=1823) pitch=455;
    }
    cam->yaw=yaw;
    v->pitch=quarter_move(v->pitch,pitch,4);
    v->height_offset=quarter_move(v->height_offset,offset_y,16);
    cam->radius=quarter_move(cam->radius,radius,4);
    cam->target[0]=x; cam->target[1]=y+v->height_offset; cam->target[2]=z;

    int horizontal=sine(v,(unsigned)v->pitch+1024), vertical=sine(v,(unsigned)v->pitch);
    int cos_roll=sine(v,1024);
    int ax=product(product(horizontal,cos_roll),sin_yaw);
    int ay=-product(vertical,cos_roll), az=product(cos_yaw,horizontal);
    cam->eye[0]=cam->target[0]+product(ax,cam->radius);
    cam->eye[1]=cam->target[1]+product(ay,cam->radius);
    cam->eye[2]=cam->target[2]+product(az,cam->radius);
}

void bfm_woods_box_view_follow_grip(BfmWoodsBoxView *v, const int16_t foot[3],
                                    int entering, int shoulder_pressed, int gripped)
{
    if (!v || !foot) return;
    int16_t reference[3];
    bfm_woods_pole_view_reference(&v->pole,foot,entering,reference);
    if (!bfm_woods_pole_view_active(&v->pole))
        ordinary_camera(v,reference,entering,shoulder_pressed);
    bfm_woods_pole_view_step(&v->pole,gripped,foot,reference,v->pitch,v->height_offset,&v->camera);
}

void bfm_woods_box_view_follow(BfmWoodsBoxView *v, const int16_t foot[3],
                               int entering, int shoulder_pressed)
{
    bfm_woods_box_view_follow_grip(v,foot,entering,shoulder_pressed,0);
}

int bfm_woods_box_view_draw(BfmWoodsBoxView *v, const BfmBedroomRoot *root,
    const BfmBedroomPose *pose, const uint8_t flags[64], unsigned age, unsigned scale_q12,
    uint32_t pixels[240][320],
    int (*prepare_actor)(void *, BfmBedroomFigure *, const BfmBedroomView *), void *actor)
{
    if (!v || !root || !pose || !flags || !pixels) return 0;
    BfmVillageCamera presented = v->camera;
    presented.eye[1] += v->camera_effect_y;
    presented.target[1] += v->camera_effect_y;
    BfmBedroomRoot displayed = *root;
    if (v->hide_shadow) displayed.shadow_visible = 0;
    uint32_t parts = v->hide_body ? 0 : bfm_village_field_visible(pose,flags);
    if (!bfm_bedroom_figure_lights(v->field.hero,v->lamps) ||
        !bfm_village_sky_draw_lit(v->scene+(room-base),v->bytes-(room-base),v->sky,pixels) ||
        !bfm_village_field_prepare_scaled(&v->field,&presented,&displayed,pose,flags,age,
            parts,scale_q12,v->tiles,&pixels[0][0]))
        return 0;
    const BfmBedroomView *view = bfm_bedroom_field_view(v->field.field);
    if (!bfm_woods_water_effect_append(&v->splash,v->field.hero,view) ||
        (prepare_actor && !prepare_actor(actor,v->field.hero,view))) return 0;
    bfm_woods_box_water_draw(&v->water,age,v->field.field,v->tiles,&pixels[0][0],
                            bfm_bedroom_figure_draw_to,v->field.hero);
    return 1;
}
