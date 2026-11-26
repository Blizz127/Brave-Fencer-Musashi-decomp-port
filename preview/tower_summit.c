/* SPDX-License-Identifier: MIT */
#include "tower_summit.h"
#include "tower_sky.h"
#include "tower_summit_figures.h"

#include <stdlib.h>

struct BfmTowerSummit {
    BfmForestField *field;
    BfmTowerSummitFigures *figures;
    BfmTowerSky *sky;
    const MusashiDiscMedia *disc;
    BfmTowerSummitArrival arrival;
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
        summit->sky = bfm_tower_summit_sky_open(scene, size, bfm_forest_field_tiles(summit->field));
    }
    if (!summit->figures || !summit->sky) {
        bfm_tower_summit_free(summit);
        return NULL;
    }
    return summit;
}

void bfm_tower_summit_free(BfmTowerSummit *summit)
{
    if (summit) {
        bfm_tower_summit_arrival_close(&summit->arrival);
        bfm_forest_message_art_free(summit->art);
        bfm_tower_summit_figures_free(summit->figures);
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

const BfmTowerSummitArrival *bfm_tower_summit_arriving(const BfmTowerSummit *summit)
{
    return summit && summit->arrival_active ? &summit->arrival : NULL;
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
    BfmForestActor figures[BFM_TOWER_SUMMIT_FIGURES];
    size_t count = bfm_tower_summit_figures_actors(summit->figures, figures,
                                                  BFM_TOWER_SUMMIT_FIGURES);
    if (count != BFM_TOWER_SUMMIT_FIGURES ||
        !bfm_forest_draw_room(summit->field, camera, pose, &model, visible_parts,
                              buffer, blade, figures, count, NULL, 0, pixels)) return 0;
    if (arrival) {
        const int16_t xyz[3] = {(int16_t)root->translation[0], (int16_t)root->translation[1],
                                (int16_t)root->translation[2]};
        bfm_tower_summit_arrival_draw_light(arrival, camera, xyz, buffer, pixels);
        bfm_tower_summit_arrival_draw(arrival, summit->art, 160, pixels);
    }
    return 1;
}
