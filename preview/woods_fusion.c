/* SPDX-License-Identifier: MIT */
#include "field_fusion.h"
#include "bedroom_figure.h"
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

enum { SATIATE = 3, SHRINK = 13, SECTOR = 2048 };
enum WoodsAward { NO_AWARD, HOLD_AWARD, READ_AWARD, RELEASE_AWARD };

typedef struct {
    unsigned id, gain, partial_bp, section_kinds;
    uint32_t duration, message;
    size_t offset, size;
} Ability;

struct BfmWoodsFusion {
    const MusashiDiscMedia *disc;
    Ability abilities[2];
    size_t ability_count;
    const Ability *loaded_ability;
    size_t loaded, section_end;
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
    unsigned key, partial_bp;
    unsigned satiate_rule;
    int satiate_active;
    int partial_return;
};

/* Each recipient selects a distinct resident resource row for its award. */
static int mount_ability(Ability *ability, const uint8_t *scene,
                         const BfmFieldFusionCatalog *catalog, const BfmFieldFusionAbility *source,
                         const ForestPlayerArchive *player, const uint8_t *directory, size_t total)
{
    const uint32_t base = UINT32_C(0x80128158);
    unsigned id = source->id;
    uint8_t row[8];
    if (!forest_player_archive_read(player, UINT32_C(0x800d3764) + source->resource_row * 8, row, sizeof row) ||
        u32le(row + 4) != source->resource_tag) return 0;
    uint32_t member = u32le(row), count = u32le(directory);
    if (member >= count || count > (SECTOR - 8) / 8) return 0;
    const uint8_t *entry = directory + 8 + 8 * member;
    uint32_t sector = u32le(entry), size = u32le(entry + 4);
    if (!sector || sector > total / SECTOR || !size || size % SECTOR) return 0;
    ability->offset = (size_t)sector * SECTOR;
    ability->size = size;
    ability->id = id;
    ability->section_kinds = source->section_kinds;
    ability->gain = scene[catalog->gain - base + id];
    ability->partial_bp = scene[catalog->partial_bp - base + id];
    ability->duration = u32le(scene + (catalog->duration - base) + 4 * id);
    ability->message = u32le(scene + (catalog->message - base) + 4 * id);
    return size <= total - ability->offset && ability->gain && ability->partial_bp && ability->message;
}

static const Ability *ability_for(const BfmWoodsFusion *s, unsigned id)
{
    for (size_t i = 0; i < s->ability_count; ++i)
        if (s->abilities[i].id == id) return &s->abilities[i];
    return NULL;
}

/* Tick the actual mounted transfer. It can finish immediately on a cache
 * hit. This room uses both type-0 sections and the type-8 skill effects. */
static int read_package(BfmWoodsFusion *s)
{
    const Ability *ability = ability_for(s, s->tether.ability);
    if (!ability) return -1;
    if (ability != s->loaded_ability) {
        s->loaded = s->section_end = s->seen_sections = 0;
        s->loaded_ability = ability;
    }
    if (s->loaded < ability->size) {
        uint8_t sector[SECTOR];
        size_t total;
        if (!musashi_disc_media_read_root_range(s->disc, "MAIN.CD", ability->offset + s->loaded,
                                                sector, sizeof sector, &total)) return -1;
        if (s->loaded == s->section_end) {
            size_t size = u32le(sector + 12);
            unsigned kind = sector[4];
            if (memcmp(sector, "PAC", 4) || (kind > 3 && kind != 8) || size < SECTOR ||
                size > ability->size - s->loaded || (kind == 1 && sector[5])) return -1;
            size_t sectors = (size - 1) / SECTOR + 1;
            s->section_end = s->loaded + sectors * SECTOR;
            s->seen_sections |= 1u << kind;
        }
        s->loaded += SECTOR;
        if (s->loaded < ability->size) return 0;
    }
    return s->loaded == s->section_end && s->seen_sections == ability->section_kinds ? 1 : -1;
}

static int scene_range(uint32_t address, size_t length, size_t bytes)
{
    const uint32_t origin = UINT32_C(0x80128158);
    return address >= origin && address-origin <= bytes && length <= bytes-(address-origin);
}

BfmWoodsFusion *bfm_field_fusion_open(const MusashiDiscMedia *disc, const uint8_t *scene,
    size_t bytes, const BfmFieldFusionCatalog *catalog, const BfmFieldFusionAbility *abilities,
    size_t count)
{
    const uint32_t base = UINT32_C(0x80128158);
    if (!disc || !scene || !catalog || !abilities || !count || count > 2 ||
        !scene_range(catalog->room, 0x8c, bytes) || !scene_range(catalog->credit_palette, 1, bytes)) return NULL;
    for (size_t i = 0; i < count; ++i) {
        unsigned id = abilities[i].id;
        if (!id || id >= 32 || !scene_range(catalog->gain, id+1, bytes) ||
            !scene_range(catalog->partial_bp, id+1, bytes) ||
            !scene_range(catalog->duration, 4*(id+1), bytes) ||
            !scene_range(catalog->message, 4*(id+1), bytes)) return NULL;
    }
    ForestPlayerArchive player;
    uint8_t directory[SECTOR];
    size_t total;
    if (!forest_player_archive_open(disc, &player) ||
        !musashi_disc_media_read_root_range(disc, "MAIN.CD", 0,
                                            directory, sizeof directory, &total)) return NULL;
    BfmWoodsFusion *fusion = calloc(1, sizeof *fusion);
    if (!fusion) return NULL;
    fusion->disc = disc;
    fusion->ability_count = count;
    fusion->credit_palette = scene[catalog->credit_palette - base];
    uint32_t table = u32le(scene + (catalog->room - base) + 0x4c);
    fusion->surface = bfm_forest_motion_surface_open(scene, bytes, catalog->room);
    if (!fusion->surface || !bfm_field_flight_glow_load(&fusion->glow, scene, bytes, catalog->glow_edge) ||
        !bfm_forest_cast_load(&fusion->cast, disc, table) || !(fusion->model = bfm_fusion_model_open(disc))) {
        bfm_woods_fusion_free(fusion);
        return NULL;
    }
    for (size_t i = 0; i < count; ++i) {
        if (!mount_ability(&fusion->abilities[i], scene, catalog, &abilities[i], &player, directory, total)) {
            bfm_woods_fusion_free(fusion);
            return NULL;
        }
    }
    return fusion;
}

BfmWoodsFusion *bfm_woods_fusion_open(const MusashiDiscMedia *disc, const uint8_t *scene,
                                     size_t bytes)
{
    const uint32_t base = UINT32_C(0x80128158);
    static const BfmFieldFusionCatalog catalog = {
        .room=0x8018ea38, .gain=0x8018a58c, .partial_bp=0x8018a5ac, .duration=0x8018a510,
        .message=0x8018a208, .credit_palette=0x8018a978, .glow_edge=0x8018a700
    };
    static const BfmFieldFusionAbility abilities[] = {{SHRINK,4,14,0x10f}, {SATIATE,7,30,0x10f}};
    const size_t pink_reward = UINT32_C(0x801c9278) + 44 - base;
    if (!scene || bytes < pink_reward + 2 || u32le(scene + (UINT32_C(0x801877c4) - base)) != catalog.room ||
        u16le(scene + (UINT32_C(0x801c473c) - base) + 44) != SHRINK ||
        u16le(scene + pink_reward) != SATIATE) return NULL;
    unsigned rule = scene[UINT32_C(0x8018a4f0)-base+SATIATE];
    if (!(rule & 0x80u)) return NULL;
    BfmWoodsFusion *fusion = bfm_field_fusion_open(disc, scene, bytes, &catalog, abilities, 2);
    if (fusion) fusion->satiate_rule = rule;
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
    const BfmFieldFusionTargets *targets;
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
    int16_t point[3];
    int actor = room->targets->contact(room->targets->actors, from, to, point);
    if (actor > 0 && !blocked(room, from, point)) {
        room->struck = actor;
        return 1;
    }
    return blocked(room, from, to) ? -1 : 0;
}

static int aim_blade(const BfmFusionFlight *flight, const BfmFieldFusionTargets *targets,
                     int16_t aim[3])
{
    unsigned nearest_angle = 512;
    for (size_t i = 0; i < targets->count; ++i) {
        BfmFusionRecipient target;
        if (!targets->read(targets->actors, targets->order[i], &target)) continue;
        const int16_t *center = target.center;
        int x = flight->model.translation[0] - center[0];
        int z = flight->model.translation[2] - center[2];
        unsigned gap = (bfm_forest_shore_bearing(x, z) - flight->yaw) & 4095u;
        if (gap >= 2048) gap = 4095 - gap;
        if (gap < nearest_angle) {
            nearest_angle = gap;
            memcpy(aim, center, sizeof target.center);
        }
    }
    return nearest_angle < 512;
}

static int step_flight(BfmWoodsFusion *s, const BfmFieldFusionTargets *targets, const int16_t hero[3],
                       unsigned tick, int square)
{
    WoodsContact contact = {.targets = targets, .surface = s->surface};
    BfmFusionFlightPhase before = s->flight.phase;
    int16_t aim[3];
    int aimed = before == BFM_FUSION_OUTBOUND && aim_blade(&s->flight, targets, aim);
    if (bfm_fusion_flight_tick(&s->flight, hero, aimed ? aim : NULL, contact_blade, &contact)
        != BFM_FUSION_ATTACHED) return 1;

    if (before != BFM_FUSION_ATTACHED) {
        BfmFusionRecipient candidate;
        const Ability *ability = targets->read(targets->actors, contact.struck, &candidate) ?
                                 ability_for(s, candidate.ability) : NULL;
        if (!ability || !targets->react(targets->actors, contact.struck, BFM_FUSION_LATCH, 0)) {
            bfm_fusion_release_tether(&s->flight);
            return 1;
        }
        s->partial_bp = ability->partial_bp;
        s->tether = (BfmForestTether){.phase = BFM_TETHER_ATTACHED,
            .target = {.slot=(unsigned)contact.struck, .generation=candidate.generation},
            .ability = ability->id, .gain = ability->gain,
            .duration = ability->duration, .award_message = ability->message};
        int16_t blade[3];
        for (unsigned i = 0; i < 3; ++i) blade[i] = (int16_t)s->flight.model.translation[i];
        bfm_forest_tether_trails_begin(&s->trails, candidate.root, blade);
        return 1;
    }

    int target = (int)s->tether.target.slot;
    BfmFusionRecipient candidate;
    int present = targets->read(targets->actors, target, &candidate);
    present = present && candidate.generation == s->tether.target.generation;
    BfmForestTetherPhase phase = bfm_forest_tether_tick(&s->tether,
        present ? &s->tether.target : NULL, tick, square, 0);
    if (present) targets->react(targets->actors, target, BFM_FUSION_METER, s->tether.meter);
    if (phase == BFM_TETHER_ATTACHED) return 1;
    if (phase == BFM_TETHER_EARNED) {
        if (!targets->react(targets->actors, target, BFM_FUSION_ABSORB, 0)) return 0;
    } else if (present) {
        /* A timed-out tether still carries the particular recipient's BP
         * return. Losing that actor earlier leaves the returning blade empty. */
        s->partial_return = phase == BFM_TETHER_LOST && s->tether.ticks >= 128;
        targets->react(targets->actors, target, BFM_FUSION_RELEASE, 0);
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

void bfm_woods_satiate_cancel(BfmWoodsFusion *s, BfmForestStatus *status)
{
    if (!s || !status || status->record[0x49] != SATIATE) return;
    memset(status->record + 0x49, 0, 4);
    s->satiate_active = 0;
}

BfmWoodsSatiateEvent bfm_woods_satiate_step(BfmWoodsFusion *s, BfmForestStatus *status,
                                            unsigned action)
{
    if (!s || !status) return BFM_WOODS_SATIATE_QUIET;
    uint8_t *p = status->record;
    if ((p[0x49] & 0x7fu) != SATIATE) {
        s->satiate_active = 0;
        return BFM_WOODS_SATIATE_QUIET;
    }
    uint32_t ticks = u32le(p + 0x50);
    if (ticks) {
        --ticks;
        for (unsigned i = 0; i < 4; ++i) p[0x50+i] = (uint8_t)(ticks >> (8*i));
    }
    if (action == 22 || action == 26) return BFM_WOODS_SATIATE_QUIET;
    if (!s->satiate_active || !p[0x4c]) {
        s->satiate_active = 1;
        if (!p[0x4c]) {
            p[0x4c] = 1;
            bfm_forest_status_spend_bp(status, s->satiate_rule & 0x7fu);
        }
        return BFM_WOODS_SATIATE_BEGIN;
    }
    if (!ticks) {
        memset(p + 0x49, 0, 4);
        s->satiate_active = 0;
        return BFM_WOODS_SATIATE_END;
    }
    return BFM_WOODS_SATIATE_QUIET;
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
    if (s->tether.phase == BFM_TETHER_EARNED) frame->award_ability = s->tether.ability;
    if (s->award == NO_AWARD) {
        if (s->flight.phase != BFM_FUSION_DONE || s->tether.phase != BFM_TETHER_EARNED) return 1;
        s->key = 0;
        status->record[0x49] = 0;
        frame->award_face_slot = 24;
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
            frame->award_finished = 1;
            s->award = NO_AWARD;
            return 1;
        }
        frame->release_award = clip->keys[s->key].launch;
        if (frame->release_award) frame->award_face_slot = 17;
    }
    frame->action = 22;
    frame->pose = clip->keys[s->key].pose;
    frame->visible = grip(frame->visible, clip->keys[s->key].visible);
    return 1;
}

void bfm_woods_fusion_sample_charge(BfmGameplayRun *run, int guard_ready, int shoulder_held)
{
    uint8_t *charge = &run->player.record[0x47];
    if (!guard_ready) {
        *charge = 0;
    } else if (shoulder_held) {
        unsigned next = *charge + (bfm_gameplay_story_flag(run, 32) ? 6 : 3);
        *charge = next < 128 ? (uint8_t)next : 128;
    }
}

int bfm_field_fusion_step(BfmWoodsFusion *s, const BfmFieldFusionTargets *targets, BfmGameplayRun *run,
                          BfmForestStatus *status, const BfmBedroomRoot *root, unsigned field_tick,
                          const BfmWoodsFusionInput *input, BfmWoodsFusionFrame *frame)
{
    if (!s || !targets || !targets->actors || !targets->order || !targets->read ||
        !targets->contact || !targets->react || !run || !status || !root || !input || !frame) return 0;
    *frame = (BfmWoodsFusionFrame){.visible = input->visible, .tether_gauge = -1,
                                   .flight = &s->flight, .trails = &s->trails};
    int available = input->controllable && s->award == NO_AWARD;
    int square = available && input->square_edge;
    int attached = s->flight.phase == BFM_FUSION_ATTACHED;
    if (s->cast.action == BFM_CAST_GROUND) s->cast.visible = input->visible;
    s->cast.active_stance_gain = bfm_gameplay_story_flag(run, 0x20) ? 6 : 3;
    BfmForestCastEvent cast = BFM_CAST_GROUND;
    if (input->body_interrupted) {
        s->cast.action = BFM_CAST_GROUND;
        s->cast.key = 0;
    } else if (input->charge_sampled) {
        cast = bfm_forest_cast_after_charge(&s->cast, status, available && input->shoulder,
                                            square, input->moving, blade_away(s));
    } else {
        cast = bfm_forest_cast_tick(&s->cast, status, available && input->shoulder,
                                   square, input->moving, blade_away(s));
    }
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
    } else if (blade_away(s) && !step_flight(s, targets, root->xyz, field_tick, square)) {
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
    if (!input->body_interrupted && !step_award(s, status, frame)) return 0;
    frame->movement_locked = frame->action == 21 || frame->action == 22;
    frame->blade_away = blade_away(s);
    if (frame->blade_away) frame->visible = grip(frame->visible, 0);
    if (s->tether.phase == BFM_TETHER_ATTACHED ||
        (frame->blade_away && (s->tether.phase == BFM_TETHER_EARNED || s->partial_return)))
        frame->tether_gauge = (int)s->tether.meter;
    frame->blade = bfm_fusion_model_tick(s->model, &s->flight, attached && square);
    if (input->eye) {
        BfmForestTransform owner;
        const BfmForestTransform *held = NULL;
        if (s->award != NO_AWARD) {
            if (!bfm_bedroom_figure_actor_root(root, 0, &owner))
                bfm_forest_actor_root(root->yaw, root->xyz, &owner);
            held = &owner;
        }
        bfm_woods_flight_glow_tick(&s->glow, &s->flight, s->tether.meter, held, input->actor_tick,
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
        BfmFusionRecipient struck;
        const int16_t *target = NULL;
        if (s->flight.phase == BFM_FUSION_ATTACHED &&
            targets->read(targets->actors, (int)s->tether.target.slot, &struck)) target = struck.root;
        int absorbed = attached && s->tether.phase == BFM_TETHER_EARNED;
        bfm_woods_capture_light_tick(&s->glow.capture, target, absorbed, input->eye, trail_random, run);
    }
    return 1;
}

static int caster_read(const void *actors, int id, BfmFusionRecipient *target)
{
    target->ability = bfm_woods_caster_ability(actors, id);
    target->generation = 0;
    return bfm_woods_caster_target(actors, id, target->center) &&
           bfm_woods_caster_root(actors, id, target->root);
}

static int caster_contact(const void *actors, const int16_t from[3], const int16_t to[3], int16_t hit[3])
{
    int16_t center[3];
    return bfm_woods_caster_contact(actors, from, to, hit, center);
}

static int caster_react(void *actors, int id, BfmFusionRecipientEvent event, unsigned meter)
{
    switch (event) {
    case BFM_FUSION_LATCH: return bfm_woods_caster_latch(actors, id);
    case BFM_FUSION_METER: bfm_woods_caster_meter(actors, id, meter); return 1;
    case BFM_FUSION_RELEASE: bfm_woods_caster_release(actors, id); return 1;
    case BFM_FUSION_ABSORB: return bfm_woods_caster_absorb(actors, id);
    }
    return 0;
}

int bfm_woods_fusion_step(BfmWoodsFusion *s, BfmWoodsCaster *casters, BfmGameplayRun *run,
                          BfmForestStatus *status, const BfmBedroomRoot *root, unsigned field_tick,
                          const BfmWoodsFusionInput *input, BfmWoodsFusionFrame *frame)
{
    static const int order[] = {BFM_WOODS_SATIATE_TARGET, 1, 2};
    const BfmFieldFusionTargets targets = {casters, order, sizeof order / sizeof order[0],
                                            caster_read, caster_contact, caster_react};
    return bfm_field_fusion_step(s, &targets, run, status, root, field_tick, input, frame);
}
