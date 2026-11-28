/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MINE_LAKE_RESIDENT_ART_H
#define BFM_VILLAGE_MINE_LAKE_RESIDENT_ART_H

#include "bedroom_figure.h"

typedef struct BfmMineLakeResidentArt BfmMineLakeResidentArt;

/* The resident profile supplies the model and idle clip addresses. The
 * departure clip comes from the conversation. Keep the mesh and unpacked
 * character resource alive until the returned art is freed. */
BfmMineLakeResidentArt *bfm_mine_lake_resident_art_load(const uint8_t *scene, size_t bytes,
    unsigned captive_id, const MusashiForestPlayerMesh *model,
    const uint8_t *resource, size_t resource_bytes, uint32_t model_va,
    uint32_t idle_clip_va, uint32_t departure_clip_va);
void bfm_mine_lake_resident_art_free(BfmMineLakeResidentArt *art);

/* pose_tick counts updates in the selected clip. While the departure effect
 * is active, effect_tick counts updates from the start of that effect. */
int bfm_mine_lake_resident_art_draw(const BfmMineLakeResidentArt *art,
    const BfmBedroomRoot *root, int departure_clip, unsigned pose_tick,
    int effect_active, unsigned effect_tick,
    BfmBedroomFigure *figure, const BfmBedroomView *view);

#endif
