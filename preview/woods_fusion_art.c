/* SPDX-License-Identifier: MIT */
#include "woods_fusion_art.h"
#include "woods_charged_fx.h"
#include "woods_flight_glow.h"
#include "cable_sparks.h"
#include <stdlib.h>

int bfm_woods_fusion_art_pose(const uint8_t *main, size_t main_bytes, uint32_t pose,
                              uint32_t visible, int blade_away, BfmBedroomPose *body)
{
    if (!body) return 0;
    if (pose) {
        const uint32_t main_base = UINT32_C(0x800cedf8);
        if (!main || (pose & 3) || pose < main_base + 16) return 0;
        size_t offset = pose - main_base - 16;
        if (offset > main_bytes || main_bytes - offset < 256) return 0;
        body->pose = main + offset;
        body->show_weapon = (visible >> 22) & 1u;
    }
    body->hide_fusion = !!blade_away;
    return 1;
}

static int effect_camera(const BfmBedroomView *view, BfmForestCamera *camera)
{
    if (view->focal <= 0 || view->focal > UINT16_MAX) return 0;
    camera->projection = (uint16_t)view->focal;
    for (unsigned i = 0; i < 3; ++i) {
        for (unsigned j = 0; j < 3; ++j) {
            int axis = view->axes[i][j];
            if (axis < INT16_MIN || axis > INT16_MAX) return 0;
            camera->rotation[i][j] = (int16_t)axis;
        }
        camera->translation[i] = view->translation[i];
    }
    return 1;
}

int bfm_woods_fusion_art_append(BfmBedroomFigure *figure, const BfmBedroomView *view,
                                const BfmForestActor *blade, const BfmForestTetherTrails *trails,
                                const BfmForestDamageDigits *bp_digits)
{
    if (!figure || !view) return 0;
    if ((!trails || !trails->enabled) && (!bp_digits || !bp_digits->amount))
        return !blade || bfm_bedroom_figure_add_fusion(figure, view, blade, NULL, 0);
    BfmForestCamera camera;
    if (!effect_camera(view, &camera)) return 0;
    BfmForestDustCard cards[BFM_TETHER_CARDS + BFM_FOREST_DAMAGE_DIGIT_CARDS];
    size_t count = bfm_forest_tether_trails_project(trails, &camera, cards, BFM_TETHER_CARDS);
    count += bfm_forest_damage_digits_project(bp_digits, &camera, cards + count);
    return bfm_bedroom_figure_add_fusion(figure, view, blade, cards, count);
}

int bfm_woods_fusion_art_cut(BfmBedroomFigure *figure, const BfmBedroomView *view,
                            const BfmWoodsChargedFx *effects)
{
    if (!figure || !view || !effects) return 0;
    BfmForestCamera camera;
    if (!effect_camera(view, &camera)) return 0;
    BfmForestDustCard cards[BFM_WOODS_CHARGED_CARDS];
    size_t count = bfm_woods_charged_fx_project(effects, &camera, cards);
    return !count || bfm_bedroom_figure_add_fusion(figure, view, NULL, cards, count);
}

int bfm_woods_fusion_art_lights(BfmBedroomFigure *figure, const BfmBedroomView *view,
                                const BfmWoodsFusionFrame *frame)
{
    if (!figure || !view || !frame) return 0;
    if (!frame->glow) return 1;
    BfmForestCamera camera;
    if (!effect_camera(view, &camera)) return 0;
    BfmForestDustCard cards[BFM_WOODS_HALO_CARDS + BFM_WOODS_BLADE_SPARK_CARDS +
                            BFM_WOODS_CAPTURE_LIGHT_CARDS];
    size_t count = bfm_woods_flight_glow_project(frame->glow, frame->flight, &camera, view->eye,
                                                 cards, sizeof cards / sizeof cards[0]);
    return !count || bfm_bedroom_figure_add_fusion(figure, view, NULL, cards, count);
}

static int slot_order(const void *left, const void *right)
{
    const BfmForestDustCard *a = left, *b = right;
    return (a->slot > b->slot) - (a->slot < b->slot);
}

int bfm_woods_charge_art_append(BfmBedroomFigure *figure, const BfmBedroomView *view,
                                const BfmSteamChargeEffects *effects, const BfmWoodsFootDust *foot,
                                const BfmWoodsChargeArt *art)
{
    if (!figure || !view || !effects || !foot || !art) return 0;
    BfmForestCamera camera;
    if (!effect_camera(view, &camera)) return 0;
    BfmForestDustCard cards[BFM_STEAM_CHARGE_SLOTS];
    unsigned lines = bfm_steam_charge_project(effects, &camera, cards, BFM_STEAM_CHARGE_SLOTS);
    unsigned dust = bfm_woods_foot_dust_project(foot, effects, &camera,
                                               cards + lines, BFM_STEAM_CHARGE_SLOTS - lines);
    if (lines && dust) qsort(cards, lines + dust, sizeof *cards, slot_order);
    unsigned begin = 0, count = lines + dust;
    uint32_t centers = effects->centers;
    for (unsigned slot = 0; centers && slot < BFM_STEAM_CHARGE_SLOTS; ++slot, centers >>= 1) {
        if (!(centers & 1u)) continue;
        /* The emitter submits its own mesh at its visit, between earlier
         * and later slots. Preserve the ten primitives' local order. */
        unsigned end = begin;
        while (end < count && cards[end].slot < slot) ++end;
        if (end > begin && !bfm_bedroom_figure_add_fusion(figure, view, NULL,
                                                          cards + begin, end - begin)) return 0;
        begin = end;
        BfmForestDustCard core[BFM_WOODS_CHARGE_CORE_CARDS];
        size_t faces = bfm_woods_charge_core_project(&art->core, effects->slots[slot].tip,
            &art->model, art->anchors, art->tick, art->gauge, view->eye, &camera,
            core, BFM_WOODS_CHARGE_CORE_CARDS);
        if (faces && !bfm_bedroom_figure_add_fusion(figure, view, NULL, core, faces)) return 0;
    }
    return begin == count ||
           bfm_bedroom_figure_add_fusion(figure, view, NULL, cards + begin, count - begin);
}

int bfm_woods_charged_impact_load(BfmWoodsChargedImpact *impact, const uint8_t *scene, size_t bytes)
{
    const size_t program = 0x8018a83cu - 0x80128158u;
    const size_t blade = 0x80189770u - 0x80128158u;
    if (!impact || !scene || bytes < program ||
        bytes-program < BFM_FOREST_IMPACT_PROGRAM_BYTES || scene[blade + 8] != 11) return 0;
    const size_t orientations = 0x8018a7f4u - 0x80128158u;
    *impact = (BfmWoodsChargedImpact){.fork_orientations = scene + orientations};
    return bfm_forest_impact_art_load(&impact->art, scene+program, bytes-program, scene[blade + 9]);
}

int bfm_woods_charged_impact_issue(BfmWoodsChargedImpact *impact, const int16_t point[3],
                                  const int16_t normal[3], const int32_t eye[3])
{
    return impact && bfm_forest_strike_effects_queue_impact(&impact->pool, &impact->art,
                                                           point, normal, eye);
}

int bfm_woods_charged_impact_terrain(BfmWoodsChargedImpact *impact, const int16_t point[3],
                                    const int16_t normal[3], int blocked_actor)
{
    if (!impact || !impact->fork_orientations || !point || !normal) return 0;
    int index = bfm_forest_strike_reserve_room(&impact->pool);
    if (index < 0) return 1;
    BfmWoodsTerrainSpark *spark = &impact->terrain[index];
    *spark = (BfmWoodsTerrainSpark){0};
    int pitch, yaw;
    bfm_forest_impact_angles(normal, &pitch, &yaw);
    bfm_forest_actor_pitch_yaw((unsigned)pitch, (unsigned)yaw, point, &spark->face);
    int y = blocked_actor ? -4 : -2;
    int z = blocked_actor ? -8 : 2;
    for (unsigned k = 0; k < 3; ++k) {
        int offset = spark->face.rotation[k][1] * y + spark->face.rotation[k][2] * z;
        int value = point[k] + offset / 4096 - (offset % 4096 < 0);
        unsigned word = (unsigned)value & 65535u;
        spark->point[k] = (int16_t)(word <= INT16_MAX ? (int)word : (int)word - 65536);
    }
    return 1;
}

typedef struct {
    BfmWoodsChargedImpact *impact;
    const BfmBedroomView *view;
} TerrainSparkView;

static unsigned terrain_spark_step(void *context, unsigned index, BfmGameplayRun *run,
                                   BfmForestDustCard *cards)
{
    TerrainSparkView *field = context;
    BfmWoodsChargedImpact *impact = field->impact;
    const BfmBedroomView *view = field->view;
    BfmForestStrikeEffect *slot = &impact->pool.slots[index];
    BfmWoodsTerrainSpark *spark = &impact->terrain[index];
    if (spark->trail) {
        if (slot->fresh) {
            bfm_field_spark_velocity(&spark->face, bfm_gameplay_random(run), spark->velocity);
            return 0;
        }
        if (spark->age < 5)
            return bfm_field_spark_trail(spark->point, spark->velocity, ++spark->age, view, cards);
    } else {
        if (slot->fresh) {
            for (unsigned i = 0; i < 2; ++i) {
                int child = bfm_forest_strike_reserve_room(&impact->pool);
                if (child < 0) break;
                impact->terrain[child] = *spark;
                impact->terrain[child].trail = 1;
            }
        }
        if (spark->age < 3)
            return bfm_field_spark_fork(&spark->face, impact->fork_orientations, 16,
                                        spark->point, spark->age++, view, cards);
    }
    slot->kind = BFM_STRIKE_EMPTY;
    return 0;
}

int bfm_woods_charged_impact_step(BfmWoodsChargedImpact *impact, BfmGameplayRun *run,
                                 const BfmBedroomView *view)
{
    BfmForestCamera camera;
    if (!impact || !run || !view || !effect_camera(view, &camera)) return 0;
    TerrainSparkView field = {impact, view};
    impact->count = bfm_forest_strike_step_shared(&impact->pool, view->eye, &camera, run,
                                                 impact->cards, terrain_spark_step, &field);
    return 1;
}

int bfm_woods_charged_impact_add(const BfmWoodsChargedImpact *impact, BfmBedroomFigure *figure,
                                const BfmBedroomView *view)
{
    return impact && figure && view &&
           (!impact->count || bfm_bedroom_figure_add_fusion(figure, view, NULL, impact->cards, impact->count));
}
