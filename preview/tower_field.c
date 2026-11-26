/* SPDX-License-Identifier: MIT */
#include "tower_field.h"
#include "tower_camera.h"
#include "tower_figures.h"
#include "tower_sky.h"
#include "tower_lightning.h"

#include <stdlib.h>

struct BfmTowerField {
    BfmForestField *field;
    BfmTowerCamera camera;
    BfmTowerFigures *figures;
    BfmTowerSky *sky;
    BfmTowerCircuit *circuit;
    unsigned entrance;
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
    if (scene && door && bfm_tower_camera_load(&tower->camera, door, 36))
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

void bfm_tower_field_tick(BfmTowerField *tower, const int16_t player[3], BfmGameplayRun *gameplay)
{
    if (!tower || !player || !gameplay) return;
    bfm_tower_camera_tick(&tower->camera, player);
    /* All four registered electrodes are intact on this exterior approach. */
    bfm_tower_lightning_tick(tower->circuit, gameplay, 0);
}

int bfm_tower_field_camera(const BfmTowerField *tower, const int16_t player[3],
                           BfmForestCamera *out, int32_t eye[3])
{
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
    size_t count = bfm_tower_figures_actors(tower->figures, field_tick, actors, BFM_TOWER_FIGURE_ACTORS);
    size_t arcs = bfm_tower_lightning_cards(tower->circuit, 2, camera, cards, BFM_TOWER_LIGHTNING_CARDS);
    size_t glows = bfm_tower_figures_cards(tower->figures, field_tick, 15, field_tick & 1u,
                                          camera, cards + arcs, BFM_TOWER_FIGURE_CARDS);
    return bfm_forest_draw_room(tower->field, camera, pose, root, visible_parts, field_tick & 1u, blade,
                                actors, count, cards, arcs + glows, pixels);
}
