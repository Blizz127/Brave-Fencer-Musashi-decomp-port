/* SPDX-License-Identifier: MIT */
#include "woods_lardwick_art.h"
#include "tower_summit_departure.h"
#include "woods_lardwick_motes.h"
#include "woods_fusion_art.h"
#include "woods_resident_glow_art.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>

enum { PARTS = 11, MAX_KEYS = 20, RING_BYTES = 42 };
static const uint32_t origin = UINT32_C(0x80128158);

typedef struct {
    uint32_t address;
    unsigned count;
    const uint8_t *keys[MAX_KEYS];
} SoldierClip;

struct BfmWoodsLardwickArt {
    BfmWoodsResidentGlowArt *crystal;
    MusashiForestPlayerMesh *soldier;
    SoldierClip standing, leaving;
    const uint8_t *departure_ring;
};

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t length)
{
    if (address < origin || address - origin > bytes || length > bytes - (address - origin))
        return NULL;
    return scene + (address - origin);
}

/* Both mounted tapes advance one compact 11-joint pose per field and loop. */
static int load_clip(SoldierClip *clip, const uint8_t *scene, size_t bytes,
                     uint32_t address, unsigned count)
{
    const uint8_t *rows = mounted(scene, bytes, address, 8*(count+1));
    if (!rows || count > MAX_KEYS || u32le(rows + 8*count) ||
        u32le(rows + 8*count + 4) != 0xffff) return 0;
    for (unsigned i = 0; i < count; ++i) {
        uint32_t pose = u32le(rows + 8*i);
        if ((pose >> 24) != 0x81 || (pose & 3u) || u32le(rows + 8*i + 4) != 1) return 0;
        clip->keys[i] = mounted(scene, bytes, pose & ~UINT32_C(0x01000000), 8*PARTS);
        if (!clip->keys[i]) return 0;
    }
    clip->address = address;
    clip->count = count;
    return 1;
}

BfmWoodsLardwickArt *bfm_woods_lardwick_art_load(const uint8_t *scene, size_t bytes,
                                                const BfmWoodsLardwickField *field)
{
    if (!scene || !field || bytes > BFM_FIELD_SCENE_CAPACITY) return NULL;
    BfmWoodsLardwickArt *art = calloc(1, sizeof *art);
    if (!art) return NULL;
    unsigned parts;
    art->crystal = bfm_woods_resident_glow_art_load(scene, bytes);
    art->soldier = musashi_forest_player_mesh_from_table(scene, bytes, origin, field->soldier_model, &parts);
    art->departure_ring = mounted(scene, bytes, UINT32_C(0x8018b360), RING_BYTES);
    if (!art->crystal || !art->soldier || !art->departure_ring || parts != PARTS ||
        !load_clip(&art->standing, scene, bytes, field->soldier_animation, 10) ||
        !load_clip(&art->leaving, scene, bytes, field->soldier_talk_animation, 20)) {
        bfm_woods_lardwick_art_free(art);
        return NULL;
    }
    return art;
}

void bfm_woods_lardwick_art_free(BfmWoodsLardwickArt *art)
{
    if (!art) return;
    bfm_woods_resident_glow_art_free(art->crystal);
    musashi_forest_player_mesh_free(art->soldier);
    free(art);
}

static int draw_soldier(const BfmWoodsLardwickArt *art, BfmBedroomFigure *figure,
                        const BfmBedroomView *view, const BfmWoodsLardwickField *field)
{
    const SoldierClip *clip = field->soldier_animation == art->standing.address ?
                              &art->standing : &art->leaving;
    if (clip->address != field->soldier_animation) return 0;
    BfmForestPose pose;
    unsigned key = field->soldier_pose_age % clip->count;
    if (!bfm_forest_actor_pose_decode(clip->keys[key], 8*PARTS, PARTS, &pose)) return 0;
    BfmForestTransform root;
    bfm_forest_actor_root(field->soldier_yaw, field->root.xyz, &root);
    if (clip == &art->leaving) bfm_tower_summit_departure_hero(field->soldier_pose_age, &root);
    const BfmForestActor body = {.mesh=art->soldier, .pose=&pose, .root=&root,
                                 .visible_parts=(1u << PARTS)-1};
    return bfm_bedroom_figure_add_actor(figure, view, &body, NULL, NULL, 0);
}

static int draw_departure(const BfmWoodsLardwickArt *art, BfmBedroomFigure *figure,
                          const BfmBedroomView *view, const BfmWoodsLardwickField *field,
                          unsigned field_tick)
{
    BfmForestCamera camera;
    if (!bfm_woods_effect_camera(view, &camera)) return 0;
    BfmForestDustCard cards[BFM_TOWER_SUMMIT_DEPARTURE_CARDS];
    size_t count = bfm_departure_ring_cards(art->departure_ring, RING_BYTES, field->root.xyz,
        field->soldier_pose_age, field_tick, &camera, cards, BFM_TOWER_SUMMIT_DEPARTURE_CARDS);
    return (!count || bfm_bedroom_figure_add_fusion(figure, view, NULL, cards, count)) &&
           bfm_woods_lardwick_motes_add(&field->motes, figure, view);
}

int bfm_woods_lardwick_art_add(const BfmWoodsLardwickArt *art, BfmBedroomFigure *figure,
                              const BfmBedroomView *view, const BfmWoodsLardwickField *field,
                              unsigned field_tick)
{
    if (!art || !figure || !view || !field) return 0;
    if (field->crystal_visible &&
        !bfm_woods_resident_glow_art_add(art->crystal, figure, view, field->root.xyz,
            field->root.yaw, field->light_pitch, field->light_yaw, field->flash,
            field->lights_active, field->burst_updates, field->glow_visible)) return 0;
    int leaving = field->soldier_animation == art->leaving.address;
    if (field->soldier_visible && (!leaving || field->soldier_pose_age <= 78) &&
        !draw_soldier(art, figure, view, field)) return 0;
    if (field->glow_scale && !bfm_woods_resident_release_glow_add(art->crystal, figure, view,
            &field->glow_transform, field->glow_scale)) return 0;
    return !leaving || !field->soldier_visible ||
           draw_departure(art, figure, view, field, field_tick);
}
