/* SPDX-License-Identifier: MIT */
#include "village_mine_lake.h"
#include "village_mine_lake_hurt.h"
#include "village_mine_currents.h"
#include "village_mine_lake_blade.h"
#include "village_mine_lake_creatures.h"
#include "forest_terrain.h"
#include <string.h>
#include "village_mine_lake_fields.h"
#include "village_mine_lake_flyer.h"
#include "village_mine_lake_view.h"
#include <stdlib.h>

struct BfmVillageMineLake {
    BfmVillageMineLakeView view;
    BfmMineCurrents currents;
    BfmMineLakeCreatures *creatures;
    BfmMineLakeFlyers *flyers;
    BfmVillageMineLakeFields *fields;
    BfmMineLakeHurt *hurt;
    BfmMineLakeBlade *blade;
    BfmVillageMotion *motion;
    BfmSaveRecord *save;
    const uint8_t *main;
    size_t main_bytes;
    unsigned flyer_slot, current_slot, fixture_slot, first_free;
    int entered;
};

void bfm_village_mine_lake_free(BfmVillageMineLake *lake)
{
    if (!lake) return;
    bfm_mine_lake_blade_free(lake->blade);
    bfm_mine_lake_hurt_free(lake->hurt);
    bfm_village_mine_lake_fields_free(lake->fields);
    bfm_mine_lake_flyers_free(lake->flyers);
    bfm_mine_lake_creatures_free(lake->creatures);
    bfm_village_mine_lake_view_close(&lake->view);
    free(lake);
}

BfmVillageMineLake *bfm_village_mine_lake_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, const uint8_t *main, size_t main_bytes,
    uint16_t tiles[512][1024], BfmVillageMotion *motion, BfmSaveRecord *save)
{
    const BfmVillageGround *ground = bfm_village_motion_ground(motion);
    const BfmVillagePlayer *player = bfm_village_motion_player(motion);
    if (!ground || !player || !save) return NULL;
    BfmVillageMineLake *lake = calloc(1, sizeof *lake);
    if (!lake) return NULL;
    lake->motion = motion;
    lake->save = save;
    lake->main = main;
    lake->main_bytes = main_bytes;
    lake->creatures = bfm_mine_lake_creatures_open(disc, scene, bytes, ground, player->root.xyz);
    lake->flyers = bfm_mine_lake_flyers_open(scene, bytes, ground);
    lake->fields = bfm_village_mine_lake_fields_open(scene, bytes, save->variables);
    lake->hurt = bfm_mine_lake_hurt_open(disc, scene, bytes, motion, save);
    lake->blade = bfm_mine_lake_blade_open(disc, scene, bytes, main, main_bytes);
    if (!lake->creatures || !lake->flyers || !lake->fields || !lake->hurt || !lake->blade ||
        !bfm_mine_currents_load(&lake->currents, scene, bytes) ||
        !bfm_village_mine_lake_view_open(&lake->view, disc, scene, bytes, tiles)) {
        bfm_village_mine_lake_free(lake);
        return NULL;
    }
    /* The western parent stays in slot three only on the far entrance.
       Every flyer creates its own shadow before the next flyer is placed. */
    lake->flyer_slot = 3 + !!bfm_mine_lake_creatures_occupied(lake->creatures, 3);
    lake->current_slot = lake->flyer_slot + 2 * BFM_MINE_LAKE_FLYERS;
    lake->fixture_slot = lake->current_slot + BFM_MINE_CURRENT_COUNT;
    lake->first_free = lake->fixture_slot + 2 + !save->variables[58+21] + !save->variables[58+27];
    bfm_village_mine_lake_view_follow(&lake->view, player->root.xyz, 0);
    return lake;
}

BfmVillageMineView *bfm_village_mine_lake_view(const BfmVillageMineLake *lake)
{
    return lake ? lake->view.view : NULL;
}

BfmMineLakeHurt *bfm_village_mine_lake_hurt(const BfmVillageMineLake *lake)
{
    return lake ? lake->hurt : NULL;
}

static void current_sound(BfmMineLakeSound emit, void *context, BfmMineCurrentSound sound)
{
    if (emit && sound.command) emit(context, sound.command, sound.value);
}

static void enter(BfmVillageMineLake *lake, BfmGameplayRun *run, BfmMineLakeSound emit, void *context)
{
    const BfmVillageGround *ground = bfm_village_motion_ground(lake->motion);
    const int16_t *foot = bfm_village_motion_player(lake->motion)->root.xyz;
    bfm_mine_lake_flyers_enter(lake->flyers, run);
    /* During construction only the already placed sources are visible to
       the current actor's sound search. Ordinary fields see all five. */
    for (unsigned i = 0; i < BFM_MINE_CURRENT_COUNT; ++i) {
        BfmMineCurrentSound sound = {0};
        bfm_mine_currents_visit(&lake->currents, i, (1u << (i + 1)) - 1, ground, foot, &sound);
        current_sound(emit, context, sound);
    }
    lake->entered = 1;
}

int bfm_village_mine_lake_player(BfmVillageMineLake *lake, BfmGameplayRun *run, BfmFieldInput input,
    int pad_yaw, unsigned tick, BfmMineLakeSound emit, void *context)
{
    if (!lake) return 0;
    if (!lake->entered && run && run->has_player) enter(lake, run, emit, context);
    int16_t vector[3];
    int posted = bfm_mine_currents_take(&lake->currents, vector);
    bfm_village_motion_mine_lake_begin_field(lake->motion, posted ? vector : NULL);
    bfm_village_motion_mine_lake_fields(lake->motion, lake->fields, tick);
    int claimed = bfm_mine_lake_hurt_step(lake->hurt, run, input, (unsigned)pad_yaw, tick);
    if (claimed < 0) return 0;
    if (!bfm_mine_lake_blade_player(lake->blade, lake->motion, run, lake->save,
                                    input, pad_yaw, claimed)) return 0;
    unsigned sound = bfm_village_motion_mine_lake_take_sound(lake->motion);
    if (sound && emit) emit(context, sound, 0);
    return 1;
}

static int other_actor(void *context, unsigned slot)
{
    BfmVillageMineLake *lake = context;
    return slot < 3 || (slot >= lake->flyer_slot && slot < lake->first_free);
}

static void blade_contacts(BfmVillageMineLake *lake, const BfmWoodsCasterMeleeFrame *swing,
                           unsigned yaw, BfmMineLakeSound emit, void *context)
{
    if (!swing || !swing->event || !swing->rays || swing->rays > 3) return;
    const BfmVillageGround *ground = bfm_village_motion_ground(lake->motion);
    int connected = 0;
    for (unsigned i = 0; i < swing->rays; ++i) {
        int16_t end[3], normal[3];
        unsigned cue = 0;
        uint16_t material = 0;
        memcpy(end, swing->to[i], sizeof end);
        bfm_village_ground_blade(ground, swing->from, end, normal, &material);
        for (unsigned slot = 3; slot < BFM_FOREST_ACTOR_LIMIT; ++slot) {
            if (bfm_mine_lake_creatures_occupied(lake->creatures, slot) &&
                bfm_mine_lake_creatures_blade(lake->creatures, slot, swing->event, swing->strength,
                                              yaw, swing->from, end, normal, &cue))
                connected = 1;
        }
        if (cue && emit) emit(context, cue, 0);
    }
    if (connected) bfm_mine_lake_blade_connected(lake->blade);
}

int bfm_village_mine_lake_actors(BfmVillageMineLake *lake, BfmGameplayRun *run, int camera_pressed,
    unsigned tick, BfmMineLakeSound emit, void *context, int collapsing)
{
    if (!lake) return 0;
    const BfmVillagePlayer *p = bfm_village_motion_player(lake->motion);
    const BfmVillageGround *ground = bfm_village_motion_ground(lake->motion);
    bfm_village_mine_lake_view_follow(&lake->view, p->root.xyz, camera_pressed);
    bfm_village_mine_lake_view_tick_tiles(&lake->view);
    const BfmBedroomView *projection = bfm_village_mine_view_projection(lake->view.view);
    if (!projection) return 0;
    const BfmWoodsCasterMeleeFrame *swing = collapsing ? NULL : bfm_mine_lake_blade_frame(lake->blade);
    if (swing && swing->sound && emit) emit(context, swing->sound, 0);
    blade_contacts(lake, swing, p->root.yaw, emit, context);
    unsigned hurt_sound = bfm_mine_lake_hurt_take_sound(lake->hurt);
    if (!collapsing && hurt_sound && emit) emit(context, hurt_sound, 0);

    int32_t position[3];
    if (!bfm_village_motion_position_q16(lake->motion, position)) return 0;
    int16_t foot[3];
    for (unsigned axis = 0; axis < 3; ++axis)
        foot[axis] = (int16_t)((uint32_t)position[axis] >> 16);
    const int16_t *target = collapsing ? NULL : bfm_mine_lake_hurt_target(lake->hurt);
    for (unsigned slot = 3; slot < BFM_FOREST_ACTOR_LIMIT; ++slot) {
        if (slot == lake->flyer_slot) {
            uint16_t controls[BFM_MINE_LAKE_FLYERS];
            unsigned requests = bfm_mine_lake_flyers_step(lake->flyers, run, foot, controls);
            if (emit) {
                for (unsigned i = 0; i < requests; ++i) emit(context, 0x6fe, controls[i]);
            }
        } else if (slot == lake->current_slot) {
            BfmMineCurrentSound sound = {0};
            bfm_mine_currents_step(&lake->currents, ground, foot, &sound);
            current_sound(emit, context, sound);
        } else if (slot == lake->fixture_slot) {
            bfm_village_mine_lake_fields_step(lake->fields, run, foot, tick);
        } else if (bfm_mine_lake_creatures_occupied(lake->creatures, slot)) {
            BfmMineLakeCreatureEvent event = {0};
            if (!bfm_mine_lake_creatures_visit(lake->creatures, slot, foot, p->root.xyz, target,
                    run, tick, projection, other_actor, lake, &event)) return 0;
            if (event.struck)
                bfm_mine_lake_hurt_queue(lake->hurt, &event.hit);
            if (emit && event.landing_cue) emit(context, event.landing_cue, event.landing_control);
        }
    }
    if (run) bfm_mine_lake_creatures_admit(lake->creatures, p->root.xyz, other_actor, lake);
    return 1;
}

static int submit(void *context, BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    const BfmVillageMineLake *lake = context;
    for (unsigned slot = 3; slot < BFM_FOREST_ACTOR_LIMIT; ++slot) {
        if (slot == lake->flyer_slot) {
            if (!bfm_mine_lake_flyers_draw(lake->flyers, figure, view)) return 0;
        } else if (slot == lake->fixture_slot) {
            if (!bfm_village_mine_lake_fields_draw(lake->fields, figure, view)) return 0;
        } else if (bfm_mine_lake_creatures_occupied(lake->creatures, slot) &&
                   !bfm_mine_lake_creatures_draw_slot(lake->creatures, slot, figure, view)) return 0;
    }
    return bfm_mine_lake_hurt_draw(lake->hurt, figure, view);
}

int bfm_village_mine_lake_draw(BfmVillageMineLake *lake, unsigned tick, unsigned scale, int hidden,
    uint32_t pixels[240][320])
{
    if (!lake) return 0;
    const BfmVillagePlayer *p = bfm_village_motion_player(lake->motion);
    BfmBedroomPose pose = p->pose;
    unsigned visible = bfm_village_field_visible(&pose, lake->save->flags);
    if (!bfm_mine_lake_blade_body(lake->blade, lake->main, lake->main_bytes, &pose) ||
        !bfm_mine_lake_hurt_body(lake->hurt, lake->main, lake->main_bytes, &pose, &visible))
        return 0;
    if (hidden) visible = 0;
    return bfm_village_mine_view_draw_parts(lake->view.view, &p->root, &pose,
        lake->save->flags, tick, visible, scale, pixels, submit, lake);
}
