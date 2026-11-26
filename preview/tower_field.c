/* SPDX-License-Identifier: MIT */
#include "tower_field.h"
#include "tower_rise.h"
#include "forest_shore_motion.h"
#include "tower_camera.h"
#include "tower_pylons.h"
#include "tower_figures.h"
#include "tower_sky.h"
#include "tower_lightning.h"

#include <stdlib.h>

struct BfmTowerField {
    BfmForestField *field;
    BfmTowerCamera camera;
    BfmTowerPylons pylons;
    BfmTowerFigures *figures;
    BfmTowerSky *sky;
    BfmTowerCircuit *circuit;
    unsigned entrance, rise_age, submitted_rise[3], face_yaw[3];
};

void bfm_tower_field_free(BfmTowerField *tower)
{
    if (tower) {
        bfm_tower_lightning_free(tower->circuit);
        bfm_tower_sky_free(tower->sky);
        bfm_tower_figures_free(tower->figures);
        bfm_forest_field_free(tower->field);
        free(tower);
    }
}

BfmTowerField *bfm_tower_field_open(const MusashiDiscMedia *disc,
                                    const RoomTransferDescriptor *entry)
{
    if (!disc || !entry || entry->room != 0x3009 || entry->world != 0 || entry->parameter != 0)
        return NULL;
    BfmTowerField *tower = calloc(1, sizeof *tower);
    if (!tower) return NULL;
    tower->entrance = entry->parameter;
    tower->field = bfm_forest_field_open(disc, entry->room);
    size_t bytes = 0;
    const uint8_t *scene = bfm_forest_field_scene(tower->field, &bytes);
    const uint8_t *door = bfm_forest_field_entrance(tower->field, tower->entrance);
    if (scene && door && bfm_tower_pylons_load(&tower->pylons, scene, bytes) &&
        bfm_tower_camera_load(&tower->camera, door, 36))
        tower->figures = bfm_tower_figures_open(scene, bytes);
    if (tower->figures) {
        tower->sky = bfm_tower_sky_open(scene, bytes, bfm_forest_field_tiles(tower->field));
        tower->circuit = bfm_tower_lightning_load(scene, bytes);
    }
    if (!tower->figures || !tower->sky || !tower->circuit) {
        bfm_tower_field_free(tower);
        return NULL;
    }
    return tower;
}

BfmForestField *bfm_tower_field_room(BfmTowerField *tower)
{
    return tower ? tower->field : NULL;
}

int bfm_tower_field_place(BfmTowerField *tower, ForestPlayerState *player,
                          int16_t *model_yaw, int16_t cached[3])
{
    return tower && bfm_forest_place_player(tower->field, tower->entrance, player, model_yaw, cached);
}

void bfm_tower_field_tick(BfmTowerField *tower, const int16_t player[3], int facing, BfmGameplayRun *gameplay)
{
    if (!tower || !player || !gameplay) return;
    /* The central body updates before the four plate actors. */
    if (tower->pylons.credits == BFM_TOWER_POSTS && tower->rise_age < BFM_TOWER_RELEASE)
        ++tower->rise_age;
    tower->submitted_rise[tower->pylons.next_frame] = tower->rise_age;
    /* The room's center is at the world origin. The stone head turns its
     * forward (-Z) axis toward the player, separately from the tower body. */
    tower->face_yaw[tower->pylons.next_frame] = bfm_forest_shore_bearing(-player[0], -player[2]);
    bfm_tower_pylons_tick(&tower->pylons, player, gameplay);
    bfm_tower_camera_plate_hint(&tower->camera, bfm_tower_field_plate_note(tower));
    if (tower->rise_age >= BFM_TOWER_CAMERA_CUT && tower->rise_age < BFM_TOWER_RELEASE)
        tower->camera.desired_pitch = 288;
    else if (tower->rise_age == BFM_TOWER_RELEASE)
        tower->camera.desired_pitch = 0;
    bfm_tower_camera_tick(&tower->camera, player);
    bfm_tower_camera_rise_tick(&tower->camera, tower->rise_age, facing);
    bfm_tower_lightning_tick(tower->circuit, gameplay, bfm_tower_pylons_broken(&tower->pylons));
}

int bfm_tower_field_blade_contact(BfmTowerField *tower, unsigned stroke_id,
                                  const int16_t start[3], const int16_t end[3],
                                  int16_t contact[3], int16_t normal[3])
{
    return tower && bfm_tower_pylons_hit(&tower->pylons, stroke_id, start, end, contact, normal);
}

int bfm_tower_field_body_slide(const BfmTowerField *tower, const int16_t from[3],
                               const int16_t to[3], int16_t contact[3])
{
    return tower && bfm_tower_pylons_slide(&tower->pylons, from, to, contact);
}

int bfm_tower_field_plate_note(const BfmTowerField *tower)
{
    return tower && tower->pylons.note_started && !tower->pylons.note_closed;
}

void bfm_tower_field_close_plate_note(BfmTowerField *tower)
{
    if (bfm_tower_field_plate_note(tower)) tower->pylons.note_closed = 1;
}

int bfm_tower_field_revealing(const BfmTowerField *tower)
{
    return tower && tower->pylons.credits == BFM_TOWER_POSTS && tower->rise_age < BFM_TOWER_RELEASE;
}

unsigned bfm_tower_field_rise_phase(const BfmTowerField *tower)
{
    unsigned age = tower ? tower->rise_age : 0;
    if (age >= BFM_TOWER_RELEASE) return 4;
    if (age >= BFM_TOWER_RISE_END) return 3;
    if (age >= BFM_TOWER_RISE_START) return 2;
    return age >= BFM_TOWER_POOL_DRAINS;
}

int bfm_tower_field_raised(const BfmTowerField *tower)
{
    return tower && tower->rise_age == BFM_TOWER_RELEASE;
}

int bfm_tower_field_plate_pressing(const BfmTowerField *tower)
{
    return tower && tower->pylons.pressing;
}

unsigned bfm_tower_field_plate_count(const BfmTowerField *tower)
{
    return tower ? tower->pylons.credits : 0;
}

int bfm_tower_field_camera(const BfmTowerField *tower, const int16_t player[3],
                           BfmForestCamera *out, int32_t eye[3])
{
    if (tower && tower->rise_age >= BFM_TOWER_CAMERA_CUT)
        return bfm_tower_camera_rise_view(&tower->camera, player, tower->rise_age, out, eye);
    return tower && bfm_tower_camera_view(&tower->camera, player, out, eye);
}

int bfm_tower_field_draw(BfmTowerField *tower, const BfmForestCamera *camera,
                         const BfmForestPose *pose, const BfmForestTransform *root,
                         uint32_t visible_parts, unsigned field_tick, const BfmForestBladeFrame *blade,
                         uint32_t pixels[240][320])
{
    if (!tower || !camera || !pose || !root || !pixels ||
        !bfm_tower_sky_draw(tower->sky, camera, pixels))
        return 0;
    BfmForestActor actors[BFM_TOWER_FIGURE_ACTORS];
    BfmForestDustCard cards[BFM_TOWER_LIGHTNING_CARDS + BFM_TOWER_FIGURE_CARDS];
    const BfmTowerPylonFrame *posts = tower->pylons.history[field_tick % 3];
    size_t count = bfm_tower_figures_actors(tower->figures, field_tick, posts,
                                            tower->submitted_rise[field_tick % 3], tower->face_yaw[field_tick % 3], actors, BFM_TOWER_FIGURE_ACTORS);
    /* The risen static mesh now supplies the moving body's stonework.
     * Its head and the other room figures remain separate submissions. */
    if (count && tower->rise_age >= BFM_TOWER_RISE_END)
        actors[0].visible_parts = 0;
    size_t arcs = bfm_tower_lightning_cards(tower->circuit, 2, camera, cards, BFM_TOWER_LIGHTNING_CARDS);
    size_t glows = bfm_tower_figures_cards(tower->figures, field_tick, posts, field_tick & 1u,
                                          tower->submitted_rise[field_tick % 3], camera, cards + arcs, BFM_TOWER_FIGURE_CARDS);
    return bfm_forest_draw_room(tower->field, camera, pose, root, visible_parts, field_tick & 1u, blade,
                                actors, count, cards, arcs + glows, pixels);
}
