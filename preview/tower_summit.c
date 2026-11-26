/* SPDX-License-Identifier: MIT */
#include "tower_summit.h"
#include "tower_sky.h"
#include "tower_summit_figures.h"
#include "tower_summit_pressure.h"
#include "tower_escape_props.h"

#include <stdlib.h>

struct BfmTowerSummit {
    BfmForestField *field;
    BfmTowerSummitFigures *figures;
    BfmTowerSummitResidents *residents;
    BfmTowerSummitPickup *pickup;
    BfmTowerSummitStun *skill;
    BfmSummitSkillFrame skill_frame;
    int32_t eye[3];
    BfmTowerSky *sky;
    const MusashiDiscMedia *disc;
    BfmTowerSummitArrival arrival;
    BfmTowerSummitPressure pressure;
    BfmTowerEscapeProps *road;
    BfmForestTransform road_head;
    int escaping, head_relocated, head_visible;
    BfmTowerSummitLesson lesson;
    unsigned field_updates;
    BfmForestMessageArt *art;
    int arrival_active;
};

BfmTowerSummit *bfm_tower_summit_open(const MusashiDiscMedia *disc,
                                      const RoomTransferDescriptor *destination)
{
    if (!disc || !destination || destination->room != 0x300b ||
        destination->parameter || destination->world) return NULL;
    BfmTowerSummit *summit = calloc(1, sizeof(*summit));
    if (!summit) return NULL;
    summit->disc = disc;
    summit->field = bfm_forest_field_open(disc, destination->room);
    size_t size = 0;
    const uint8_t *scene = bfm_forest_field_scene(summit->field, &size);
    if (scene) {
        summit->figures = bfm_tower_summit_figures_open(scene, size);
        summit->residents = bfm_tower_summit_residents_open(summit->field);
        summit->sky = bfm_tower_summit_sky_open(scene, size, bfm_forest_field_tiles(summit->field));
    }
    int16_t plate[3], sword[3];
    if (!summit->sky || !summit->residents || !bfm_tower_summit_figures_targets(summit->figures, plate, sword)) {
        bfm_tower_summit_free(summit);
        return NULL;
    }
    bfm_tower_summit_pressure_init(&summit->pressure, plate, sword);
    return summit;
}

void bfm_tower_summit_free(BfmTowerSummit *summit)
{
    if (summit) {
        bfm_tower_summit_arrival_close(&summit->arrival);
        bfm_tower_summit_lesson_close(&summit->lesson);
        bfm_forest_message_art_free(summit->art);
        bfm_tower_escape_props_free(summit->road);
        bfm_tower_summit_figures_free(summit->figures);
        bfm_tower_summit_stun_free(summit->skill);
        bfm_tower_summit_residents_free(summit->residents);
        bfm_tower_summit_pickup_free(summit->pickup);
        bfm_tower_sky_free(summit->sky);
        bfm_forest_field_free(summit->field);
        free(summit);
    }
}

BfmForestField *bfm_tower_summit_room(BfmTowerSummit *summit)
{
    return summit ? summit->field : NULL;
}

int bfm_tower_summit_place(BfmTowerSummit *summit, ForestPlayerState *player,
                           int16_t *yaw, int16_t position[3])
{
    return summit && bfm_forest_place_player(summit->field, 0, player, yaw, position);
}

int bfm_tower_summit_targets(const BfmTowerSummit *summit, int16_t plate[3], int16_t sword[3])
{
    return summit && bfm_tower_summit_figures_targets(summit->figures, plate, sword);
}

void bfm_tower_summit_present_sword(BfmTowerSummit *summit, unsigned tick, unsigned settle,
                                    int shielded, int sword_present)
{
    if (summit)
        bfm_tower_summit_figures_present_sword(summit->figures, tick, settle, shielded, sword_present);
}

int bfm_tower_summit_begin(BfmTowerSummit *summit, uint32_t parts, BfmForestPose *pose)
{
    if (!summit || !pose || summit->art) return 0;
    BfmForestMessageArt *art = bfm_tower_summit_arrival_art(summit->field, summit->disc);
    if (!art) return 0;
    if (!bfm_tower_summit_arrival_begin(&summit->arrival, summit->field, summit->disc, parts, pose)) {
        bfm_forest_message_art_free(art);
        return 0;
    }
    summit->art = art;
    summit->arrival_active = 1;
    return 1;
}

int bfm_tower_summit_tick_arrival(BfmTowerSummit *summit, ForestVoice *voice, unsigned *generation,
                                  int entrance_ready, int confirm, int reveal, BfmForestPose *pose)
{
    if (!summit || !summit->art) return BFM_SUMMIT_ARRIVAL_ERROR;
    if (!summit->arrival_active) return BFM_SUMMIT_ARRIVAL_RELEASED;
    int state = bfm_tower_summit_arrival_tick(&summit->arrival, voice, generation,
                                               entrance_ready, confirm, reveal, pose);
    if (state == BFM_SUMMIT_ARRIVAL_RELEASED) summit->arrival_active = 0;
    return state;
}

void bfm_tower_summit_update_sword(BfmTowerSummit *summit, unsigned tick,
                                   int plate_occupied, int sword_present)
{
    if (summit) bfm_tower_summit_figures_update_sword(summit->figures, tick, plate_occupied, sword_present);
}

const BfmTowerSummitArrival *bfm_tower_summit_arriving(const BfmTowerSummit *summit)
{
    return summit && summit->arrival_active ? &summit->arrival : NULL;
}

BfmTowerSummitPickup *bfm_tower_summit_pickup(BfmTowerSummit *summit)
{
    return summit ? summit->pickup : NULL;
}

BfmTowerSummitResidents *bfm_tower_summit_residents(BfmTowerSummit *summit)
{
    return summit ? summit->residents : NULL;
}

int bfm_tower_summit_step_fixtures(BfmTowerSummit *summit, const int16_t player[3],
                                   const int32_t eye[3], BfmGameplayRun *run, int scripted, uint32_t parts)
{
    if (!summit || !eye) return 0;
    for (unsigned i = 0; i < 3; ++i) summit->eye[i] = eye[i];
    /* The absorption award suspends the guard manager and its actors. */
    if (!scripted && summit->skill_frame.action == 22) {
        bfm_tower_summit_residents_pause(summit->residents);
    } else if (!bfm_tower_summit_residents_tick(summit->residents, player, scripted,
                                                summit->pressure.sword_requested, run)) {
        return 0;
    }
    unsigned events = bfm_tower_summit_pressure_step(&summit->pressure, player, summit->residents);
    if (events & BFM_SUMMIT_SWORD_IN_REACH) {
        summit->pickup = bfm_tower_summit_pickup_begin(summit->field, summit->disc, parts);
        if (!summit->pickup) return 0;
        summit->pressure.sword_requested = 1;
    }
    bfm_tower_summit_update_sword(summit, summit->field_updates++, summit->pressure.occupied,
                                  !summit->pressure.sword_requested);
    if (events & BFM_SUMMIT_FIRST_PLATE_LESSON) {
        if (!bfm_tower_summit_lesson_begin(&summit->lesson, summit->field, player, eye)) return 0;
        summit->pressure.lesson_started = 1;
    }
    return 1;
}

const BfmSummitSkillFrame *bfm_tower_summit_skill_tick(BfmTowerSummit *summit,
    uint32_t animation_table, const BfmForestTransform *root, int16_t yaw, unsigned tick,
    const BfmSummitSkillInput *input, BfmGameplayRun *run)
{
    if (!summit || !root || !input || !run || !run->has_player) return NULL;
    if (!summit->skill) {
        summit->skill = bfm_tower_summit_stun_open(summit->disc, summit->field, animation_table);
        if (!summit->skill) return NULL;
    }
    if (!bfm_tower_summit_stun_tick(summit->skill, summit->residents, run, root, yaw,
                                    tick, input, &summit->skill_frame)) return NULL;
    return &summit->skill_frame;
}

BfmTowerSummitLesson *bfm_tower_summit_active_lesson(BfmTowerSummit *summit)
{
    return summit && summit->lesson.phase ? &summit->lesson : NULL;
}

int bfm_tower_summit_resume_escape(BfmTowerSummit *summit, BfmTowerEscape *escape,
                                   BfmTowerEscapeCamera *camera)
{
    if (!summit || !escape || !camera) return 0;
    BfmTowerEscape next = *escape;
    if (!bfm_tower_escape_resume(&next, summit->field)) return 0;
    int lower = next.checkpoint == BFM_ESCAPE_LOWER_CHECKPOINT;
    BfmTowerEscapeProps *road = NULL;
    if (lower) {
        road = bfm_tower_escape_props_open(summit->field);
        if (!road) return 0;
        bfm_tower_escape_props_tick(road, next.head.position[2]);
    }
    bfm_tower_escape_props_free(summit->road);
    summit->road = road;
    summit->road_head = next.head.root;
    summit->escaping = summit->head_relocated = summit->head_visible = 1;
    bfm_tower_escape_lens_resume(camera, lower, next.position[0]);
    *escape = next;
    return 1;
}

int bfm_tower_summit_road_tick(BfmTowerSummit *summit, const BfmTowerEscape *escape)
{
    if (!summit || !escape) return 0;
    summit->escaping = 1;
    summit->road_head = escape->head.root;
    summit->head_relocated = escape->phase >= BFM_ESCAPE_REVEAL;
    summit->head_visible = escape->head.visible;
    if (escape->phase > BFM_ESCAPE_REVEAL || (escape->phase == BFM_ESCAPE_REVEAL && escape->age >= 19)) {
        if (!bfm_forest_mount_summit_road(summit->field, summit->disc)) return 0;
    }
    if (escape->checkpoint != BFM_ESCAPE_LOWER_CHECKPOINT) return 1;
    if (!summit->road) summit->road = bfm_tower_escape_props_open(summit->field);
    if (!summit->road) return 0;
    bfm_tower_escape_props_tick(summit->road, escape->head.position[2]);
    return 1;
}

int bfm_tower_summit_draw(const BfmTowerSummit *summit, const BfmForestCamera *camera,
                          const BfmForestPose *pose, const BfmForestTransform *root,
                          uint32_t visible_parts, unsigned buffer, const BfmForestBladeFrame *blade,
                          uint32_t pixels[240][320])
{
    if (!summit || !root || !bfm_tower_sky_draw(summit->sky, camera, pixels)) return 0;
    const BfmTowerSummitArrival *arrival = bfm_tower_summit_arriving(summit);
    BfmForestTransform model = *root;
    if (arrival) {
        visible_parts = arrival->parts;
        for (unsigned axis = 0; axis < 3; ++axis) {
            for (unsigned row = 0; row < 3; ++row) {
                int component = model.rotation[row][axis] * arrival->scale[axis];
                model.rotation[row][axis] = (int16_t)(component / 4096 - (component % 4096 < 0));
            }
        }
        if (arrival->scale[0] != 4096 || !(visible_parts & (UINT32_C(1) << 22))) blade = NULL;
    }
    const BfmSummitPickupFrame *pickup = bfm_tower_summit_pickup_frame(summit->pickup);
    const BfmSummitSkillFrame *skill = summit->skill && !arrival && !pickup && !summit->lesson.phase
                                     ? &summit->skill_frame : NULL;
    if (skill && skill->blade_away) {
        visible_parts &= ~((UINT32_C(1) << 21) | (UINT32_C(1) << 22));
        blade = NULL;
    }
    BfmForestActor figures[BFM_TOWER_SUMMIT_FIGURES + BFM_SUMMIT_RESIDENTS + 1 + BFM_TOWER_ESCAPE_PROPS];
    size_t count = bfm_tower_summit_figures_actors(summit->figures, figures,
                                                  BFM_TOWER_SUMMIT_FIGURES);
    if (count != BFM_TOWER_SUMMIT_FIGURES) return 0;
    count += bfm_tower_summit_residents_actors(summit->residents, figures + count, BFM_SUMMIT_RESIDENTS);
    if (skill && skill->blade) figures[count++] = *skill->blade;
    if (summit->escaping) figures[0].root = &summit->road_head;
    if (summit->head_relocated) {
        count = summit->head_visible ? 1 : 0;
        count += bfm_tower_escape_props_actors(summit->road, figures + count, BFM_TOWER_ESCAPE_PROPS);
    }

    BfmForestDustCard glow[BFM_SUMMIT_PICKUP_GLOW_CARDS];
    size_t glow_count = 0;
    if (!summit->escaping && pickup && pickup->has_pose && (pickup->visible_parts & (UINT32_C(1) << 25))) {
        glow_count = bfm_tower_summit_pickup_glow(summit->field, pickup->lumina_anchors,
            &model, summit->eye, camera, summit->field_updates, glow, BFM_SUMMIT_PICKUP_GLOW_CARDS);
    }
    if (!bfm_forest_draw_room(summit->field, camera, pose, &model, visible_parts,
                              buffer, blade, figures, count, glow, glow_count, pixels)) return 0;
    if (arrival) {
        const int16_t xyz[3] = {(int16_t)root->translation[0], (int16_t)root->translation[1],
                                (int16_t)root->translation[2]};
        bfm_tower_summit_arrival_draw_light(arrival, camera, xyz, buffer, pixels);
        bfm_tower_summit_arrival_draw(arrival, summit->art, 160, pixels);
    }
    if (skill && skill->award)
        bfm_forest_message_art_draw(summit->art, bfm_forest_message_frame(skill->award), pixels);
    if (summit->lesson.phase)
        bfm_tower_summit_lesson_draw(&summit->lesson, summit->art, pixels);
    return 1;
}
