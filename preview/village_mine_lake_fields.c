/* SPDX-License-Identifier: MIT */
#include "village_mine_lake_fields.h"
#include "valley_hinge.h"
#include "woods_lardwick_field.h"
#include "woods_resident_glow_art.h"
#include "../pc_port/forest_player_bytes.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

enum { CAPTIVES = 2, FIXTURES = 2, ROW_BYTES = 20 };
static const uint32_t base = UINT32_C(0x80128158);
static const uint32_t crystal_profile = UINT32_C(0x80185780);
static const uint32_t fixture_model = UINT32_C(0x8019bf28);

typedef struct {
    unsigned id;
    BfmWoodsLardwickField field;
} LakeCaptive;

struct BfmVillageMineLakeFields {
    LakeCaptive captives[CAPTIVES];
    BfmWoodsResidentGlowArt *glow;
    MusashiForestPlayerMesh *mesh;
    BfmForestPose fixture_pose;
    BfmForestTransform fixtures[FIXTURES];
};

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t count)
{
    if (address < base || address - base > bytes || count > bytes - (address - base))
        return NULL;
    return scene + (address - base);
}

static int load_captive(LakeCaptive *c, const uint8_t *row, const uint8_t *params,
                        const uint8_t *bounds, const uint8_t *hits, const uint8_t variables[256])
{
    if (!params || (params[0] != 21 && params[0] != 27) || params[1] != 1) return 0;
    c->id = params[0];
    if (variables[58 + c->id]) return 1;
    BfmWoodsLardwickField *f = &c->field;
    for (unsigned axis = 0; axis < 3; ++axis) {
        f->bounds.low[axis] = s16le(bounds + 4 + 4 * axis);
        f->bounds.high[axis] = s16le(bounds + 6 + 4 * axis);
        if (f->bounds.low[axis] > f->bounds.high[axis]) return 0;
        f->root.xyz[axis] = s16le(row + 2 * axis);
    }
    if (f->root.xyz[1] < INT16_MIN + 4) return 0;
    f->root.xyz[1] -= 4;
    f->root.yaw = u16le(row + 12) & 4095;
    f->root.normal[1] = -4096;
    f->active = f->crystal_visible = f->glow_visible = 1;
    f->accepted_hits = hits;
    return 1;
}

BfmVillageMineLakeFields *bfm_village_mine_lake_fields_open(const uint8_t *scene, size_t bytes,
                                                            const uint8_t variables[256])
{
    if (!scene || !variables || bytes > BFM_FIELD_SCENE_CAPACITY) return NULL;
    const uint8_t *room = mounted(scene, bytes, UINT32_C(0x80185a50), 0xbc);
    const uint8_t *profile = mounted(scene, bytes, crystal_profile, 12);
    const uint8_t *hits = mounted(scene, bytes, UINT32_C(0x801857b4), 8);
    if (!room || !profile || !(u16le(profile + 2) & 0x1800) || !hits) return NULL;
    const uint8_t *bounds = mounted(scene, bytes, u32le(profile + 8), 16);
    if (!bounds || u32le(bounds)) return NULL;
    uint32_t first = u32le(room + 0xb4), end = u32le(room + 0xb8);
    if (end <= first || (end - first) % ROW_BYTES) return NULL;
    const uint8_t *rows = mounted(scene, bytes, first, end - first);
    if (!rows) return NULL;
    BfmVillageMineLakeFields *fields = calloc(1, sizeof *fields);
    if (!fields) return NULL;
    unsigned captives = 0, fixtures = 0;
    size_t offset = 0;
    for (; offset < end - first; offset += ROW_BYTES) {
        const uint8_t *row = rows + offset;
        unsigned kind = u16le(row + 6);
        if (!kind) break;
        if (kind == 296) {
            const uint8_t *params = mounted(scene, bytes, u32le(row + 16), 16);
            if (captives == CAPTIVES ||
                !load_captive(&fields->captives[captives], row, params, bounds, hits, variables)) goto fail;
            ++captives;
        } else if (kind == 520) {
            if (fixtures == FIXTURES) goto fail;
            int16_t xyz[3];
            for (unsigned axis = 0; axis < 3; ++axis) xyz[axis] = s16le(row + 2 * axis);
            bfm_forest_actor_root(u16le(row + 12) & 4095, xyz, &fields->fixtures[fixtures++]);
        }
    }
    if (offset == end - first || captives != CAPTIVES || fixtures != FIXTURES ||
        fields->captives[0].id == fields->captives[1].id) goto fail;
    if (fields->captives[0].field.active || fields->captives[1].field.active) {
        fields->glow = bfm_bincho_glow_art_load(scene, bytes, crystal_profile, UINT32_C(0x8019f528));
        if (!fields->glow) goto fail;
    }
    unsigned parts = 0;
    fields->mesh = musashi_forest_player_mesh_from_table(scene, bytes, base, fixture_model, &parts);
    if (!fields->mesh || parts != 1) goto fail;
    bfm_forest_actor_root(0, (const int16_t[3]){0}, &fields->fixture_pose.local[0]);
    return fields;
fail:
    bfm_village_mine_lake_fields_free(fields);
    return NULL;
}

void bfm_village_mine_lake_fields_free(BfmVillageMineLakeFields *fields)
{
    if (!fields) return;
    musashi_forest_player_mesh_free(fields->mesh);
    bfm_woods_resident_glow_art_free(fields->glow);
    free(fields);
}

void bfm_village_mine_lake_fields_step(BfmVillageMineLakeFields *fields, BfmGameplayRun *run,
                                      const int16_t player[3], unsigned field_tick)
{
    if (!fields) return;
    for (unsigned i = 0; i < CAPTIVES; ++i)
        bfm_woods_lardwick_field_step(&fields->captives[i].field, run, player, field_tick);
}

int bfm_village_mine_lake_fields_body(const BfmVillageMineLakeFields *fields,
    const int16_t previous[3], int16_t proposed[3], unsigned field_tick, int airborne)
{
    if (!fields || !previous || !proposed) return 0;
    int blocked = 0;
    for (unsigned i = 0; i < CAPTIVES; ++i) {
        const BfmWoodsLardwickField *f = &fields->captives[i].field;
        if (!f->crystal_visible) continue;
        BfmForestTransform body;
        bfm_forest_actor_root(f->root.yaw, f->root.xyz, &body);
        blocked |= bfm_valley_box_block_body(&f->bounds, &body, previous, proposed, field_tick, airborne);
    }
    return blocked;
}

unsigned bfm_village_mine_lake_fields_hit(const BfmVillageMineLakeFields *fields,
    const BfmVillageGround *ground, const BfmWoodsCasterMeleeFrame *swing, int16_t hit[3])
{
    if (!fields || !ground || !swing || !hit || swing->rays > sizeof swing->to / sizeof swing->to[0])
        return 0;
    for (unsigned ray = 0; ray < swing->rays; ++ray) {
        int16_t end[3], normal[3], point[3];
        uint16_t material;
        unsigned id = 0;
        memcpy(end, swing->to[ray], sizeof end);
        bfm_village_ground_blade(ground, swing->from, end, normal, &material);
        for (unsigned i = 0; i < CAPTIVES; ++i) {
            const LakeCaptive *c = &fields->captives[i];
            if (bfm_woods_lardwick_contact(&c->field, swing->event, swing->from, end, point, NULL)) {
                memcpy(end, point, sizeof end);
                id = c->id;
            }
        }
        if (id) {
            memcpy(hit, end, sizeof end);
            return id;
        }
    }
    return 0;
}

int bfm_village_mine_lake_fields_draw(void *context, BfmBedroomFigure *figure,
                                     const BfmBedroomView *view)
{
    const BfmVillageMineLakeFields *fields = context;
    if (!fields || !figure || !view) return 0;
    BfmForestActor fixture = {.mesh=fields->mesh, .pose=&fields->fixture_pose, .visible_parts=1};
    for (unsigned i = 0; i < FIXTURES; ++i) {
        fixture.root = &fields->fixtures[i];
        /* The lake fixtures darken the underlying geometry with blend mode 2. */
        if (!bfm_bedroom_figure_add_lit_translucent(figure, view, &fixture, NULL, 2)) return 0;
    }
    for (unsigned i = 0; i < CAPTIVES; ++i) {
        const BfmWoodsLardwickField *f = &fields->captives[i].field;
        if (f->crystal_visible && !bfm_woods_resident_glow_art_add(fields->glow, figure, view,
            f->root.xyz, f->root.yaw, f->light_pitch, f->light_yaw, f->flash,
            f->lights_active, 0, f->glow_visible)) return 0;
    }
    return 1;
}
