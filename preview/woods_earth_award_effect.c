/* SPDX-License-Identifier: MIT */
#include "woods_earth_award_effect.h"
#include "forest_tether_trails.h"
#include "field_waypoint.h"
#include "scene_tiles.h"
#include "../pc_port/forest_player_bytes.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

enum { UNIT = 4096, CHILDREN = 29, RAYS = 9, RAY_CARDS = 32,
       MAX_CARDS = 1 + RAYS * RAY_CARDS + CHILDREN * 6 };
enum MedalPhase { IDLE, GROW, GATHER, RAY_DELAY, MESSAGE, RETIRE };
enum ChildKind { EMPTY, FLASH, STREAK, GLOW, RAY };
enum Cue { APPEAR, RAY_START, VANISH, STREAK_SOUND, GLOW_SOUND, CUE_COUNT };

typedef struct { int x, y; } Point;
typedef struct { int sine, cosine; } Rotation;
typedef struct {
    enum ChildKind kind;
    unsigned age, angle;
    int delay, acceleration, scale, speed, light, rim;
    unsigned cue_timer;
    int radius[2], velocity[2];
    Rotation rotation;
} EarthChild;

struct BfmWoodsEarthAwardEffect {
    uint16_t image[128][32], cue[CUE_COUNT];
    int width, height, left, top, camera_bias;
    int dx[4], dy[4], spread[4];
    uint32_t ray_rgb, flash_rgb, streak_rgb[2];
    enum MedalPhase phase;
    unsigned counter, bucket, angle;
    int scale, speed, visible, projected;
    Point anchor;
    EarthChild child[CHILDREN];
    BfmForestDustCard cards[MAX_CARDS];
    unsigned count;
};

static int fixed(int64_t value)
{
    return (int)(value / UNIT - (value % UNIT < 0));
}

static int sine(unsigned angle)
{
    return (int)lround(UNIT * sin((angle & 4095u) * (6.2831853071795864769 / UNIT)));
}

static Rotation rotation(unsigned angle)
{
    return (Rotation){sine(angle), sine(angle + 1024)};
}

static BfmForestRasterVertex vertex(Point anchor, Rotation r, Point p, uint32_t color)
{
    return (BfmForestRasterVertex){
        .x = anchor.x + fixed((int64_t)r.cosine * p.x - (int64_t)r.sine * p.y),
        .y = anchor.y + fixed((int64_t)r.sine * p.x + (int64_t)r.cosine * p.y), .rgb = color
    };
}

static BfmForestDustCard *additive(BfmWoodsEarthAwardEffect *e, unsigned bucket)
{
    BfmForestDustCard *card = &e->cards[e->count++];
    *card = (BfmForestDustCard){.bucket = bucket,
        .material = {.count = 4, .translucent = 1, .page = 32}};
    if (card->bucket >= 4096) card->bucket = 4095;
    return card;
}

BfmWoodsEarthAwardEffect *bfm_woods_earth_award_effect_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes)
{
    /* Relative to the mounted summit field. Colors, ray endpoints, shape and
     * element-specific sounds all come from the same ceremony definition. */
    const size_t art_offset = 0x80194ba4u - 0x80128158u;
    if (!disc || !scene || art_offset > bytes || bytes - art_offset < 0x112) return NULL;
    const uint8_t *art = scene + art_offset, *shape = art + 0xe8;
    if (shape[2] != 127 || shape[3] != 127) return NULL;
    BfmWoodsEarthAwardEffect *e = calloc(1, sizeof *e);
    if (!e) return NULL;
    uint16_t (*tiles)[1024] = calloc(512, sizeof *tiles);
    if (!tiles || !bfm_scene_read_tiles(disc, "SC03.CD", 60, tiles)) {
        free(tiles);
        free(e);
        return NULL;
    }
    for (unsigned y = 0; y < 128; ++y) memcpy(e->image[y], tiles[256+y]+640, sizeof e->image[y]);
    free(tiles);
    e->width = shape[2];
    e->height = shape[3];
    e->left = s16le(shape + 8);
    e->top = s16le(shape + 10);
    e->camera_bias = s16le(art + 0x110);
    e->ray_rgb = u32le(art) & 0xffffff;
    e->flash_rgb = u32le(art + 20) & 0xffffff;
    e->streak_rgb[0] = u32le(art + 40) & 0xffffff;
    e->streak_rgb[1] = u32le(art + 60) & 0xffffff;
    for (unsigned i = 0; i < CUE_COUNT; ++i) e->cue[i] = u16le(art + 80 + i * 20);
    for (unsigned i = 0; i < 4; ++i) {
        const uint8_t *row = art + 0xc8 + 8*i;
        e->dx[i] = s16le(row);
        e->dy[i] = s16le(row + 2);
        e->spread[i] = u16le(row + 6);
    }
    return e;
}

void bfm_woods_earth_award_effect_free(BfmWoodsEarthAwardEffect *effect)
{
    free(effect);
}

static void sound(const BfmWoodsEarthAwardEffect *e, BfmEarthAwardEffectFrame *out, enum Cue which)
{
    out->sounds[out->sound_count++] = e->cue[which];
}

static EarthChild *spawn_child(BfmWoodsEarthAwardEffect *e, enum ChildKind kind)
{
    for (unsigned i = 0; i < CHILDREN; ++i) {
        if (e->child[i].kind != EMPTY) continue;
        e->child[i] = (EarthChild){.kind = kind};
        return &e->child[i];
    }
    return NULL;
}

static void flash(BfmWoodsEarthAwardEffect *e, int white)
{
    EarthChild *p = spawn_child(e, FLASH);
    if (!p) return;
    p->scale = white ? 128 : UNIT;
    p->acceleration = white ? 128 : 32;
    p->light = white ? 255 : (int)(e->flash_rgb & 255);
}

static void place(BfmWoodsEarthAwardEffect *e, const BfmForestTransform *hero,
                  const BfmBedroomView *view, uint16_t tiles[512][1024])
{
    /* The field derives two Euler angles from eye minus target. The model
     * offset uses the pitch rotation after yaw, before the room projection. */
    int64_t dx = (int64_t)view->eye[0] - view->target[0];
    int64_t dy = (int64_t)view->eye[1] - view->target[1];
    int64_t dz = (int64_t)view->eye[2] - view->target[2];
    int64_t horizontal = (int64_t)hypot((double)dx, (double)dz);
    Rotation pitch = rotation(bfm_waypoint_heading(dy,horizontal));
    Rotation yaw = rotation(bfm_waypoint_heading(dx,dz));
    int direction[3] = {yaw.sine, fixed(-yaw.cosine*pitch.sine), fixed(yaw.cosine*pitch.cosine)};
    int16_t world[3];
    for (unsigned i = 0; i < 3; ++i) {
        int offset = fixed(-96 * hero->rotation[i][1]);
        int bias = fixed((int64_t)e->camera_bias * direction[i]);
        world[i] = (int16_t)(hero->translation[i] + offset + bias);
    }
    int screen[3];
    e->projected = bfm_bedroom_project(view, world, screen);
    e->anchor = (Point){screen[0], screen[1]};
    e->bucket = e->projected ? (unsigned)screen[2] / 4 : 0;
    for (unsigned y = 0; y < 128; ++y) memcpy(tiles[256+y]+640, e->image[y], sizeof e->image[y]);
}

static void medal_card(BfmWoodsEarthAwardEffect *e)
{
    if (!e->visible || e->scale <= 0 || (e->bucket & 0x3fff) >= 4096) return;
    BfmForestDustCard *card = &e->cards[e->count++];
    *card = (BfmForestDustCard){.bucket = e->bucket & 0x3fff,
        .material = {.page = 0x3a, .clut = (384u << 6) | (352u / 16), .count = 4, .textured = 1}};
    int horizontal = fixed((int64_t)e->scale * sine(e->angle + 1024));
    for (unsigned i = 0; i < 4; ++i) {
        int right = i & 1, bottom = (i >> 1) & 1;
        card->vertices[i] = (BfmForestRasterVertex){
            .x = e->anchor.x + fixed((int64_t)(e->left + right*e->width) * horizontal),
            .y = e->anchor.y + fixed((int64_t)(e->top + bottom*e->height) * e->scale),
            .u = (uint8_t)(right*(e->width-1)), .v = (uint8_t)(bottom*(e->height-1)), .rgb = 0x808080};
    }
}

static void ray(BfmWoodsEarthAwardEffect *e, Rotation r, BfmGameplayRun *run)
{
    for (unsigned strip = 0; strip < 4; ++strip) {
        Point center = {0}, edges[2] = {{0},{0}};
        for (unsigned i = 0; i < 4; ++i) {
            int spread = e->spread[i];
            int jitter = bfm_gameplay_random(run) & spread;
            Point next = {center.x + e->dx[i] + (((strip+i)&1) ? jitter : -jitter),
                          center.y + e->dy[i] + (bfm_gameplay_random(run)&31) - 16};
            for (unsigned side = 0; side < 2; ++side) {
                int width = bfm_gameplay_random(run) & spread;
                Point edge = {next.x + (side ? width : -width), next.y};
                BfmForestDustCard *c = additive(e, e->bucket + 16);
                c->vertices[0] = vertex(e->anchor,r,center,e->ray_rgb);
                c->vertices[1] = vertex(e->anchor,r,next,e->ray_rgb);
                c->vertices[2] = vertex(e->anchor,r,edges[side],0);
                c->vertices[3] = vertex(e->anchor,r,edge,0);
                edges[side] = edge;
            }
            center = next;
        }
    }
}

static int fade(int brightness)
{
    return brightness > 0 ? brightness - (brightness >= 10 ? brightness / 10 : 1) : 0;
}

static void disc_cards(BfmWoodsEarthAwardEffect *e, const EarthChild *p)
{
    int glow = p->kind == GLOW;
    int radius = glow ? 256 : 80;
    unsigned bucket = glow ? 16 : e->bucket + 16;
    unsigned channels = glow || p->acceleration == 128 ? 0x010101 : 0x000101;
    uint32_t color = channels * (unsigned)p->light, edge = 0x010101u * (unsigned)p->rim;
    Point rim[12];
    for (unsigned i = 0; i < 4; ++i) {
        Rotation r = rotation(i*341);
        rim[i] = (Point){fixed(radius*r.sine),fixed(radius*r.cosine)};
    }
    for (unsigned i = 4; i < 7; ++i) rim[i] = (Point){rim[6-i].x,-rim[6-i].y};
    for (unsigned i = 7; i < 12; ++i) rim[i] = (Point){-rim[12-i].x,rim[12-i].y};
    BfmForestRasterVertex vertices[12];
    for (unsigned i = 0; i < 12; ++i)
        vertices[i] = (BfmForestRasterVertex){.x=e->anchor.x+fixed(rim[i].x*p->scale),
            .y=e->anchor.y+fixed(rim[i].y*p->scale),.rgb=edge};
    for (unsigned i = 0; i < 12; i += 2) {
        BfmForestDustCard *c = additive(e,bucket);
        c->vertices[0] = vertices[i];
        c->vertices[1] = vertices[i+1];
        c->vertices[2] = (BfmForestRasterVertex){.x=e->anchor.x,.y=e->anchor.y,.rgb=color};
        c->vertices[3] = vertices[(i+2)%12];
    }
}

static void streak(BfmWoodsEarthAwardEffect *e, EarthChild *p, BfmGameplayRun *run,
                   BfmEarthAwardEffectFrame *out)
{
    if (p->age == 0) {
        p->rotation = rotation(p->angle + (bfm_gameplay_random(run) & 511));
        p->delay = bfm_gameplay_random(run) & 7;
        p->radius[0] = p->radius[1] = 512;
        return;
    }
    if (p->age == 1) {
        if (p->delay > 1) { --p->delay; return; }
        p->delay = 8;
        p->age = 2;
        p->cue_timer = 32;
        return;
    }
    if (p->cue_timer && --p->cue_timer == 0) sound(e,out,STREAK_SOUND);
    if (p->delay) --p->delay;
    for (unsigned end = 0; end < (p->delay ? 1u : 2u); ++end) {
        p->velocity[end] += p->acceleration;
        p->radius[end] -= p->velocity[end] / 65536;
        if (p->radius[end] < 0) p->radius[end] = 0;
    }
    for (unsigned side = 0; side < 2; ++side) {
        BfmForestDustCard *c = additive(e,e->bucket);
        for (unsigned i = 0; i < 4; ++i) {
            Point local = {i < 2 ? 0 : (side ? 2 : -2),p->radius[i&1]};
            c->vertices[i] = vertex(e->anchor,p->rotation,local,e->streak_rgb[i&1]);
        }
    }
    if (!p->radius[0] && !p->radius[1]) p->kind = EMPTY;
}

static void glow(BfmWoodsEarthAwardEffect *e, EarthChild *p, BfmEarthAwardEffectFrame *out)
{
    if (p->age == 0) {
        p->light = 255;
    } else if (p->age <= 256) {
        unsigned remaining = (257 - p->age) & 63;
        int base = 128 + (p->age > 193 ? 32*(int)(p->age-193) : 0);
        p->scale = base + sine(remaining*64) / 128;
        if (p->age == 192) sound(e,out,GLOW_SOUND);
    } else if (!p->rim) {
        p->scale += 512;
        if (p->scale > UNIT) {
            p->scale = UNIT;
            p->rim = 255;
        }
    } else {
        p->light = p->rim = fade(p->light);
        if (!p->light) { p->kind = EMPTY; return; }
    }
    disc_cards(e,p);
}

static void advance_children(BfmWoodsEarthAwardEffect *e, BfmGameplayRun *run,
                             BfmEarthAwardEffectFrame *out)
{
    for (unsigned i = 0; i < CHILDREN; ++i) {
        EarthChild *p = &e->child[i];
        switch (p->kind) {
        case EMPTY: continue;
        case RAY:
            if (p->age) ray(e,p->rotation,run);
            p->age = 1;
            continue;
        case STREAK:
            streak(e,p,run,out);
            if (!p->age) p->age = 1;
            continue;
        case GLOW: glow(e,p,out); break;
        case FLASH:
            if (p->age == 16) { p->kind = EMPTY; continue; }
            if (p->age) {
                p->speed += p->acceleration;
                p->scale += p->speed;
                p->light = fade(p->light);
            }
            disc_cards(e,p);
            break;
        }
        ++p->age;
    }
}

static void emit_wave(BfmWoodsEarthAwardEffect *e)
{
    flash(e,0);
    for (unsigned i = 0; i < 8; ++i) {
        EarthChild *p = spawn_child(e,STREAK);
        if (!p) break;
        p->angle = i * 512;
        p->acceleration = 0x6000 + (int)e->counter * 128;
    }
}

static void start_rays(BfmWoodsEarthAwardEffect *e, BfmEarthAwardEffectFrame *out)
{
    for (unsigned i = 0; i < CHILDREN; ++i)
        if (e->child[i].kind == STREAK) e->child[i].kind = EMPTY;
    for (unsigned i = 0; i < RAYS; ++i) {
        EarthChild *p = spawn_child(e,RAY);
        if (p) p->rotation = rotation(i*455);
    }
    e->counter = 32;
    e->phase = RAY_DELAY;
    sound(e,out,RAY_START);
}

int bfm_woods_earth_award_effect_step(BfmWoodsEarthAwardEffect *e, BfmGameplayRun *run,
    int spawn, int finish, const BfmForestTransform *hero, const BfmBedroomView *view,
    uint16_t tiles[512][1024], BfmEarthAwardEffectFrame *out)
{
    if (!e || !run || !out || (spawn && (e->phase != IDLE || !hero || !view || !tiles)) ||
        (finish && e->phase != MESSAGE)) return 0;
    *out = (BfmEarthAwardEffectFrame){0};
    e->count = 0;
    if (finish) {
        e->phase = RETIRE;
        for (unsigned i = 0; i < CHILDREN; ++i)
            if (e->child[i].kind == RAY) e->child[i].kind = EMPTY;
    }
    if (spawn) {
        place(e,hero,view,tiles);
        e->phase = GROW;
        e->scale = e->speed = e->counter = e->angle = 0;
        e->visible = 1;
        flash(e,1);
        sound(e,out,APPEAR);
    } else {
        switch (e->phase) {
        case IDLE: break;
        case GROW:
            e->visible = !(e->counter++ & 1);
            e->speed += 0x6c000;
            e->scale += e->speed / 65536;
            if (e->scale > UNIT) {
                e->scale = UNIT;
                e->phase = GATHER;
                e->counter = 0;
                spawn_child(e,GLOW);
            }
            break;
        case GATHER:
            if (e->counter == 256) { start_rays(e,out); break; }
            if (!(e->counter & 15)) emit_wave(e);
            e->visible = !(e->counter++ & 1);
            break;
        case RAY_DELAY:
            e->visible = !(e->counter & 1);
            if (!--e->counter) {
                e->phase = MESSAGE;
                out->message_ready = 1;
            }
            break;
        case MESSAGE:
            e->visible = !(e->counter++ & 1);
            break;
        case RETIRE:
            e->visible = !(e->counter++ & 1);
            e->angle = (e->angle + 128) & 4095;
            e->scale -= 64;
            if (e->scale < 0) {
                e->phase = IDLE;
                flash(e,1);
                sound(e,out,VANISH);
            }
            break;
        }
    }
    if (e->phase != IDLE) medal_card(e);
    advance_children(e,run,out);
    return 1;
}

int bfm_woods_earth_award_effect_add(const BfmWoodsEarthAwardEffect *e,
    BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    if (!e || !figure || !view) return 0;
    if (!e->projected) return 1;
    for (unsigned i = 0; i < e->count; i += BFM_TETHER_CARDS) {
        unsigned count = e->count-i;
        if (count > BFM_TETHER_CARDS) count = BFM_TETHER_CARDS;
        if (!bfm_bedroom_figure_add_fusion(figure,view,NULL,e->cards+i,count)) return 0;
    }
    return 1;
}
