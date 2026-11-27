/* SPDX-License-Identifier: MIT */
#include "woods_king.h"
#include "woods_king_art.h"
#include "forest_guard_contact.h"
#include "forest_lift.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>
#include <string.h>

enum { KING = 149, REMOVED_FLAG = 157, BITE_RAYS = 6 };
enum { CHARGED_STRIKE = 11, SHRINK_SPELL = 30, STOMP = 31 };
enum Reaction { NONE, SHRINK, CRUSH, STRUCK };
enum Mode { START_IDLE, IDLE, START_BITE, BITE, HURT };
struct BfmWoodsKing {
    BfmWoodsKingArt *art;
    BfmBedroomRoot root;
    BfmForestGuardBox player_box, body;
    BfmForestTransform world;
    int16_t local_bite[BITE_RAYS][2][3];
    enum Mode mode;
    BfmWoodsKingPose pose;
    unsigned age, next_sample, remaining, shrink_ticks, flat_ticks, width, height;
    unsigned completed_width, completed_height, last_event, body_points, hit_sound;
    uint16_t contact_flags;
    enum Reaction pending;
    int retired, attempted;
};

static const uint32_t base = UINT32_C(0x80128158), room = UINT32_C(0x8018ea38);

static const uint8_t *mounted(const uint8_t *scene, size_t size, uint32_t address, size_t n)
{
    if (address < base || address-base > size || n > size-(address-base)) return NULL;
    return scene+(address-base);
}

static const uint8_t *placement(const uint8_t *scene, size_t size)
{
    const uint8_t *header = mounted(scene, size, room, 0xbc);
    if (!header) return NULL;
    uint32_t first = u32le(header+0xb4), end = u32le(header+0xb8);
    if (end < first || (end-first)%20) return NULL;
    const uint8_t *rows = mounted(scene, size, first, end-first);
    if (!rows) return NULL;
    const uint8_t *result = NULL;
    for (size_t at = 0; at < end-first; at += 20) {
        unsigned type = u16le(rows+at+6);
        if (!type) return result;
        if (type != KING) continue;
        if (result || u16le(rows+at+8)) return NULL;
        result = rows+at;
    }
    return NULL;
}

static int load_bite(BfmWoodsKing *king, const uint8_t *scene, size_t bytes)
{
    const uint8_t *rays = mounted(scene, bytes, UINT32_C(0x8018b774), 16*BITE_RAYS);
    const uint8_t *shape = mounted(scene, bytes, UINT32_C(0x801891c0), 16);
    const uint8_t *profile = mounted(scene,bytes,UINT32_C(0x8018b720),48);
    const uint8_t *body = profile ? mounted(scene,bytes,u32le(profile+8),16) : NULL;
    if (!rays || !shape || u32le(shape) || !body || u32le(body) || s16le(profile+46) < 0) return 0;
    king->body_points = (unsigned)s16le(profile+46);
    king->hit_sound = u16le(profile+20);
    king->contact_flags = u16le(profile+2);
    bfm_forest_actor_root(king->root.yaw, king->root.xyz, &king->world);
    for (unsigned k = 0; k < 3; ++k) {
        king->player_box.low[k] = s16le(shape+4+4*k);
        king->player_box.high[k] = s16le(shape+6+4*k);
        king->body.low[k] = s16le(body+4+4*k);
        king->body.high[k] = s16le(body+6+4*k);
        if (king->player_box.low[k] > king->player_box.high[k] ||
            king->body.low[k] > king->body.high[k]) return 0;
    }
    for (unsigned i = 0; i < BITE_RAYS; ++i)
        for (unsigned j = 0; j < 2; ++j)
            for (unsigned k = 0; k < 3; ++k)
                king->local_bite[i][j][k] = s16le(rays+16*i+8*j+2*k);
    return 1;
}

BfmWoodsKing *bfm_woods_king_open(const uint8_t *scene, size_t bytes, const uint8_t flags[64])
{
    if (!scene || !flags) return NULL;
    BfmWoodsKing *king = calloc(1, sizeof *king);
    if (!king) return NULL;
    if (flags[REMOVED_FLAG/8] & (1u << (REMOVED_FLAG%8))) {
        king->retired = 1;
        return king;
    }
    const uint8_t *row = placement(scene, bytes);
    if (!row) goto fail;
    for (unsigned k = 0; k < 3; ++k) king->root.xyz[k] = s16le(row+2*k);
    king->root.yaw = u16le(row+12) & 4095u;
    if (!load_bite(king,scene,bytes) ||
        !bfm_bedroom_travel_air_shadow(scene,bytes,room,&king->root)) goto fail;
    king->art = bfm_woods_king_art_open(scene,bytes);
    if (!king->art) goto fail;
    king->pose = BFM_KING_IDLE;
    king->completed_width = king->completed_height = king->width = king->height = 4096;
    return king;
fail:
    bfm_woods_king_free(king);
    return NULL;
}

void bfm_woods_king_free(BfmWoodsKing *king)
{
    if (!king) return;
    bfm_woods_king_art_free(king->art);
    free(king);
}

int bfm_woods_king_add(const BfmWoodsKing *king, BfmBedroomFigure *figure,
                       const BfmBedroomView *view)
{
    if (!king || !figure || !view) return 0;
    if (king->retired) return 1;
    return bfm_woods_king_art_add(king->art,figure,view,&king->root,king->pose,king->age,
                                  king->width,king->height);
}

static int64_t distance2(const int16_t from[3], const int16_t to[3])
{
    int64_t x = from[0]-to[0], y = from[1]-to[1], z = from[2]-to[2];
    return x*x+y*y+z*z;
}

static int scaled(int value, int q12)
{
    int64_t n = (int64_t)value*q12;
    return (int)(n/4096 - (n%4096 < 0));
}

static void completed_basis(const BfmWoodsKing *king, BfmForestTransform *basis)
{
    *basis = king->world;
    for (unsigned col = 0; col < 3; ++col) {
        unsigned size = col == 1 ? king->completed_height : king->completed_width;
        for (unsigned row = 0; row < 3; ++row)
            basis->rotation[row][col] = (int16_t)scaled(king->world.rotation[row][col], (int)size);
    }
}

/* The original body tag rescales the box about its root. Motion probes
 * use the completed model basis, removing its scale with the current
 * reciprocal. Keep the rounding of both matrix operations at size changes. */
static void body_shape(const BfmWoodsKing *king, BfmForestGuardBox *box, BfmForestTransform *basis)
{
    completed_basis(king, basis);
    for (unsigned col = 0; col < 3; ++col) {
        unsigned current = col == 1 ? king->height : king->width;
        int reciprocal = (1 << 24)/(int)current;
        box->low[col] = (int16_t)scaled(king->body.low[col], (int)current);
        box->high[col] = (int16_t)scaled(king->body.high[col], (int)current);
        for (unsigned row = 0; row < 3; ++row)
            basis->rotation[row][col] = (int16_t)scaled(basis->rotation[row][col], reciprocal);
    }
}

int bfm_woods_king_slide(const BfmWoodsKing *king, const int16_t previous[3],
                         int16_t endpoint[3], unsigned field_tick)
{
    if (!king || !previous || !endpoint || king->flat_ticks || king->retired ||
        previous[1] < INT16_MIN+16 || endpoint[1] < INT16_MIN+16) return 0;
    BfmForestGuardBox box;
    BfmForestTransform basis;
    body_shape(king,&box,&basis);
    const int16_t start[3] = {previous[0],previous[1]-16,previous[2]};
    const int16_t end[3] = {endpoint[0],endpoint[1]-16,endpoint[2]};
    int16_t center[3], normal[3], hit[3];
    if (!bfm_forest_guard_contact_point(&box,&basis,start,end,center,NULL,normal) ||
        !bfm_forest_body_slide(&box,&basis,start,end,hit)) return 0;

    int inside = bfm_forest_guard_contact(&box,&basis,start,start,center);
    int x = hit[0], y = inside ? endpoint[1] : hit[1]+16, z = hit[2];
    if (inside || abs(normal[1]) > 2048) {
        int phase = (field_tick >> 5) & 3;
        x += (x > center[0] ? 1 : -1) * (phase >= 2 ? 8 : 7);
        z += (z > center[2] ? 1 : -1) * (phase == 2 ? 7 : 8);
    }
    if (x < INT16_MIN || x > INT16_MAX || y < INT16_MIN || y > INT16_MAX ||
        z < INT16_MIN || z > INT16_MAX) return 0;
    endpoint[0] = (int16_t)x;
    endpoint[1] = (int16_t)y;
    endpoint[2] = (int16_t)z;
    return 1;
}

static int blade_contact(BfmWoodsKing *king, unsigned cause, const int16_t from[3],
                         const int16_t to[3], int16_t hit[3], int16_t normal[3])
{
    if (!king || !from || !to || king->retired || king->flat_ticks || king->last_event == cause)
        return 0;
    BfmForestGuardBox box;
    BfmForestTransform basis;
    int16_t center[3];
    body_shape(king,&box,&basis);
    if (!bfm_forest_guard_contact_point(&box,&basis,from,to,center,hit,normal)) return 0;
    king->pending = STRUCK;
    king->last_event = cause;
    return 1;
}

int bfm_woods_king_strike(BfmWoodsKing *king, unsigned cause,
                          const int16_t from[3], const int16_t to[3])
{
    return king && king->pending == NONE && cause >= 1 && cause <= 8 &&
           blade_contact(king,cause,from,to,NULL,NULL);
}

int bfm_woods_king_charged_blade(BfmWoodsKing *king, const int16_t from[3],
                                 const int16_t to[3], int16_t hit[3], int16_t normal[3],
                                 unsigned *sound)
{
    if (!king || (king->contact_flags & 0xc100) != 0x8000 ||
        !blade_contact(king,CHARGED_STRIKE,from,to,hit,normal)) return 0;
    if (sound) *sound = king->hit_sound;
    return 1;
}

int bfm_woods_king_charged_wave(BfmWoodsKing *king, const int16_t spine[3], int16_t hit[3])
{
    if (!king || !spine || (king->contact_flags & 0xc100) != 0x8000 ||
        king->retired || king->flat_ticks || king->last_event == CHARGED_STRIKE ||
        distance2(king->root.xyz,spine) >= 80*80) return 0;
    if (hit) memcpy(hit,king->root.xyz,sizeof king->root.xyz);
    king->pending = STRUCK;
    king->last_event = CHARGED_STRIKE;
    return 1;
}

static int bite_connects(const BfmWoodsKing *king, const int16_t player[3])
{
    BfmForestTransform body, jaw;
    bfm_forest_actor_root(0, player, &body);
    /* A bite can become eligible on the field Shrink expires, before the
     * King's visible root has completed its return to full size. */
    completed_basis(king, &jaw);
    for (unsigned i = 0; i < BITE_RAYS; ++i) {
        int16_t start[3], end[3], center[3];
        bfm_forest_lift_world(&jaw, king->local_bite[i][0], start);
        bfm_forest_lift_world(&jaw, king->local_bite[i][1], end);
        if (bfm_forest_guard_contact(&king->player_box, &body, start, end, center)) return 1;
    }
    return 0;
}

static void scale_shrunk(BfmWoodsKing *king)
{
    unsigned ticks = --king->shrink_ticks;
    unsigned scale = 4096;
    if (ticks > 240) scale -= 144*(256-ticks);
    else if (ticks == 240) scale = 1792;
    else if (ticks >= 16) scale = 896;
    else scale -= 144*ticks;
    king->width = king->height = scale;
}

static void set_pose(BfmWoodsKing *king, enum Mode mode, BfmWoodsKingPose pose, unsigned duration)
{
    king->mode = mode;
    king->pose = pose;
    king->age = king->next_sample = 0;
    king->remaining = duration;
    king->attempted = 0;
}

static void advance_pose(BfmWoodsKing *king, const int16_t player[3], BfmWoodsKingEvent *event)
{
    int64_t range = distance2(king->root.xyz, player);
    switch (king->mode) {
    case START_IDLE:
        set_pose(king, IDLE, BFM_KING_IDLE, 0);
        break;
    case IDLE:
        if (range <= 69632) king->mode = START_BITE;
        else king->age = king->next_sample++;
        break;
    case START_BITE:
        set_pose(king, BITE, BFM_KING_BITE, 30);
        break;
    case BITE:
        if (king->remaining == 12) event->sound = 2771;
        if (king->remaining <= 10 && !king->attempted && !king->shrink_ticks) {
            king->attempted = 1;
            if (bite_connects(king,player)) {
                event->bite_power = 33;
                event->bite_yaw = king->root.yaw;
            }
        }
        if (!--king->remaining) {
            if (range > 28672) {
                king->mode = START_IDLE;
                break;
            }
            king->remaining = 30;
            king->attempted = 0;
        }
        king->age = king->next_sample++;
        break;
    case HURT:
        if (!--king->remaining) {
            king->mode = START_IDLE;
            king->last_event = 0;
        } else king->age = king->next_sample++;
        break;
    }
}

int bfm_woods_king_prepare(BfmWoodsKing *king, uint8_t flags[64], BfmWoodsKingEvent *event)
{
    if (!king || !flags || !event) return 0;
    *event = (BfmWoodsKingEvent){0};
    if (king->retired) return 1;
    king->completed_width = king->width;
    king->completed_height = king->height;
    if (king->shrink_ticks) scale_shrunk(king);
    if (king->pending == CRUSH) {
        king->shrink_ticks = 0;
        king->flat_ticks = 16;
        king->width = 1024;
        king->height = 512;
        event->sound = 1704;
    } else if (king->pending == SHRINK) {
        king->shrink_ticks = 256;
    } else if (king->pending == STRUCK) {
        set_pose(king,HURT,BFM_KING_HURT,32);
    }
    king->pending = NONE;
    if (king->flat_ticks) {
        if (king->width < 4096) king->width += 1024;
        if (!--king->flat_ticks) {
            king->retired = 1;
            flags[REMOVED_FLAG/8] |= (uint8_t)(1u << (REMOVED_FLAG%8));
            event->body_reward = 1;
            event->body_points = king->body_points;
        }
    }
    return 1;
}

void bfm_woods_king_target(BfmWoodsKing *king, const int16_t player[3], BfmWoodsKingEvent *event)
{
    if (king && player && event && !king->retired && !king->flat_ticks)
        advance_pose(king, player, event);
}

int bfm_woods_king_step(BfmWoodsKing *king, const int16_t player[3],
                        uint8_t flags[64], BfmWoodsKingEvent *event)
{
    if (!player || !bfm_woods_king_prepare(king, flags, event)) return 0;
    bfm_woods_king_target(king, player, event);
    return 1;
}

void bfm_woods_king_touch(BfmWoodsKing *king, const int16_t player[3])
{
    if (king && player && !king->retired && king->shrink_ticks &&
        distance2(king->root.xyz,player) < 48*48) {
        king->pending = CRUSH;
        king->last_event = STOMP;
    }
}

const int16_t *bfm_woods_king_shrink_root(const BfmWoodsKing *king)
{
    return king && !king->retired && !king->flat_ticks && !king->shrink_ticks &&
           king->pending == NONE ? king->root.xyz : NULL;
}

int bfm_woods_king_shrink(BfmWoodsKing *king)
{
    if (!bfm_woods_king_shrink_root(king)) return 0;
    king->pending = SHRINK;
    king->last_event = SHRINK_SPELL;
    return 1;
}
