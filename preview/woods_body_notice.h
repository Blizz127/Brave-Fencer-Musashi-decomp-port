/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_BODY_NOTICE_H
#define BFM_WOODS_BODY_NOTICE_H

#include "musashi_disc_media.h"

typedef struct {
    uint16_t prefix[8];
    int16_t height;
    uint8_t palette, count, frame, delta;
} BfmWoodsRankLabel;

typedef struct {
    BfmWoodsRankLabel body, fusion, lumina;
    uint16_t digits[10];
    int16_t slide[6];
    uint8_t pulse[4];
} BfmWoodsBodyNotice;

/* Load once from the US southern Woods field and its mounted executable.
 * The notice retains the decoded layout and borrows no room storage. */
int bfm_woods_body_notice_load(BfmWoodsBodyNotice *notice, const MusashiDiscMedia *disc,
                              const uint8_t *scene, size_t scene_bytes);
/* Start once on a Body rank gain of 1 to 9. Draw that first update before
 * stepping. Step once per subsequent 30 Hz field update. */
void bfm_woods_body_notice_start(BfmWoodsBodyNotice *notice, unsigned delta);
/* A single contact can increase both weapons. Each nonzero gain restarts
 * that weapon's own display, independently of Body and the other weapon. */
void bfm_woods_weapon_notice_start(BfmWoodsBodyNotice *notice, unsigned lumina_delta,
                                   unsigned fusion_delta);
void bfm_woods_body_notice_step(BfmWoodsBodyNotice *notice);
void bfm_woods_body_notice_draw(const BfmWoodsBodyNotice *notice,
                               const uint16_t tiles[512][1024], uint32_t pixels[240][320]);

#endif
