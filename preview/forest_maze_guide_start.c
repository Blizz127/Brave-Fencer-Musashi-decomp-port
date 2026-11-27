/* SPDX-License-Identifier: MIT */
#include "forest_maze_guide_start.h"
#include "leno_clip.h"
#include <stdlib.h>

enum { ONE = 65536, BANK_DOG = 0x7b, CALLING_DISTANCE = 256 };
enum GuideAction { APPEARING, WAITING, LEADING, RETURNING, GONE };

struct BfmForestMazeGuideStart {
    const uint8_t *scene;
    size_t bytes;
    MusashiForestPlayerMesh *mesh;
    BfmLenoClip wait, run;
    enum GuideAction action;
    unsigned tick, yaw;
    int visible;
    int32_t speed;
    int64_t position[3];
};

static int read_placement(BfmForestMazeGuideStart *guide)
{
    const uint8_t *scene = guide->scene;
    size_t bytes = guide->bytes;
    const uint8_t *room = bfm_leno_scene_at(scene,bytes,UINT32_C(0x801910e4),0xbc);
    if (!room) return 0;
    uint32_t first = u32le(room+0xb4), limit = u32le(room+0xb8);
    if (limit < first || (limit-first)%20) return 0;
    const uint8_t *rows = bfm_leno_scene_at(scene,bytes,first,limit-first);
    if (!rows) return 0;
    for (size_t i=0; i+20 <= limit-first; i+=20) {
        const uint8_t *actor = rows+i;
        unsigned kind = u16le(actor+6);
        if (!kind) break;
        if (kind != BANK_DOG) continue;
        for (unsigned k=0; k<3; ++k)
            guide->position[k] = (int64_t)s16le(actor+2*k)*ONE;
        guide->yaw = u16le(actor+12) & 4095u;
        return 1;
    }
    return 0;
}

BfmForestMazeGuideStart *bfm_forest_maze_guide_start_open(const uint8_t *scene, size_t bytes)
{
    const uint8_t *stride = bfm_leno_scene_at(scene,bytes,UINT32_C(0x80190818),12);
    if (!stride || u32le(stride) || u32le(stride+4)) return NULL;
    BfmForestMazeGuideStart *guide = calloc(1,sizeof *guide);
    if (!guide) return NULL;
    guide->scene = scene;
    guide->bytes = bytes;
    guide->speed = (int32_t)u32le(stride+8);
    unsigned parts;
    guide->mesh = musashi_forest_player_mesh_from_table(scene,bytes,UINT32_C(0x80128158),
                                                        UINT32_C(0x801bd274),&parts);
    if (!guide->mesh || parts != BFM_LENO_PARTS || !read_placement(guide) ||
        !bfm_leno_clip_load(&guide->wait,scene,bytes,UINT32_C(0x801bae98)) ||
        !bfm_leno_clip_load(&guide->run,scene,bytes,UINT32_C(0x801baef0))) {
        bfm_forest_maze_guide_start_free(guide);
        return NULL;
    }
    return guide;
}

void bfm_forest_maze_guide_start_free(BfmForestMazeGuideStart *guide)
{
    if (guide) {
        musashi_forest_player_mesh_free(guide->mesh);
        free(guide);
    }
}

static void foot(const BfmForestMazeGuideStart *guide, int16_t xyz[3])
{
    for (unsigned k=0; k<3; ++k) {
        int64_t q = guide->position[k];
        xyz[k] = (int16_t)(q/ONE - (q%ONE < 0));
    }
}

static int within_calling_distance(const BfmForestMazeGuideStart *guide, const int16_t player[3])
{
    int16_t xyz[3];
    foot(guide,xyz);
    int64_t squared = 0;
    for (unsigned k=0; k<3; ++k) {
        int distance = player[k]-xyz[k];
        squared += (int64_t)distance*distance;
    }
    return squared <= CALLING_DISTANCE*CALLING_DISTANCE;
}

uint8_t bfm_forest_maze_guide_start_step(BfmForestMazeGuideStart *guide,
                                        const int16_t player[3], uint8_t quest)
{
    if (guide->action == GONE) return quest;
    ++guide->tick;
    switch (guide->action) {
    case APPEARING:
        guide->action = WAITING;
        guide->tick = 0;
        break;
    case WAITING:
        if (quest == 1) {
            guide->visible = 1;
            if (within_calling_distance(guide,player)) {
                guide->action = LEADING;
                guide->tick = 0;
            }
        } else if (quest == 15) {
            guide->visible = 1;
            guide->position[2] -= 256*ONE;
            guide->action = RETURNING;
            guide->tick = 0;
        }
        break;
    case LEADING: {
        int16_t xyz[3];
        BfmForestTransform heading;
        foot(guide,xyz);
        /* The field moves in the previous pose's heading before turning. */
        bfm_forest_actor_root(guide->yaw,xyz,&heading);
        for (unsigned k=0; k<3; k+=2)
            guide->position[k] += (int64_t)guide->speed*heading.rotation[k][2]/4096;
        if (guide->yaw < 2048) guide->yaw += 256;
        if (guide->position[2] >= (int64_t)4171*ONE) {
            guide->action = GONE;
            return 2;
        }
        break;
    }
    case RETURNING:
        guide->position[2] -= 16*ONE;
        if (guide->position[2] < (int64_t)837*ONE) {
            guide->action = GONE;
            return 16;
        }
        break;
    case GONE: break;
    }
    return quest;
}

int bfm_forest_maze_guide_start_draw(const BfmForestMazeGuideStart *guide,
                                    BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    if (!guide->visible || guide->action == GONE) return 1;
    BfmForestPose pose;
    const BfmLenoClip *clip = guide->action == WAITING ? &guide->wait : &guide->run;
    if (!bfm_leno_clip_pose(clip,guide->scene,guide->bytes,guide->tick,&pose)) return 0;
    int16_t xyz[3];
    BfmForestTransform root;
    foot(guide,xyz);
    bfm_forest_actor_root(guide->yaw,xyz,&root);
    BfmForestActor actor = {.mesh=guide->mesh,.pose=&pose,.root=&root,
                            .visible_parts=(1u<<BFM_LENO_PARTS)-1};
    return bfm_bedroom_figure_add_actor(figure,view,&actor,NULL,NULL,0);
}
