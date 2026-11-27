/* SPDX-License-Identifier: MIT */
#include "woods_fusion.h"
#include "woods_caster.h"
#include "woods_flight_glow.h"
#include "forest_absorption.h"
#include "forest_cast.h"
#include "forest_fusion_model.h"
#include "forest_shore_motion.h"
#include "../pc_port/forest_player_archive.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>
#include <string.h>

enum { SHRINK = 13, SECTOR = 2048 };
enum WoodsAward { NO_AWARD, HOLD_AWARD, READ_AWARD, RELEASE_AWARD };

struct BfmWoodsFusion {
    const MusashiDiscMedia *disc;
    size_t package_offset, package_size, loaded, section_end;
    unsigned seen_sections;
    BfmForestField *surface;
    BfmForestCast cast;
    BfmFusionFlight flight;
    BfmForestTether tether;
    BfmForestTetherTrails trails;
    BfmWoodsFlightGlow glow;
    BfmForestDamageDigits bp_digits;
    uint8_t credit_palette;
    BfmFusionModel *model;
    enum WoodsAward award;
    unsigned key, gain, partial_bp;
    int partial_return;
    uint32_t duration, message;
};

/* The resident selector requests its skill resource before opening the award
 * text. A different ability, such as the summit's Stun, uses another row. */
static int locate_package(BfmWoodsFusion *s, const MusashiDiscMedia *disc)
{
    ForestPlayerArchive player;
    uint8_t row[8], directory[SECTOR];
    size_t total;
    if (!forest_player_archive_open(disc, &player) ||
        !forest_player_archive_read(&player, UINT32_C(0x800d3784), row, sizeof row) ||
        u32le(row + 4) != 14 || !musashi_disc_media_read_root_range(disc, "MAIN.CD", 0,
            directory, sizeof directory, &total)) return 0;
    uint32_t member = u32le(row), count = u32le(directory);
    if (member >= count || count > (SECTOR - 8) / 8) return 0;
    const uint8_t *entry = directory + 8 + 8 * member;
    uint32_t sector = u32le(entry), bytes = u32le(entry + 4);
    if (!sector || sector > total / SECTOR || !bytes || bytes % SECTOR) return 0;
    s->package_offset = (size_t)sector * SECTOR;
    s->package_size = bytes;
    s->disc = disc;
    return bytes <= total - s->package_offset;
}

/* Tick the actual mounted transfer. It can finish immediately on a cache
 * hit. This room uses both type-0 sections and the type-8 skill effects. */
static int read_package(BfmWoodsFusion *s)
{
    if (s->loaded < s->package_size) {
        uint8_t sector[SECTOR];
        size_t total;
        if (!musashi_disc_media_read_root_range(s->disc, "MAIN.CD", s->package_offset + s->loaded,
                                                sector, sizeof sector, &total)) return -1;
        if (s->loaded == s->section_end) {
            size_t size = u32le(sector + 12);
            unsigned kind = sector[4];
            if (memcmp(sector, "PAC", 4) || (kind > 3 && kind != 8) || size < SECTOR ||
                size > s->package_size - s->loaded || (kind == 1 && sector[5])) return -1;
            size_t sectors = (size - 1) / SECTOR + 1;
            s->section_end = s->loaded + sectors * SECTOR;
            s->seen_sections |= 1u << kind;
        }
        s->loaded += SECTOR;
        if (s->loaded < s->package_size) return 0;
    }
    return s->loaded == s->section_end && s->seen_sections == 0x10fu ? 1 : -1;
}

/* SC03/29 uses the resident player's casting clips, while the actor profile
 * and its assimilation catalogue belong to the current room overlay. */
BfmWoodsFusion *bfm_woods_fusion_open(const MusashiDiscMedia *disc, const uint8_t *scene,
                                     size_t bytes)
{
    const uint32_t base = UINT32_C(0x80128158), room = UINT32_C(0x8018ea38);
    const size_t reward_offset = UINT32_C(0x801c473c) + 44 - base;
    if (!disc || !scene || bytes < reward_offset + 2 ||
        u32le(scene + (UINT32_C(0x801877c4) - base)) != room ||
        u16le(scene + reward_offset) != SHRINK) return NULL;

    BfmWoodsFusion *fusion = calloc(1, sizeof *fusion);
    if (!fusion) return NULL;
    fusion->gain = scene[UINT32_C(0x8018a58c) - base + SHRINK];
    fusion->partial_bp = scene[UINT32_C(0x8018a5ac) - base + SHRINK];
    fusion->credit_palette = scene[UINT32_C(0x8018a974) - base + 4];
    fusion->duration = u32le(scene + (UINT32_C(0x8018a510) - base) + 4 * SHRINK);
    fusion->message = u32le(scene + (UINT32_C(0x8018a208) - base) + 4 * SHRINK);
    uint32_t table = u32le(scene + (room - base) + 0x4c);
    fusion->surface = bfm_forest_motion_surface_open(scene, bytes, room);
    if (!fusion->gain || !fusion->partial_bp || !fusion->message || !fusion->surface ||
        !bfm_woods_flight_glow_load(&fusion->glow, scene, bytes) ||
        !locate_package(fusion, disc) || !bfm_forest_cast_load(&fusion->cast, disc, table) ||
        !(fusion->model = bfm_fusion_model_open(disc))) {
        bfm_woods_fusion_free(fusion);
        return NULL;
    }
    return fusion;
}

void bfm_woods_fusion_free(BfmWoodsFusion *fusion)
{
    if (!fusion) return;
    bfm_fusion_model_free(fusion->model);
    bfm_forest_field_free(fusion->surface);
    free(fusion);
}

typedef struct {
    BfmWoodsCaster *casters;
    const BfmForestField *surface;
    int struck;
} WoodsContact;

static int blocked(const WoodsContact *room, const int16_t from[3], const int16_t to[3])
{
    /* A material-only trigger does not stop the sword. The upper flag bits
     * are the resolved solid ground/slope/wall contacts. */
    return (bfm_forest_resolve_solid_segment(room->surface, from, to, NULL) & 0xe000u) != 0;
}

static int contact_blade(void *context, const int16_t from[3], const int16_t to[3])
{
    WoodsContact *room = context;
    int16_t point[3], center[3];
    int actor = bfm_woods_caster_contact(room->casters, from, to, point, center);
    if (actor > 0 && !blocked(room, from, point)) {
        room->struck = actor;
        return 1;
    }
    return blocked(room, from, to) ? -1 : 0;
}

static int aim_blade(const BfmFusionFlight *flight, const BfmWoodsCaster *casters,
                     int16_t aim[3])
{
    unsigned nearest_angle = 512;
    for (int target = 1; target <= BFM_WOODS_CASTERS; ++target) {
        int16_t center[3];
        if (!bfm_woods_caster_target(casters, target, center)) continue;
        int x = flight->model.translation[0] - center[0];
        int z = flight->model.translation[2] - center[2];
        unsigned gap = (bfm_forest_shore_bearing(x, z) - flight->yaw) & 4095u;
        if (gap >= 2048) gap = 4095 - gap;
        if (gap < nearest_angle) {
            nearest_angle = gap;
            memcpy(aim, center, sizeof center);
        }
    }
    return nearest_angle < 512;
}

static int step_flight(BfmWoodsFusion *s, BfmWoodsCaster *casters, const int16_t hero[3],
                       unsigned tick, int square)
{
    WoodsContact contact = {.casters = casters, .surface = s->surface};
    BfmFusionFlightPhase before = s->flight.phase;
    int16_t aim[3];
    int aimed = before == BFM_FUSION_OUTBOUND && aim_blade(&s->flight, casters, aim);
    if (bfm_fusion_flight_tick(&s->flight, hero, aimed ? aim : NULL, contact_blade, &contact)
        != BFM_FUSION_ATTACHED) return 1;

    if (before != BFM_FUSION_ATTACHED) {
        int16_t root[3];
        if (!bfm_woods_caster_root(casters, contact.struck, root) ||
            !bfm_woods_caster_latch(casters, contact.struck)) {
            bfm_fusion_release_tether(&s->flight);
            return 1;
        }
        s->tether = (BfmForestTether){.phase = BFM_TETHER_ATTACHED,
            .target.slot = (unsigned)contact.struck, .ability = SHRINK, .gain = s->gain,
            .duration = s->duration, .award_message = s->message};
        int16_t blade[3];
        for (unsigned i = 0; i < 3; ++i) blade[i] = (int16_t)s->flight.model.translation[i];
        bfm_forest_tether_trails_begin(&s->trails, root, blade);
        return 1;
    }

    int target = (int)s->tether.target.slot;
    int16_t center[3];
    int present = bfm_woods_caster_target(casters, target, center);
    BfmForestTetherPhase phase = bfm_forest_tether_tick(&s->tether,
        present ? &s->tether.target : NULL, tick, square, 0);
    if (present) bfm_woods_caster_meter(casters, target, s->tether.meter);
    if (phase == BFM_TETHER_ATTACHED) return 1;
    if (phase == BFM_TETHER_EARNED) {
        if (!bfm_woods_caster_absorb(casters, target)) return 0;
    } else if (present) {
        /* A timed-out tether still carries the particular recipient's BP
         * return. Losing that actor earlier leaves the returning blade empty. */
        s->partial_return = phase == BFM_TETHER_LOST && s->tether.ticks >= 128;
        bfm_woods_caster_release(casters, target);
    }
    bfm_fusion_release_tether(&s->flight);
    return 1;
}

static int blade_away(const BfmWoodsFusion *fusion)
{
    return fusion->flight.phase != BFM_FUSION_IDLE && fusion->flight.phase != BFM_FUSION_DONE;
}

/* The mounted cast keys select which Fusion model is in Musashi's hand.
 * Keep the off-hand Lumina equipment and body parts from ordinary play. */
static uint32_t grip(uint32_t equipped, uint32_t key)
{
    const uint32_t blades = (UINT32_C(1) << 21) | (UINT32_C(1) << 22);
    return (equipped & ~blades) | (key & blades);
}

static unsigned trail_random(void *run)
{
    return bfm_gameplay_random(run);
}

int bfm_woods_fusion_awarding(const BfmWoodsFusion *s)
{
    return s && s->award != NO_AWARD;
}

int bfm_woods_fusion_close_award(BfmWoodsFusion *s)
{
    if (!s || s->award != READ_AWARD) return 0;
    s->award = RELEASE_AWARD;
    return 1;
}

static int step_award(BfmWoodsFusion *s, BfmForestStatus *status, BfmWoodsFusionFrame *frame)
{
    const BfmForestCastClip *clip = &s->cast.award;
    if (s->award == NO_AWARD) {
        if (s->flight.phase != BFM_FUSION_DONE || s->tether.phase != BFM_TETHER_EARNED) return 1;
        s->key = 0;
        status->record[0x49] = 0;
        s->award = HOLD_AWARD;
    } else if (s->award == HOLD_AWARD) {
        if (s->key + 1 < clip->hold_after) ++s->key;
        else {
            int ready = read_package(s);
            if (ready < 0) return 0;
            if (ready) {
                frame->open_award = 1;
                s->award = READ_AWARD;
            }
        }
    } else if (s->award == RELEASE_AWARD) {
        if (++s->key == clip->count) {
            if (!bfm_forest_tether_commit(&s->tether, status)) return 0;
            s->award = NO_AWARD;
            return 1;
        }
        frame->release_award = clip->keys[s->key].launch;
    }
    frame->action = 22;
    frame->pose = clip->keys[s->key].pose;
    frame->visible = grip(frame->visible, clip->keys[s->key].visible);
    return 1;
}

int bfm_woods_fusion_step(BfmWoodsFusion *s, BfmWoodsCaster *casters, BfmGameplayRun *run,
                          BfmForestStatus *status, const BfmBedroomRoot *root, unsigned field_tick,
                          const BfmWoodsFusionInput *input, BfmWoodsFusionFrame *frame)
{
    if (!s || !casters || !run || !status || !root || !input || !frame) return 0;
    *frame = (BfmWoodsFusionFrame){.visible = input->visible, .tether_gauge = -1,
                                   .flight = &s->flight, .trails = &s->trails};
    int available = input->controllable && s->award == NO_AWARD;
    int square = available && input->square_edge;
    int attached = s->flight.phase == BFM_FUSION_ATTACHED;
    if (s->cast.action == BFM_CAST_GROUND) s->cast.visible = input->visible;
    BfmForestCastEvent cast = bfm_forest_cast_tick(&s->cast, status,
        available && input->shoulder, square, input->moving, blade_away(s));
    if (cast == BFM_CAST_STANCE || cast == BFM_CAST_THROW || cast == BFM_CAST_LAUNCH) {
        frame->action = cast == BFM_CAST_STANCE ? 17 : 21;
        frame->pose = s->cast.pose;
        frame->visible = grip(frame->visible, s->cast.visible);
    }
    if (cast == BFM_CAST_LAUNCH) {
        int16_t angles[3] = {0, (int16_t)root->yaw, 0}, tilt[3] = {0};
        BfmForestTransform model;
        bfm_forest_actor_root(root->yaw, root->xyz, &model);
        if (!bfm_fusion_launch(&s->flight, &model, angles, tilt)) return 0;
    } else if (blade_away(s) && !step_flight(s, casters, root->xyz, field_tick, square)) {
        return 0;
    }
    bfm_forest_damage_digits_advance(&s->bp_digits);
    if (s->partial_return && s->flight.phase == BFM_FUSION_DONE) {
        s->partial_return = 0;
        if (bfm_forest_status_values(status).hp > 0) {
            unsigned maximum = u16le(status->record + 0x3e);
            unsigned bp = u16le(status->record + 0x40) + s->partial_bp;
            if (bp > maximum) bp = maximum;
            status->record[0x40] = (uint8_t)bp;
            status->record[0x41] = (uint8_t)(bp >> 8);
            frame->bp_credit = s->partial_bp;
            bfm_forest_damage_digits_start_data(&s->bp_digits, &s->credit_palette,
                                                root->xyz, (uint16_t)s->partial_bp);
            s->bp_digits.bp_credit = 1;
        }
    }
    if (s->bp_digits.amount) frame->bp_digits = &s->bp_digits;
    if (!step_award(s, status, frame)) return 0;
    frame->movement_locked = frame->action == 21 || frame->action == 22;
    frame->blade_away = blade_away(s);
    if (frame->blade_away) frame->visible = grip(frame->visible, 0);
    if (s->tether.phase == BFM_TETHER_ATTACHED ||
        (s->tether.phase == BFM_TETHER_EARNED && frame->blade_away))
        frame->tether_gauge = (int)s->tether.meter;
    frame->blade = bfm_fusion_model_tick(s->model, &s->flight, attached && square);
    if (input->eye) {
        bfm_woods_flight_glow_tick(&s->glow, &s->flight, s->tether.meter,
                                   input->eye, trail_random, run);
        frame->glow = &s->glow;
    }
    if (s->trails.enabled && (attached || s->flight.phase != BFM_FUSION_ATTACHED)) {
        int16_t blade[3];
        for (unsigned i = 0; i < 3; ++i) blade[i] = (int16_t)s->flight.model.translation[i];
        bfm_forest_tether_trails_tick(&s->trails, s->flight.phase == BFM_FUSION_ATTACHED,
            s->tether.meter, frame->blade ? blade : NULL, root->xyz, trail_random, run);
    }
    if (input->eye) {
        int16_t struck[3];
        const int16_t *target = NULL;
        if (s->flight.phase == BFM_FUSION_ATTACHED &&
            bfm_woods_caster_root(casters, (int)s->tether.target.slot, struck)) target = struck;
        int absorbed = attached && s->tether.phase == BFM_TETHER_EARNED;
        bfm_woods_capture_light_tick(&s->glow.capture, target, absorbed, input->eye, trail_random, run);
    }
    return 1;
}
