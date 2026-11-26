/* SPDX-License-Identifier: MIT */
#include "tower_summit_stun.h"
#include "forest_absorption.h"
#include "forest_cast.h"
#include "forest_opening_data.h"
#include "forest_shore_motion.h"
#include "tower_stun_cast.h"
#include "../pc_port/forest_player_archive.h"
#include "../pc_port/forest_player_bytes.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

enum { STUN = 7, SECTOR = 2048 };
enum AwardPhase { NO_AWARD, CATCH, LOAD, READ, CLEAR, RELEASE };

struct BfmTowerSummitStun {
    const BfmForestField *room;
    BfmForestCast cast;
    BfmForestTether tether;
    BfmFusionFlight flight;
    BfmFusionModel *model;
    BfmTowerStunCast *emitter;
    BfmForestMessage *page;
    uint8_t message[256];
    size_t message_size;
    uint32_t message_address, duration;
    unsigned gain, cost, sectors, remaining, key;
    int casting;
    uint32_t struck[BFM_SUMMIT_RESIDENTS];
    enum AwardPhase award;
};

static int mount_catalogue(BfmTowerSummitStun *s, const MusashiDiscMedia *disc)
{
    const uint8_t *gain = bfm_forest_room_bytes(s->room, UINT32_C(0x8018d978), STUN + 1);
    const uint8_t *cost = bfm_forest_room_bytes(s->room, UINT32_C(0x8018d8dc), STUN + 1);
    const uint8_t *duration = bfm_forest_room_bytes(s->room, UINT32_C(0x8018d8fc) + 4 * STUN, 4);
    const uint8_t *messages = bfm_forest_room_bytes(s->room, UINT32_C(0x8018d5f4) + 4 * STUN, 8);
    ForestPlayerArchive player;
    uint8_t row[8], directory[SECTOR];
    size_t total;
    if (!gain || !gain[STUN] || !cost || !cost[STUN] || (cost[STUN] & 128) || !duration || !messages ||
        !forest_player_archive_open(disc, &player) ||
        !forest_player_archive_read(&player, UINT32_C(0x800d3764), row, sizeof row) ||
        u32le(row + 4) != 7 || !musashi_disc_media_read_root_range(disc, "MAIN.CD",
            0, directory, sizeof directory, &total)) return 0;

    uint32_t member = u32le(row), count = u32le(directory);
    if (member >= count || count > (SECTOR - 8) / 8) return 0;
    const uint8_t *entry = directory + 8 + 8 * member;
    uint32_t sector = u32le(entry), bytes = u32le(entry + 4);
    if (!sector || sector > total / SECTOR || !bytes || bytes % SECTOR ||
        bytes > total - (size_t)sector * SECTOR) return 0;
    s->sectors = bytes / SECTOR;
    s->gain = gain[STUN];
    s->cost = cost[STUN];
    s->duration = u32le(duration);
    s->message_address = u32le(messages);
    uint32_t end = u32le(messages + 4);
    if (end <= s->message_address || end - s->message_address > sizeof s->message) return 0;
    s->message_size = end - s->message_address;
    return forest_player_archive_read(&player, s->message_address, s->message, s->message_size);
}

BfmTowerSummitStun *bfm_tower_summit_stun_open(const MusashiDiscMedia *disc,
        const BfmForestField *room, uint32_t animation_table)
{
    if (!disc || !room) return NULL;
    BfmTowerSummitStun *s = calloc(1, sizeof *s);
    if (!s) return NULL;
    s->room = room;
    if (!mount_catalogue(s, disc) || !bfm_forest_cast_load(&s->cast, disc, animation_table) ||
        !(s->model = bfm_fusion_model_open(disc)) ||
        !(s->emitter = bfm_tower_stun_cast_open(disc, room))) {
        bfm_tower_summit_stun_free(s);
        return NULL;
    }
    return s;
}

void bfm_tower_summit_stun_free(BfmTowerSummitStun *s)
{
    if (!s) return;
    bfm_tower_stun_cast_free(s->emitter);
    bfm_forest_message_free(s->page);
    bfm_fusion_model_free(s->model);
    free(s);
}

static int64_t distance_squared(const int16_t a[3], const int16_t b[3])
{
    int64_t x = (int)a[0] - b[0], y = (int)a[1] - b[1], z = (int)a[2] - b[2];
    return x*x + y*y + z*z;
}

typedef struct {
    BfmTowerSummitResidents *pool;
    const BfmForestField *room;
    BfmForestTetherTarget target;
} SummitContact;

/* Register only a crossing of a live recipient's mounted body. Among several
 * candidates on a single step, the nearer intersection takes precedence. */
static int contact_blade(void *context, const int16_t from[3], const int16_t to[3])
{
    SummitContact *q = context;
    int16_t point[3], center[3];
    int wall = bfm_forest_resolve_solid_segment(q->room, from, to, point) != 0;
    int64_t best = wall ? distance_squared(from, point) : INT64_MAX;
    int result = wall ? -1 : 0;
    for (unsigned i = 0; i < BFM_SUMMIT_RESIDENTS; ++i) {
        BfmTowerSummitResident actor;
        if (!bfm_tower_summit_residents_get(q->pool, i, &actor) || !actor.health ||
            actor.reward != STUN || actor.state == 4 ||
            !bfm_tower_summit_residents_contact(q->pool, i, from, to, center, point)) continue;
        int64_t distance = distance_squared(from, point);
        if (distance < best) {
            best = distance;
            q->target = (BfmForestTetherTarget){i, actor.generation, 47, actor.variant};
            result = 1;
        }
    }
    return result;
}

static int aim_blade(const BfmFusionFlight *f, const BfmTowerSummitResidents *pool, int16_t aim[3])
{
    int16_t from[3];
    for (unsigned i = 0; i < 3; ++i) from[i] = (int16_t)f->model.translation[i];
    unsigned best = 512;
    for (unsigned i = 0; i < BFM_SUMMIT_RESIDENTS; ++i) {
        BfmTowerSummitResident actor;
        if (!bfm_tower_summit_residents_get(pool, i, &actor) || !actor.health ||
            actor.reward != STUN || actor.state == 4) continue;
        int bearing = (int)bfm_forest_shore_bearing(from[0] - actor.position[0],
                                                    from[2] - actor.position[2]);
        unsigned delta = (unsigned)(bearing - f->yaw) & 4095u;
        if (delta >= 2048) delta = 4095 - delta;
        /* The original steering chooses the narrowest angle in slot order;
         * physical contact is tested independently along the traveled step. */
        if (delta < best) {
            best = delta;
            memcpy(aim, actor.position, sizeof actor.position);
        }
    }
    return best < 512;
}

static int step_flight(BfmTowerSummitStun *s, BfmTowerSummitResidents *pool, BfmGameplayRun *run,
                       const int16_t hero[3], unsigned tick, int square)
{
    SummitContact contact = {.pool = pool, .room = s->room};
    int16_t aim[3];
    BfmFusionFlightPhase before = s->flight.phase;
    int aiming = before == BFM_FUSION_OUTBOUND && aim_blade(&s->flight, pool, aim);
    if (bfm_fusion_flight_tick(&s->flight, hero, aiming ? aim : NULL, contact_blade, &contact)
            != BFM_FUSION_ATTACHED) return 1;
    if (before != BFM_FUSION_ATTACHED) {
        BfmForestTetherTarget target = contact.target;
        if (!bfm_tower_summit_residents_bind(pool, target.slot, target.generation)) {
            bfm_fusion_release_tether(&s->flight);
            return 1;
        }
        s->tether = (BfmForestTether){.phase = BFM_TETHER_ATTACHED, .target = target,
            .gain = s->gain, .ability = STUN, .duration = s->duration,
            .award_message = s->message_address};
        return 1;
    }
    BfmForestTetherTarget target = s->tether.target;
    BfmTowerSummitResident actor;
    int alive = bfm_tower_summit_residents_get(pool, target.slot, &actor) && actor.health &&
        actor.generation == target.generation && actor.state == 4 &&
        actor.variant == target.variant && actor.reward == s->tether.ability;
    BfmForestTetherPhase phase = bfm_forest_tether_tick(&s->tether, alive ? &target : NULL,
                                                        tick, square, 0);
    if (phase == BFM_TETHER_ATTACHED)
        return bfm_tower_summit_residents_hold(pool, target.slot, target.generation, s->tether.meter);
    if (phase == BFM_TETHER_EARNED) {
        unsigned reward;
        if (!bfm_tower_summit_residents_consume(pool, target.slot, target.generation, &reward) ||
            reward != s->tether.ability) return 0;
    } else if (phase == BFM_TETHER_LOST && alive) {
        if (!bfm_tower_summit_residents_release(pool, target.slot, target.generation, run))
            return 0;
    }
    if (phase != BFM_TETHER_ATTACHED) bfm_fusion_release_tether(&s->flight);
    return 1;
}

static int step_award(BfmTowerSummitStun *s, BfmForestStatus *status,
                      const BfmSummitSkillInput *in, BfmSummitSkillFrame *out)
{
    if (s->award == NO_AWARD) {
        if (s->flight.phase != BFM_FUSION_DONE || s->tether.phase != BFM_TETHER_EARNED) return 1;
        status->record[0x49] = 0;
        s->key = 0;
        s->remaining = s->sectors;
        s->award = CATCH;
    } else if (s->award == CATCH) {
        s->award = LOAD;
    } else if (s->award == LOAD) {
        /* Presentation follows the mounted member's sector-wise transfer.
         * The native caster already retains its validated data for reuse. */
        if (s->remaining) --s->remaining;
        else {
            s->page = bfm_field_message_open(s->message, s->message_size);
            if (!s->page) return 0;
            s->award = READ;
        }
    } else if (s->award == READ) {
        const BfmForestMessageFrame *frame = bfm_field_message_tick(s->page, 0,
                in->confirm_edge, in->reveal_edge);
        if (!frame) return 0;
        if (frame->events & BFM_FOREST_MESSAGE_CLOSED) {
            bfm_forest_message_free(s->page);
            s->page = NULL;
            s->award = CLEAR;
        }
    } else if (s->award == CLEAR) {
        s->award = RELEASE;
    } else if (++s->key >= s->cast.award.count) {
        if (!bfm_forest_tether_commit(&s->tether, status)) return 0;
        s->award = NO_AWARD;
        return 1;
    }
    out->action = 22;
    out->pose = s->cast.award.keys[s->key].pose;
    out->visible = s->cast.award.keys[s->key].visible;
    out->award = s->page;
    out->suppress_square = 1;
    return 1;
}

static int step_stun(BfmTowerSummitStun *s, BfmTowerSummitResidents *pool, BfmGameplayRun *run,
                     const BfmForestTransform *root, int begin, BfmSummitSkillFrame *out)
{
    BfmForestStatusValues status = bfm_forest_status_values(&run->player);
    int paid = begin && status.ability == STUN && status.bp >= (int)s->cost;
    BfmTowerStunFrame frame;
    s->casting = bfm_tower_stun_cast_tick(s->emitter, paid, root, &frame);
    if (!s->casting) return !paid;
    if (paid) {
        bfm_forest_status_spend_bp(&run->player, s->cost);
        memset(s->struck, 0, sizeof s->struck);
    }
    out->action = 23;
    out->pose = frame.pose;
    out->visible = frame.visible;
    out->skill_body = frame.body;
    memcpy(out->skill_anchors, frame.blade, sizeof out->skill_anchors);
    out->suppress_square = 1;
    for (unsigned i = 0; i < BFM_SUMMIT_RESIDENTS; ++i) {
        BfmTowerSummitResident actor;
        if (!frame.ray_count || !bfm_tower_summit_residents_get(pool, i, &actor) ||
            !actor.health || actor.generation == s->struck[i]) continue;
        for (unsigned j = 0; j < frame.ray_count; ++j) {
            int16_t point[3], center[3];
            if (!bfm_tower_summit_residents_contact(pool, i, frame.origin, frame.end[j], center, point) ||
                bfm_forest_resolve_solid_segment(s->room, frame.origin, point, NULL)) continue;
            if (bfm_tower_summit_residents_stun(pool, i, actor.generation))
                s->struck[i] = actor.generation;
            break;
        }
    }
    return 1;
}

int bfm_tower_summit_stun_tick(BfmTowerSummitStun *s, BfmTowerSummitResidents *pool,
        BfmGameplayRun *run, const BfmForestTransform *root, int16_t yaw, unsigned tick,
        const BfmSummitSkillInput *in, BfmSummitSkillFrame *out)
{
    if (!s || !pool || !run || !run->has_player || !root || !in || !out) return 0;
    int16_t hero[3];
    for (unsigned axis = 0; axis < 3; ++axis) {
        if (root->translation[axis] < INT16_MIN || root->translation[axis] > INT16_MAX) return 0;
        hero[axis] = (int16_t)root->translation[axis];
    }
    *out = (BfmSummitSkillFrame){.visible = in->visible, .tether_gauge = -1};
    int away = s->flight.phase != BFM_FUSION_IDLE && s->flight.phase != BFM_FUSION_DONE;
    int awarding = s->award != NO_AWARD;
    if (s->cast.action == BFM_CAST_GROUND) s->cast.visible = in->visible;
    int available = in->grounded && !awarding && !s->casting;
    BfmForestCastEvent cast = bfm_forest_cast_tick(&s->cast, &run->player,
        available && in->shoulder, available && in->square_edge, in->moving, away);
    if (cast == BFM_CAST_STANCE || cast == BFM_CAST_THROW || cast == BFM_CAST_LAUNCH) {
        out->action = cast == BFM_CAST_STANCE ? 17 : 21;
        out->pose = s->cast.pose;
        out->visible = s->cast.visible;
        out->suppress_square = 1;
    }
    if (cast == BFM_CAST_LAUNCH) {
        const int16_t angles[3] = {0, yaw, 0}, tilt[3] = {0};
        if (!bfm_fusion_launch(&s->flight, root, angles, tilt)) return 0;
    } else if (away && !step_flight(s, pool, run, hero, tick, in->square_edge)) return 0;
    if (!step_award(s, &run->player, in, out)) return 0;
    if (!step_stun(s, pool, run, root, available && !away && cast == BFM_CAST_GROUND &&
            !out->action && !awarding && in->circle_edge, out)) return 0;

    out->blade_away = s->flight.phase != BFM_FUSION_IDLE && s->flight.phase != BFM_FUSION_DONE;
    if (out->blade_away) {
        out->suppress_square = 1;
        out->visible &= ~((UINT32_C(1) << 21) | (UINT32_C(1) << 22));
    }
    if (s->tether.phase == BFM_TETHER_ATTACHED ||
        (s->tether.phase == BFM_TETHER_EARNED && out->blade_away))
        out->tether_gauge = (int)s->tether.meter;
    out->blade = bfm_fusion_model_tick(s->model, &s->flight,
        s->tether.phase == BFM_TETHER_ATTACHED && in->square_edge);
    return 1;
}
