/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_EARTH_SHRINE_VISIT_H
#define BFM_WOODS_EARTH_SHRINE_VISIT_H

#include "woods_box_view.h"
#include "woods_caster_melee.h"
#include "village_arrival_motion.h"
#include "forest_message.h"
#include "bedroom_record.h"

typedef struct BfmEarthShrineVisit BfmEarthShrineVisit;

/* Open only while the mounted room's unspent tablet is present. */
BfmEarthShrineVisit *bfm_earth_visit_open(const MusashiDiscMedia *disc, const uint8_t *scene,
    size_t scene_bytes, const uint8_t *main, size_t main_bytes, uint16_t tiles[512][1024],
    BfmVillageMotion *motion, BfmWoodsBoxView *view, BfmSaveRecord *save);
void bfm_earth_visit_free(BfmEarthShrineVisit *visit);
int bfm_earth_visit_controls(const BfmEarthShrineVisit *visit);
int bfm_earth_visit_strike(BfmEarthShrineVisit *visit, BfmGameplayRun *run, unsigned clock,
    const BfmWoodsCasterMeleeFrame *swing, BfmForestMessage **opened);
int bfm_earth_visit_step(BfmEarthShrineVisit *visit, BfmGameplayRun *run, unsigned clock,
                         int blade_settled, BfmForestMessage **opened);
void bfm_earth_visit_camera(BfmEarthShrineVisit *visit);
int bfm_earth_visit_close(BfmEarthShrineVisit *visit);
unsigned bfm_earth_visit_score(const BfmEarthShrineVisit *visit, unsigned ordinary);
unsigned bfm_earth_visit_sound(BfmEarthShrineVisit *visit);
int bfm_earth_visit_hud(BfmEarthShrineVisit *visit);
void bfm_earth_visit_pose(const BfmEarthShrineVisit *visit, BfmBedroomPose *pose);
int bfm_earth_visit_dark(const BfmEarthShrineVisit *visit);
int bfm_earth_visit_draw(const BfmEarthShrineVisit *visit, unsigned age,
                         BfmBedroomFigure *figure, const BfmBedroomView *view);
#endif
