/* SPDX-License-Identifier: MIT */
#include "forest_shore_figures.h"
#include "forest_shore_pose.h"
#include "forest_guard_contact.h"
#include "forest_shore_behavior.h"
#include "forest_shore_response.h"
#include "forest_shore_throw.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>

enum { BODY_PARTS = 24, BODY_BOXES = 2 };

typedef struct {
    int16_t placement[3];
    BfmForestTransform root, base_root, attachment_root;
    BfmForestPose pose;
    BfmForestShoreBehavior behavior;
    BfmForestShoreResponse response;
    int32_t q16[3];
    BfmForestShoreThrow flight;
    enum { SHORE_UNHELD, SHORE_GRABBING, SHORE_HELD, SHORE_RELEASING, SHORE_FLYING, SHORE_FADING } carry;
    unsigned held_ticks, fade_ticks;
    uint32_t clip;
    unsigned key;
    int16_t pending_power, pending_yaw;
    int16_t at[3];
} ShoreFigure;

struct BfmForestShoreFigures {
    const BfmForestField *field;
    MusashiForestPlayerMesh *body, *base;
    BfmForestPose base_pose;
    BfmForestGuardBox boxes[BODY_BOXES];
    uint16_t initial_health, initial_flags;
    int16_t grip_offset[3], base_offset[3];
    ShoreFigure placed[BFM_FOREST_SHORE_FIGURES];
};

static void identity(BfmForestTransform *transform)
{
    for (unsigned axis = 0; axis < 3; ++axis)
        transform->rotation[axis][axis] = 4096;
}

static MusashiForestPlayerMesh *load_shape(const BfmForestField *field,
                                           uint32_t slot, unsigned parts)
{
    const uint8_t *reference = bfm_forest_room_bytes(field, slot, 4);
    if (!reference) return NULL;
    uint32_t table = u32le(reference);
    size_t table_bytes = 4 * (parts + 1);
    const uint8_t *list = bfm_forest_room_bytes(field, table, table_bytes);
    if (!list || u32le(list + 4 * parts)) return NULL;
    uint32_t begin = u32le(list);
    if (begin >= table || table - begin > (1u << 20)) return NULL;
    size_t span = (size_t)(table - begin) + table_bytes;
    const uint8_t *region = bfm_forest_room_bytes(field, begin, span);
    unsigned count = 0;
    MusashiForestPlayerMesh *mesh =
        musashi_forest_player_mesh_from_table(region, span, begin, table, &count);
    if (count == parts) return mesh;
    musashi_forest_player_mesh_free(mesh);
    return NULL;
}

static int load_boxes(const BfmForestField *field, uint32_t address, BfmForestGuardBox *boxes)
{
    for (unsigned i = 0; i < BODY_BOXES; ++i) {
        const uint8_t *record = bfm_forest_room_bytes(field, address, 16);
        if (!record) return 0;
        for (unsigned axis = 0; axis < 3; ++axis) {
            boxes[i].low[axis] = s16le(record + 4 + 4 * axis);
            boxes[i].high[axis] = s16le(record + 6 + 4 * axis);
            if (boxes[i].low[axis] > boxes[i].high[axis]) return 0;
        }
        address = u32le(record);
    }
    return address == 0;
}

static int load_placements(BfmForestShoreFigures *figures)
{
    const BfmForestField *field = figures->field;
    const uint8_t *room = bfm_forest_room_bytes(field, UINT32_C(0x80186ad0), 4);
    const uint8_t *desc = room ? bfm_forest_room_bytes(field, u32le(room), 0xbc) : NULL;
    if (!desc) return 0;
    uint32_t list = u32le(desc + 0xb8);
    unsigned count = 0;
    for (unsigned i = 0; i < 64; ++i) {
        const uint8_t *record = bfm_forest_room_bytes(field, list + i * 20, 20);
        if (!record) return 0;
        unsigned kind = u16le(record + 6);
        if (!kind) return count == BFM_FOREST_SHORE_FIGURES;
        if (kind != 12 || u16le(record + 8)) continue;
        if (count >= BFM_FOREST_SHORE_FIGURES) return 0;
        ShoreFigure *figure = &figures->placed[count++];
        for (unsigned axis = 0; axis < 3; ++axis)
            figure->placement[axis] = s16le(record + 2 * axis);
        bfm_forest_actor_root(u16le(record + 12), figure->placement, &figure->base_root);
        figure->root = figure->base_root;
    }
    return 0;
}

BfmForestShoreFigures *bfm_forest_shore_figures_open(const BfmForestField *field)
{
    const uint8_t *descriptor = bfm_forest_room_bytes(field, UINT32_C(0x8018aeb8), 20);
    const uint8_t *base_offset = bfm_forest_room_bytes(field, UINT32_C(0x8018afb0), 8);
    if (!descriptor || !base_offset) return NULL;
    BfmForestShoreFigures *figures = calloc(1, sizeof(*figures));
    if (!figures) return NULL;
    figures->field = field;
    for (unsigned axis = 0; axis < 3; ++axis) {
        figures->grip_offset[axis] = s16le(descriptor + 12 + 2 * axis);
        figures->base_offset[axis] = s16le(base_offset + 2 * axis);
    }
    figures->initial_health = u16le(descriptor);
    figures->initial_flags = u16le(descriptor + 2);
    if (!load_placements(figures) || !load_boxes(field, u32le(descriptor + 8), figures->boxes))
        goto failed;
    figures->body = load_shape(field, UINT32_C(0x8018aebc), BODY_PARTS);
    figures->base = load_shape(field, UINT32_C(0x8018afa8), 1);
    if (!figures->body || !figures->base) goto failed;
    identity(&figures->base_pose.local[0]);
    return figures;
failed:
    bfm_forest_shore_figures_free(figures);
    return NULL;
}

void bfm_forest_shore_figures_free(BfmForestShoreFigures *figures)
{
    if (!figures) return;
    musashi_forest_player_mesh_free(figures->body);
    musashi_forest_player_mesh_free(figures->base);
    free(figures);
}

static void start_figure(BfmForestShoreFigures *figures, ShoreFigure *f)
{
    bfm_forest_shore_behavior_begin(&f->behavior);
    bfm_forest_shore_response_init(&f->response, (int16_t)figures->initial_health,
                                   figures->initial_flags);
    for (unsigned axis = 0; axis < 3; ++axis) {
        f->at[axis] = f->placement[axis];
        f->q16[axis] = (int32_t)f->at[axis] * 65536;
    }
    f->carry = SHORE_UNHELD;
}

static void flinch_height(const BfmForestField *field, ShoreFigure *f)
{
    BfmForestContact floor;
    if (f->at[1] < INT16_MIN + 4 ||
        !bfm_forest_ground(field, f->at[0], (int16_t)(f->at[1] - 4), f->at[2], &floor) ||
        floor.flags || floor.normal[0] || floor.normal[2] || floor.normal[1] >= 0)
        return;
    f->q16[1] = bfm_forest_shore_response_height(&f->response, f->q16[1], floor.height);
    f->at[1] = (int16_t)(((int64_t)f->q16[1] - (uint16_t)f->q16[1]) / 65536);
}

int bfm_forest_shore_figure_probe(const BfmForestShoreFigures *figures, unsigned i,
                                 uint16_t flags, const int16_t start[3], const int16_t end[3],
                                 int16_t contact[3])
{
    if (!figures || i >= BFM_FOREST_SHORE_FIGURES) return 0;
    const ShoreFigure *f = &figures->placed[i];
    if (!f->behavior.state || !(f->response.flags & flags)) return 0;
    /* The stance and grab queries use the creature's primary sliding box. */
    return bfm_forest_body_slide(figures->boxes, &f->root, start, end, contact);
}

int bfm_forest_shore_figure_grip_offset(const BfmForestShoreFigures *figures, int16_t out[3])
{
    if (!figures || !out) return 0;
    for (unsigned axis = 0; axis < 3; ++axis) out[axis] = figures->grip_offset[axis];
    return 1;
}

int bfm_forest_shore_figure_grab(BfmForestShoreFigures *figures, unsigned i,
                                uint16_t power, int16_t player_yaw)
{
    if (!figures || i >= BFM_FOREST_SHORE_FIGURES || power > INT16_MAX) return 0;
    ShoreFigure *f = &figures->placed[i];
    if (!f->behavior.state || !(f->response.flags & 0x200) || f->carry != SHORE_UNHELD)
        return 0;
    f->carry = SHORE_GRABBING;
    f->pending_power = (int16_t)power;
    f->pending_yaw = player_yaw;
    f->response.type = 10;
    f->response.flags |= 1;
    return 1;
}

int bfm_forest_shore_figure_hold(BfmForestShoreFigures *figures, unsigned i,
                                const int16_t world[3], int16_t player_yaw)
{
    if (!figures || i >= BFM_FOREST_SHORE_FIGURES || !world) return 0;
    ShoreFigure *f = &figures->placed[i];
    if (f->carry != SHORE_HELD) return 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        f->at[axis] = world[axis];
        f->q16[axis] = (int32_t)world[axis] * 65536 + (uint16_t)f->q16[axis];
    }
    f->behavior.yaw = (int16_t)(((unsigned)player_yaw + 2048) & 4095);
    return 1;
}

static int release_figure(BfmForestShoreFigures *figures, unsigned i, uint16_t power,
                          int16_t player_yaw, uint16_t kind)
{
    if (!figures || i >= BFM_FOREST_SHORE_FIGURES || power > INT16_MAX) return 0;
    ShoreFigure *f = &figures->placed[i];
    if (f->carry != SHORE_HELD ||
        !bfm_forest_shore_release_begin(&f->flight, figures->field, f->q16, player_yaw, kind))
        return 0;
    f->carry = SHORE_RELEASING;
    f->pending_power = (int16_t)power;
    f->response.type = kind;
    f->response.flags |= 1;
    return 1;
}

int bfm_forest_shore_figure_throw(BfmForestShoreFigures *figures, unsigned i,
                                 uint16_t power, int16_t player_yaw)
{
    return release_figure(figures, i, power, player_yaw, 9);
}

int bfm_forest_shore_figure_drop(BfmForestShoreFigures *figures, unsigned i,
                                uint16_t power, int16_t player_yaw)
{
    return release_figure(figures, i, power, player_yaw, 17);
}

static void take_carry_power(ShoreFigure *f)
{
    f->response.flags &= (uint16_t)~1u;
    f->response.health -= f->pending_power;
    if (f->response.health <= 0) f->response.flags = 0;
}

static int carry_tick(BfmForestShoreFigures *figures, ShoreFigure *f)
{
    if (f->carry == SHORE_FADING) {
        if (--f->fade_ticks) return 1;
        f->behavior.state = 0;
        f->carry = SHORE_UNHELD;
        return 0;
    }
    if (f->carry == SHORE_GRABBING) {
        take_carry_power(f);
        f->behavior.yaw = (int16_t)(((unsigned)f->pending_yaw + 2048) & 4095);
        f->held_ticks = 0;
        f->carry = SHORE_HELD;
    }
    if (f->carry == SHORE_HELD) {
        /* Preserve the pickup pose for fourteen updates. The player drives
         * the creature's world position independently during this interval. */
        if (f->held_ticks < 15) {
            if (++f->held_ticks == 15) {
                f->clip = UINT32_C(0x80197514);
                f->key = 0;
            }
        } else {
            f->key = (f->key + 1) % 15;
        }
        return 1;
    }
    if (f->carry == SHORE_RELEASING) {
        take_carry_power(f);
        f->carry = SHORE_FLYING;
    }
    BfmForestShoreThrowStep step = bfm_forest_shore_throw_tick(&f->flight, figures->field);
    if (step == BFM_SHORE_THROW_UNRESOLVED) return 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        int32_t q = f->flight.position[axis];
        f->q16[axis] = q;
        f->at[axis] = (int16_t)(((int64_t)q - (uint16_t)q) / 65536);
    }
    f->behavior.yaw = f->flight.yaw;
    if (step == BFM_SHORE_THROW_RECOVER && f->response.health <= 0) {
        f->clip = f->flight.clip;
        f->key = f->flight.landed_key;
        f->carry = SHORE_FADING;
        f->fade_ticks = 30;
    } else if (step == BFM_SHORE_THROW_RECOVER) {
        f->response.phase = BFM_SHORE_RESPONSE_NORMAL;
        f->response.type = 0;
        f->response.flags = f->response.ordinary_flags;
        f->carry = SHORE_UNHELD;
        bfm_forest_shore_behavior_ready(&f->behavior);
        f->clip = bfm_forest_shore_behavior_clip(&f->behavior);
        f->key = f->behavior.tick;
    } else {
        f->clip = f->flight.clip;
        f->key = f->flight.key;
    }
    return 1;
}

void bfm_forest_shore_figures_tick_aim(BfmForestShoreFigures *figures, const int16_t player[3],
                                      const int16_t aim[3], int targetable)
{
    if (!figures || !player || !aim) return;
    for (unsigned i = 0; i < BFM_FOREST_SHORE_FIGURES; ++i) {
        ShoreFigure *f = &figures->placed[i];
        uint32_t clip = 0;
        unsigned key = 0;
        int release = f->carry == SHORE_RELEASING;
        if (!f->behavior.state) {
            int dz = player[2] - f->placement[2];
            int distance = abs(dz);
            if (distance <= 704 || distance >= 768) continue;
            start_figure(figures, f);
        } else if (f->carry != SHORE_UNHELD) {
            if (!carry_tick(figures, f)) continue;
            clip = f->clip;
            key = f->key;
        } else {
            BfmForestShoreResponseStep step = bfm_forest_shore_response_tick(&f->response);
            if (step == BFM_SHORE_RESPONSE_REMOVE) {
                f->behavior.state = 0;
                continue;
            }
            if (step == BFM_SHORE_RESPONSE_NORMAL) {
                unsigned event = bfm_forest_shore_behavior_tick(&f->behavior, f->at, player, aim, targetable);
                if (event & BFM_SHORE_FIGURE_RETIRE) {
                    f->behavior.state = 0;
                    continue;
                }
            } else if (step == BFM_SHORE_RESPONSE_RECOVER) {
                bfm_forest_shore_behavior_ready(&f->behavior);
            } else {
                f->behavior.yaw = (int16_t)f->response.yaw;
                if (step == BFM_SHORE_RESPONSE_FLINCH) flinch_height(figures->field, f);
                clip = step == BFM_SHORE_RESPONSE_FLINCH ? UINT32_C(0x80196fe4) : UINT32_C(0x8019746c);
                key = f->response.key;
            }
        }
        if (!clip) {
            clip = bfm_forest_shore_behavior_clip(&f->behavior);
            key = f->behavior.tick;
        }
        if (bfm_forest_shore_clip_pose(figures->field, clip, key, &f->pose)) {
            f->clip = clip;
            f->key = key;
            /* The base sees the last submitted parent matrix. On release the
             * grab pose stays submitted through the first flight update. */
            const BfmForestTransform *previous = &f->attachment_root;
            unsigned pitch = f->carry == SHORE_FLYING ? (unsigned)f->flight.pitch : 0;
            bfm_forest_actor_pitch_yaw(pitch, (unsigned)f->behavior.yaw, f->at, &f->root);
            if (clip == UINT32_C(0x80197514)) {
                f->base_root = f->root;
                for (unsigned row = 0; row < 3; ++row) {
                    int64_t sum = 0;
                    for (unsigned axis = 0; axis < 3; ++axis)
                        sum += (int64_t)previous->rotation[row][axis] * figures->base_offset[axis];
                    f->base_root.translation[row] = previous->translation[row] +
                        (int32_t)(sum / 4096 - (sum % 4096 < 0));
                }
            } else {
                bfm_forest_actor_pitch_yaw(pitch, 0, f->at, &f->base_root);
            }
            if (!release) f->attachment_root = f->root;
        }
    }
}

void bfm_forest_shore_figures_tick(BfmForestShoreFigures *figures, const int16_t player[3])
{
    bfm_forest_shore_figures_tick_aim(figures, player, player, 1);
}

int bfm_forest_shore_figure_state(const BfmForestShoreFigures *figures, unsigned placement,
                                  BfmForestShoreFigureState *out)
{
    if (!figures || placement >= BFM_FOREST_SHORE_FIGURES || !out) return 0;
    const ShoreFigure *f = &figures->placed[placement];
    if (!f->behavior.state) return 0;
    *out = (BfmForestShoreFigureState){
        .position = {f->at[0], f->at[1], f->at[2]}, .yaw = f->behavior.yaw,
        .behavior = f->behavior.state, .tick = f->behavior.tick,
        .health = f->response.health, .flags = f->response.flags, .tag = f->response.type,
        .position_q16 = {f->q16[0], f->q16[1], f->q16[2]}
    };
    return 1;
}

int bfm_forest_shore_figure_contact(BfmForestShoreFigures *figures, unsigned placement,
                                   const int16_t start[3], const int16_t end[3],
                                   uint16_t kind, uint16_t strength, int16_t attacker_yaw,
                                   int16_t center[3], int16_t point[3], int16_t normal[3])
{
    if (!figures || placement >= BFM_FOREST_SHORE_FIGURES || !start || !end || !center)
        return 0;
    ShoreFigure *f = &figures->placed[placement];
    if (!f->behavior.state || (f->response.flags & 0x8100) != 0x8000 ||
        kind < 1 || kind > 3 || f->response.type == kind || f->response.type == 10)
        return 0;
    /* The mounted descriptor is tagged for the scaled primary body. The
     * strike selector transforms that box into a single temporary shape. */
    int16_t middle[3], surface[3], direction[3];
    if (!bfm_forest_guard_contact_point(&figures->boxes[0], &f->root, start, end,
                                        middle, surface, direction))
        return 0;
    int blocked = !!(f->response.flags & 0x4000);
    if (!blocked && !bfm_forest_shore_response_queue(&f->response, kind, strength, attacker_yaw))
        return 0;
    if (!blocked) f->carry = SHORE_UNHELD;
    for (unsigned axis = 0; axis < 3; ++axis) {
        center[axis] = middle[axis];
        if (point) point[axis] = surface[axis];
        if (normal) normal[axis] = direction[axis];
    }
    return blocked ? 4 : 1;
}

size_t bfm_forest_shore_figures_actors(const BfmForestShoreFigures *figures,
                                      BfmForestActor *out, size_t capacity)
{
    if (!figures || !out) return 0;
    size_t count = 0;
    for (unsigned i = 0; i < BFM_FOREST_SHORE_FIGURES; ++i)
        if (figures->placed[i].behavior.state) count += 2;
    if (capacity < count) return 0;
    size_t index = 0;
    for (unsigned i = 0; i < BFM_FOREST_SHORE_FIGURES; ++i) {
        const ShoreFigure *f = &figures->placed[i];
        if (!f->behavior.state) continue;
        int hidden = f->carry == SHORE_FADING && (f->fade_ticks & 1);
        uint32_t parts = hidden ? 0 : (UINT32_C(1) << BODY_PARTS) - 1;
        out[index++] = (BfmForestActor){figures->body, &f->pose, &f->root, parts, NULL};
        out[index++] = (BfmForestActor){figures->base, &figures->base_pose, &f->base_root, !hidden, NULL};
    }
    return count;
}
