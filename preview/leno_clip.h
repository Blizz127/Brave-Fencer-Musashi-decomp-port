/* SPDX-License-Identifier: MIT */
#ifndef BFM_LENO_CLIP_H
#define BFM_LENO_CLIP_H

#include "forest_pose.h"
#include "../pc_port/forest_player_bytes.h"

enum { BFM_LENO_PARTS = 12 };
typedef struct {
    const uint8_t *keys;
    unsigned count;
} BfmLenoClip;

/* Both forest visits keep the expanded scene resident while Leno is active. */
static inline const uint8_t *bfm_leno_scene_at(const uint8_t *scene, size_t bytes,
                                               uint32_t address, size_t span)
{
    const uint32_t origin = UINT32_C(0x80128158);
    if (!scene || address < origin || address-origin > bytes || span > bytes-(address-origin))
        return NULL;
    return scene+(address-origin);
}

static inline int bfm_leno_clip_load(BfmLenoClip *clip, const uint8_t *scene, size_t bytes,
                                    uint32_t address)
{
    const uint8_t *keys = bfm_leno_scene_at(scene,bytes,address,8);
    if (!clip || !keys) return 0;
    size_t rows = (bytes-(size_t)(keys-scene))/8;
    for (size_t i = 0; i < rows; ++i) {
        const uint8_t *row = keys+8*i;
        uint32_t key = u32le(row);
        if (!key) {
            if (!i || i > UINT32_MAX || u32le(row+4) != 0xffff) return 0;
            *clip = (BfmLenoClip){keys,(unsigned)i};
            return 1;
        }
        if (key >> 24 != 0x81 || (key & 3) || u32le(row+4) != 1 ||
            !bfm_leno_scene_at(scene,bytes,key & ~UINT32_C(0x01000000),8*BFM_LENO_PARTS))
            return 0;
    }
    return 0;
}

static inline int bfm_leno_clip_pose(const BfmLenoClip *clip, const uint8_t *scene, size_t bytes,
                                    unsigned tick, BfmForestPose *pose)
{
    if (!clip || !clip->keys || !clip->count) return 0;
    uint32_t key = u32le(clip->keys+8*(tick%clip->count)) & ~UINT32_C(0x01000000);
    const uint8_t *frame = bfm_leno_scene_at(scene,bytes,key,8*BFM_LENO_PARTS);
    return bfm_forest_actor_pose_decode(frame,8*BFM_LENO_PARTS,BFM_LENO_PARTS,pose);
}

#endif
