/* SPDX-License-Identifier: MIT */
#include "forest_maze.h"
#include "bedroom_field.h"
#include "forest_maze_actors.h"
#include "forest_maze_drift.h"
#include "forest_maze_motion.h"
#include "forest_maze_view.h"
#include "forest_message_art.h"
#include "village_clock.h"
#include "steam_knight_charge_effect.h"

#include <stdlib.h>
#include <string.h>

struct BfmForestMaze {
    uint8_t scene[BFM_FIELD_SCENE_CAPACITY];
    size_t bytes;
    BfmVillageMotion *motion;
    BfmForestMazeView *view;
    BfmForestMazeActors *actors;
    BfmForestMazeMotion *action;
    BfmForestMessageArt *writing;
    BfmForestDrift drift;
    unsigned arrival_sound, actor_sound, player_sound;
    int sign_open;
};

void bfm_forest_maze_free(BfmForestMaze *maze)
{
    if (!maze) return;
    bfm_forest_message_art_free(maze->writing);
    bfm_forest_maze_actors_free(maze->actors);
    bfm_forest_maze_motion_free(maze->action);
    bfm_forest_maze_view_free(maze->view);
    bfm_village_motion_free(maze->motion);
    free(maze);
}

BfmForestMaze *bfm_forest_maze_open(const MusashiDiscMedia *disc, const uint8_t *main,
                                    size_t main_bytes, BfmSaveRecord *incoming)
{
    if (!disc || !main || !incoming) return NULL;
    BfmForestMaze *maze = calloc(1, sizeof *maze);
    if (!maze) return NULL;
    maze->bytes = bfm_read_archive_field_scene(disc,"SC03.CD",7,maze->scene);
    unsigned entrance = incoming->destination[2];
    maze->motion = bfm_village_motion_maze(maze->scene,maze->bytes,main,main_bytes,entrance,disc);
    if (!maze->motion) goto fail;
    maze->view = bfm_forest_maze_view_open(disc,maze->scene,maze->bytes,entrance);
    if (!maze->view || !bfm_forest_maze_view_daylight(maze->view,incoming->progress[0x37]) ||
        !bfm_forest_maze_drift_load(&maze->drift,maze->scene,maze->bytes,entrance)) goto fail;
    const BfmVillagePlayer *player = bfm_village_motion_player(maze->motion);
    maze->actors = bfm_forest_maze_actors_open(maze->scene,maze->bytes,entrance,
                      bfm_village_motion_ground(maze->motion),&player->root,incoming);
    maze->action = bfm_forest_maze_motion_open(disc,maze->scene,maze->bytes);
    if (!maze->actors || !maze->action) goto fail;
    if (entrance == 33) {
        maze->writing = bfm_castle_message_art_load(disc,maze->scene,maze->bytes,
                          bfm_forest_maze_view_tiles(maze->view),0x5f950);
        if (!maze->writing) goto fail;
    }
    maze->arrival_sound = bfm_forest_maze_gate_arrive(bfm_village_motion_maze_gate(maze->motion),
                                                      incoming->flags);
    return maze;
fail:
    bfm_forest_maze_free(maze);
    return NULL;
}

BfmVillageMotion *bfm_forest_maze_motion(BfmForestMaze *maze)
{
    return maze ? maze->motion : NULL;
}

void bfm_forest_maze_close_sign(BfmForestMaze *maze)
{
    if (maze) maze->sign_open = 0;
}

static int step_actors(BfmForestMaze *maze, BfmGameplayRun *run, BfmSaveRecord *save,
                       int camera_pressed)
{
    const BfmVillagePlayer *player = bfm_village_motion_player(maze->motion);
    bfm_forest_drift_start(&maze->drift,run);
    maze->actor_sound = bfm_forest_maze_actors_step(maze->actors,&player->root,save,run);
    bfm_forest_drift_tick(&maze->drift,run);
    int16_t target[3];
    memcpy(target,player->root.xyz,sizeof target);
    if (!bfm_village_motion_transfer(maze->motion))
        target[1] = bfm_forest_maze_tracking_y(maze->action,target[1]);
    bfm_forest_maze_view_step(maze->view,target,camera_pressed);
    bfm_forest_maze_motion_effects(maze->action,bfm_forest_maze_view_camera(maze->view));
    return bfm_forest_maze_view_daylight(maze->view,save->progress[0x37]);
}

int bfm_forest_maze_departing(BfmForestMaze *maze, BfmGameplayRun *run, BfmSaveRecord *save)
{
    return bfm_village_motion_maze_departing(maze->motion) && step_actors(maze,run,save,0);
}

int bfm_forest_maze_step(BfmForestMaze *maze, BfmFieldInput input, BfmGameplayRun *run,
                         BfmSaveRecord *save, BfmForestMessage **opened)
{
    const BfmVillagePlayer *player = bfm_village_motion_player(maze->motion);
    int16_t before[3];
    memcpy(before,player->root.xyz,sizeof before);
    if (!maze->sign_open) {
        bfm_village_clock_step(save,0);
        if (run) memcpy(run->player.record,save->progress,sizeof run->player.record);
        const BfmVillageCamera *camera = bfm_forest_maze_view_camera(maze->view);
        if (!bfm_forest_maze_motion_step(maze->action,maze->motion,run,input,
                              (camera->yaw+2048)&4095,&maze->player_sound)) return 0;
        if (run) memcpy(save->progress,run->player.record,sizeof save->progress);
        if (bfm_village_motion_transfer(maze->motion)) return 1;
        int16_t endpoint[3];
        memcpy(endpoint,player->root.xyz,sizeof endpoint);
        if (bfm_forest_maze_sign_slide(maze->actors,before,endpoint) &&
            !bfm_village_motion_maze_contact(maze->motion,endpoint)) return 0;
        size_t text;
        if (opened && input.square_pressed && player->controlled &&
            bfm_forest_maze_sign_message(maze->actors,player->root.xyz,&text)) {
            *opened = bfm_field_message_open(maze->scene+text,maze->bytes-text);
            if (!*opened) return 0;
            maze->sign_open = 1;
        }
    }
    return step_actors(maze,run,save,input.camera_pressed);
}

unsigned bfm_forest_maze_take_sound(BfmForestMaze *maze)
{
    unsigned *pending = &maze->actor_sound;
    if (maze->player_sound) pending = &maze->player_sound;
    if (maze->arrival_sound) pending = &maze->arrival_sound;
    unsigned sound = *pending;
    *pending = 0;
    return sound;
}

int bfm_forest_maze_draw(BfmForestMaze *maze, const uint8_t flags[64],
                         const BfmForestMessageFrame *window, uint32_t pixels[240][320])
{
    const BfmVillagePlayer *player = bfm_village_motion_player(maze->motion);
    size_t count;
    const BfmForestActor *actors = bfm_forest_maze_actors_draw(maze->actors,&count);
    /* Only the guide and the sign have opaque models in this fork. */
    BfmForestMazeShadow shadows[2] = {0};
    BfmBedroomRoot ground[2];
    if (count > 2) return 0;
    for (size_t i = 0; i < count; ++i) {
        shadows[i].descriptor = bfm_forest_maze_actor_shadow(maze->actors,i,&ground[i],
                                                              &shadows[i].size);
        shadows[i].ground = shadows[i].descriptor ? &ground[i] : NULL;
    }
    BfmForestDustCard drift[BFM_FOREST_MAZE_DRIFT_CARDS + BFM_STEAM_CHARGE_SLOTS];
    unsigned cards = bfm_forest_maze_drift_project(&maze->drift,drift);
    cards += bfm_forest_maze_motion_cards(maze->action,bfm_forest_maze_view_camera(maze->view),
                                         drift+cards,BFM_STEAM_CHARGE_SLOTS);
    if (!bfm_forest_maze_view_draw_room(maze->view,&player->root,&player->pose,flags,
                                        actors,shadows,count,drift,cards,pixels)) return 0;
    if (window && maze->writing) bfm_forest_message_art_draw(maze->writing,window,pixels);
    return 1;
}
