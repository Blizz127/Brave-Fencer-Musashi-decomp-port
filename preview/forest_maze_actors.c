/* SPDX-License-Identifier: MIT */
#include "forest_maze_actors.h"
#include "bedroom_field.h"
#include "forest_maze_dog.h"
#include "leno_clip.h"
#include "forest_guard_contact.h"
#include "../pc_port/forest_player_bytes.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static const uint32_t origin = UINT32_C(0x80128158);

struct BfmForestMazeActors {
    const uint8_t *scene;
    size_t bytes;
    BfmMazeDog *guide;
    const uint8_t *guide_shadow;
    BfmLenoClip clips[BFM_MAZE_DOG_CLIPS];
    MusashiForestPlayerMesh *guide_mesh, *sign_mesh;
    BfmForestPose guide_pose, sign_pose;
    BfmForestTransform guide_root, sign_root;
    BfmForestActor drawn[2];
    BfmForestGuardBox sign_box;
    size_t sign_message;
};

static const uint8_t *mounted(const BfmForestMazeActors *actors, uint32_t address, size_t bytes)
{
    if (address < origin || address-origin > actors->bytes ||
        bytes > actors->bytes-(address-origin)) return NULL;
    return actors->scene+(address-origin);
}

static MusashiForestPlayerMesh *load_mesh(const BfmForestMazeActors *actors,
                                         const uint8_t *profile, unsigned wanted)
{
    unsigned parts;
    MusashiForestPlayerMesh *mesh = musashi_forest_player_mesh_from_table(
        actors->scene,actors->bytes,origin,u32le(profile+4),&parts);
    if (mesh && parts != wanted) {
        musashi_forest_player_mesh_free(mesh);
        return NULL;
    }
    return mesh;
}

static int guide_art(BfmForestMazeActors *actors)
{
    const uint8_t *profile = mounted(actors,UINT32_C(0x8018bfe0),12);
    if (!profile) return 0;
    actors->guide_mesh = load_mesh(actors,profile,BFM_LENO_PARTS);
    actors->guide_shadow = mounted(actors,UINT32_C(0x80188cf0),12);
    if (!actors->guide_mesh || !actors->guide_shadow) return 0;
    static const uint32_t motions[BFM_MAZE_DOG_CLIPS] = {
        [BFM_MAZE_DOG_RUN] = 0x801b101c, [BFM_MAZE_DOG_BRAKE] = 0x801b0cfc,
        [BFM_MAZE_DOG_SEARCH] = 0x801b0ed4, [BFM_MAZE_DOG_BARK] = 0x801b0da4,
        [BFM_MAZE_DOG_REST] = 0x801b0dd4
    };
    for (unsigned i = 0; i < BFM_MAZE_DOG_CLIPS; ++i)
        if (!bfm_leno_clip_load(&actors->clips[i],actors->scene,actors->bytes,motions[i])) return 0;
    return 1;
}

static int load_sign(BfmForestMazeActors *actors, const int16_t xyz[3], unsigned yaw)
{
    const uint8_t *profile = mounted(actors,UINT32_C(0x8018c104),12);
    const uint8_t *script = mounted(actors,UINT32_C(0x8018c138),24);
    if (!profile || !script || u32le(script) != 10 || u32le(script+8) != 1 ||
        u32le(script+16) != 2) return 0;
    const uint8_t *box = mounted(actors,u32le(profile+8),16);
    const uint8_t *message = mounted(actors,u32le(script+12),4);
    if (!box || u32le(box) || !message) return 0;
    actors->sign_message = (size_t)(message-actors->scene);
    actors->sign_mesh = load_mesh(actors,profile,1);
    if (!actors->sign_mesh) return 0;
    for (unsigned k = 0; k < 3; ++k) {
        actors->sign_box.low[k] = s16le(box+4+4*k);
        actors->sign_box.high[k] = s16le(box+6+4*k);
        if (actors->sign_box.low[k] > actors->sign_box.high[k]) return 0;
        actors->sign_pose.local[0].rotation[k][k] = 4096;
    }
    bfm_forest_actor_root(yaw,xyz,&actors->sign_root);
    return 1;
}

BfmForestMazeActors *bfm_forest_maze_actors_open(const uint8_t *scene, size_t bytes,
    unsigned entrance, const BfmVillageGround *ground, const BfmBedroomRoot *player,
    const BfmSaveRecord *save)
{
    if (!scene || !ground || !player || !save || (entrance != 0 && entrance != 33) ||
        bytes > BFM_FIELD_SCENE_CAPACITY) return NULL;
    BfmForestMazeActors *actors = calloc(1,sizeof *actors);
    if (!actors) return NULL;
    actors->scene = scene;
    actors->bytes = bytes;
    const uint8_t *room = mounted(actors,UINT32_C(0x8018c890),0xbc);
    if (!room) goto fail;
    uint32_t first = u32le(room+0xb4), dynamic = u32le(room+0xb8);
    if (entrance == 33) {
        /* This arrival callback installs its own fixed and proximity lists. */
        first = UINT32_C(0x8018ce04);
        dynamic = UINT32_C(0x8018ce90);
    }
    if (dynamic <= first || (dynamic-first)%20) goto fail;
    const uint8_t *fixed = mounted(actors,first,dynamic-first);
    const uint8_t *proximity = mounted(actors,dynamic,20);
    if (!fixed || !proximity || u16le(proximity+6)) goto fail;
    for (size_t off = 0; off < dynamic-first; off += 20) {
        const uint8_t *row = fixed+off;
        unsigned kind = u16le(row+6);
        if (!kind) return actors;
        const int16_t xyz[3] = {s16le(row),s16le(row+2),s16le(row+4)};
        if (kind == 118) {
            if (actors->guide) goto fail;
            actors->guide = bfm_maze_dog_open(scene,bytes,ground,xyz,player,save);
            if (!actors->guide || (bfm_maze_dog_view(actors->guide) && !guide_art(actors)))
                goto fail;
        } else if (kind == 0x3d2 && u16le(row+8) && u32le(save->progress+8) < 900) {
            /* The first fork's variant-zero sign is retired during initialization. */
            if (actors->sign_mesh || !load_sign(actors,xyz,u16le(row+12))) goto fail;
        }
    }
fail:
    bfm_forest_maze_actors_free(actors);
    return NULL;
}

void bfm_forest_maze_actors_free(BfmForestMazeActors *actors)
{
    if (!actors) return;
    bfm_maze_dog_free(actors->guide);
    musashi_forest_player_mesh_free(actors->guide_mesh);
    musashi_forest_player_mesh_free(actors->sign_mesh);
    free(actors);
}

unsigned bfm_forest_maze_actors_step(BfmForestMazeActors *actors, const BfmBedroomRoot *player,
                                     BfmSaveRecord *save, BfmGameplayRun *run)
{
    return actors && actors->guide ? bfm_maze_dog_step(actors->guide,player,save,run) : 0;
}

const BfmForestActor *bfm_forest_maze_actors_draw(BfmForestMazeActors *actors, size_t *count)
{
    if (!count) return NULL;
    *count = SIZE_MAX;
    if (!actors) return NULL;
    size_t n = 0;
    const BfmMazeDogView *dog = bfm_maze_dog_view(actors->guide);
    if (dog) {
        if ((unsigned)dog->clip >= BFM_MAZE_DOG_CLIPS) return NULL;
        if (!bfm_leno_clip_pose(&actors->clips[dog->clip],actors->scene,actors->bytes,
                                dog->tick,&actors->guide_pose)) return NULL;
        bfm_forest_actor_root(dog->yaw,dog->xyz,&actors->guide_root);
        actors->drawn[n++] = (BfmForestActor){actors->guide_mesh,&actors->guide_pose,
                                               &actors->guide_root,(1u<<BFM_LENO_PARTS)-1u,NULL};
    }
    if (actors->sign_mesh)
        actors->drawn[n++] = (BfmForestActor){actors->sign_mesh,&actors->sign_pose,
                                              &actors->sign_root,1,NULL};
    *count = n;
    return actors->drawn;
}

const uint8_t *bfm_forest_maze_actor_shadow(const BfmForestMazeActors *actors,
                                          size_t actor_index, BfmBedroomRoot *ground,
                                          unsigned *size)
{
    if (!actors || actor_index != 0 || !ground || !size) return NULL;
    const BfmMazeDogView *dog = bfm_maze_dog_view(actors->guide);
    if (!dog) return NULL;
    *ground = (BfmBedroomRoot){.yaw=dog->yaw};
    memcpy(ground->xyz,dog->xyz,sizeof ground->xyz);
    if (!bfm_bedroom_travel_air_shadow(actors->scene,actors->bytes,UINT32_C(0x8018c890),ground) ||
        !ground->shadow_visible) return NULL;
    *size = 8192;
    return actors->guide_shadow;
}

int bfm_forest_maze_sign_slide(const BfmForestMazeActors *actors,
                              const int16_t before[3], int16_t endpoint[3])
{
    if (!actors || !actors->sign_mesh || !before || !endpoint ||
        before[1] < INT16_MIN+16 || endpoint[1] < INT16_MIN+16) return 0;
    const int16_t from[3] = {before[0],before[1]-16,before[2]};
    const int16_t to[3] = {endpoint[0],endpoint[1]-16,endpoint[2]};
    int16_t hit[3];
    if (!bfm_forest_body_slide(&actors->sign_box,&actors->sign_root,from,to,hit) ||
        hit[1] > INT16_MAX-16) return 0;
    memcpy(endpoint,hit,sizeof hit);
    endpoint[1] += 16;
    return 1;
}

int bfm_forest_maze_sign_message(const BfmForestMazeActors *actors,
                                 const int16_t player[3], size_t *message_offset)
{
    if (!actors || !actors->sign_mesh || !player || !message_offset) return 0;
    int64_t distance = 0;
    for (unsigned k = 0; k < 3; ++k) {
        int64_t delta = (int32_t)player[k]-actors->sign_root.translation[k];
        distance += delta*delta;
    }
    if (distance > 4096) return 0;
    *message_offset = actors->sign_message;
    return 1;
}
