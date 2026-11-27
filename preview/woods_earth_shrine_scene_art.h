/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_EARTH_SHRINE_SCENE_ART_H
#define BFM_WOODS_EARTH_SHRINE_SCENE_ART_H

#include "bedroom_figure.h"
#include "gameplay_run.h"
#include "woods_earth_shrine_release.h"

typedef struct BfmWoodsEarthShrineSceneArt BfmWoodsEarthShrineSceneArt;

/* Open only for the actual, unspent shrine registration. */
BfmWoodsEarthShrineSceneArt *bfm_woods_earth_shrine_scene_art_open(
    const uint8_t *scene, size_t bytes);
void bfm_woods_earth_shrine_scene_art_free(BfmWoodsEarthShrineSceneArt *art);
/* MAIN/10 supplies the ten smoke cells. Supply the main-loop field clock
 * and the current room lamp before advancing the child. */
int bfm_woods_earth_shrine_scene_art_bind_smoke(BfmWoodsEarthShrineSceneArt *art,
    const uint8_t *main, size_t bytes);
void bfm_woods_earth_shrine_scene_art_set_field(BfmWoodsEarthShrineSceneArt *art,
    unsigned main_clock, const uint8_t third_lamp_rgb[3]);

/* Forward the release host's CREST_STAGE, DUST, SHATTER and TRAIL
 * notifications as issued. SHATTER uses the current camera and run RNG. */
int bfm_woods_earth_shrine_scene_art_notify(BfmWoodsEarthShrineSceneArt *art,
    const BfmEarthShrineRelease *child, BfmEarthReleaseEvent event, unsigned kind,
    BfmGameplayRun *run, const BfmBedroomView *view);

/* Once per completed child actor update. Redrawing a paused field does not
 * advance light, fragments, echoes, or the completed model used by pickup. */
void bfm_woods_earth_shrine_scene_art_update(BfmWoodsEarthShrineSceneArt *art,
    const BfmEarthShrineRelease *child);
int bfm_woods_earth_shrine_scene_art_draw(const BfmWoodsEarthShrineSceneArt *art,
    const BfmEarthShrineRelease *child, unsigned actor_age, unsigned texture_member,
    BfmBedroomFigure *figure, const BfmBedroomView *view);

/* Supply this transform to bfm_woods_earth_scroll_touch_test from
 * TOUCH_SCROLL, before publishing this update's new model via update. */
const BfmForestTransform *bfm_woods_earth_shrine_scene_art_scroll_model(
    const BfmWoodsEarthShrineSceneArt *art);

#endif
