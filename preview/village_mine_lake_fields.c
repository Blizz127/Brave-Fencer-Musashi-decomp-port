/* SPDX-License-Identifier: MIT */
#include "village_mine_lake_fields.h"
#include "valley_hinge.h"
#include "bedroom_record.h"
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
    unsigned id, member, pending_hit, parts, pose_count, pose_age;
    uint32_t model_address;
    const uint8_t *standing_script;
    const uint8_t *registered;
    const uint8_t *poses[12];
    uint8_t *model_data;
    size_t model_bytes;
    MusashiForestPlayerMesh *resident;
    BfmForestPose *resident_pose;
    BfmBedroomRoot resident_root;
    int owns_crystal_body, glow_born, ready, approach, claimed;
    BfmWoodsLardwickField field;
} LakeCaptive;

struct BfmVillageMineLakeFields {
    LakeCaptive captives[CAPTIVES];
    BfmWoodsResidentGlowArt *glow;
    MusashiForestPlayerMesh *mesh;
    uint32_t resource_base;
    BfmVillageMineLakeFieldEvent events[CAPTIVES];
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
    c->registered = &variables[58 + c->id];
    if (*c->registered) return 1;
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
    c->owns_crystal_body = 1;
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
    const uint8_t *base_ptr = mounted(scene, bytes, UINT32_C(0x8019fb3c), 4);
    if (!base_ptr) return NULL;
    const uint8_t *bounds = mounted(scene, bytes, u32le(profile + 8), 16);
    if (!bounds || u32le(bounds)) return NULL;
    uint32_t first = u32le(room + 0xb4), end = u32le(room + 0xb8);
    if (end <= first || (end - first) % ROW_BYTES) return NULL;
    const uint8_t *rows = mounted(scene, bytes, first, end - first);
    if (!rows) return NULL;
    BfmVillageMineLakeFields *fields = calloc(1, sizeof *fields);
    if (!fields) return NULL;
    fields->resource_base = u32le(base_ptr);
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
            LakeCaptive *c = &fields->captives[captives];
            /* The US directory gives SC03 the global member range 178..317. */
            const uint8_t *member = mounted(scene, bytes, UINT32_C(0x8018202c) +
                                            2u * u16le(params + 2), 2);
            if (!member || s16le(member) < 178 || s16le(member) >= 318) goto fail;
            c->member = (unsigned)s16le(member) - 178;
            c->model_address = u32le(params + 4);
            const size_t capacity = sizeof c->poses / sizeof c->poses[0];
            const uint8_t *script = mounted(scene, bytes, u32le(params + 8), (capacity + 1) * 8);
            if (!script) goto fail;
            while (c->pose_count < capacity && u32le(script + 8*c->pose_count)) ++c->pose_count;
            if (!c->pose_count || u32le(script + 8*c->pose_count) ||
                u32le(script + 8*c->pose_count + 4) != UINT16_MAX) goto fail;
            c->standing_script = script;
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
    for (unsigned i = 0; i < CAPTIVES; ++i) {
        LakeCaptive *c = &fields->captives[i];
        musashi_forest_player_mesh_free(c->resident);
        free(c->model_data);
        free(c->resident_pose);
    }
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

void bfm_village_mine_lake_fields_resident_allocation(BfmVillageMineLakeFields *fields,
                                                      unsigned resident_id)
{
    if (!fields) return;
    for (unsigned i = 0; i < CAPTIVES; ++i) {
        LakeCaptive *c = &fields->captives[i];
        if (c->id == resident_id) c->owns_crystal_body = 0;
    }
}

int bfm_village_mine_lake_fields_body_contact(const BfmVillageMineLakeFields *fields,
    const int16_t previous[3], int16_t proposed[3], unsigned field_tick, int airborne,
    int *airborne_entry)
{
    if (airborne_entry) *airborne_entry = 0;
    if (!fields || !previous || !proposed) return 0;
    int touched = 0;
    for (unsigned i = 0; i < CAPTIVES; ++i) {
        const LakeCaptive *c = &fields->captives[i];
        if (!c->owns_crystal_body) continue;
        const BfmWoodsLardwickField *f = &c->field;
        BfmForestTransform body;
        bfm_forest_actor_root(f->root.yaw, f->root.xyz, &body);
        if (airborne && airborne_entry && !*airborne_entry) {
            const int16_t from[3] = {previous[0], (int16_t)(previous[1] - 16), previous[2]};
            const int16_t to[3] = {proposed[0], (int16_t)(proposed[1] - 16), proposed[2]};
            int16_t contact[3];
            *airborne_entry = bfm_forest_body_slide_on_entry(&f->bounds, &body, from, to, contact);
        }
        touched |= bfm_valley_box_block_body(&f->bounds, &body, previous, proposed, field_tick, airborne);
    }
    return touched;
}

int bfm_village_mine_lake_fields_body(const BfmVillageMineLakeFields *fields,
    const int16_t previous[3], int16_t proposed[3], unsigned field_tick, int airborne)
{
    return bfm_village_mine_lake_fields_body_contact(fields, previous, proposed, field_tick, airborne, NULL);
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

unsigned bfm_village_mine_lake_fields_blade(BfmVillageMineLakeFields *fields, unsigned event,
    const int16_t from[3], int16_t to[3], int16_t normal[3])
{
    if (!fields || !from || !to) return 0;
    unsigned id = 0;
    for (unsigned i = 0; i < CAPTIVES; ++i) {
        LakeCaptive *c = &fields->captives[i];
        int16_t point[3];
        if (bfm_woods_lardwick_contact(&c->field, event, from, to, point, normal)) {
            memcpy(to, point, sizeof point);
            c->pending_hit = event;
            id = c->id;
        }
    }
    return id;
}

static int load_resident(const BfmVillageMineLakeFields *fields, LakeCaptive *c,
                         const MusashiDiscMedia *disc)
{
    size_t bytes;
    uint8_t *payload = disc ? bfm_village_event_data(disc, c->member, &bytes) : NULL;
    if (!payload) return 0;
    unsigned parts = 0;
    MusashiForestPlayerMesh *mesh = musashi_forest_player_mesh_from_table(
        payload, bytes, fields->resource_base, c->model_address, &parts);
    if (!mesh) goto fail;
    for (unsigned i = 0; i < c->pose_count; ++i) {
        uint32_t key = u32le(c->standing_script + 8*i);
        if ((key >> 24) != 0x81 || u32le(c->standing_script + 8*i + 4) != 1) goto fail;
        uint32_t address = key & ~UINT32_C(0x01000000);
        if (address < fields->resource_base) goto fail;
        size_t pos = address - fields->resource_base;
        if (pos > bytes || 8u*parts > bytes-pos) goto fail;
        c->poses[i] = payload + pos;
    }
    c->model_data = payload;
    c->model_bytes = bytes;
    c->resident = mesh;
    c->parts = parts;
    return 1;
fail:
    musashi_forest_player_mesh_free(mesh);
    free(payload);
    return 0;
}

int bfm_village_mine_lake_fields_service(BfmVillageMineLakeFields *fields, BfmGameplayRun *run,
    const int16_t player[3], const int32_t eye[3], unsigned tick, const MusashiDiscMedia *disc)
{
    if (!fields || !player || !eye) return 0;
    memset(fields->events, 0, sizeof fields->events);
    if (!run) return 1;
    for (unsigned i = 0; i < CAPTIVES; ++i) {
        LakeCaptive *c = &fields->captives[i];
        BfmWoodsLardwickField *f = &c->field;
        BfmVillageMineLakeFieldEvent *event = &fields->events[i];
        event->resident_id = c->id;
        if (!f->active || *c->registered) continue;
        if (!bfm_woods_lardwick_busy(f)) {
            bfm_woods_lardwick_field_step(f, run, player, tick);
            if (c->pending_hit) {
                event->started = bfm_woods_lardwick_strike(f, c->pending_hit);
                c->pending_hit = 0;
            }
            continue;
        }
        if (c->glow_born && !c->resident) load_resident(fields, c, disc);
        if (!f->crystal_visible && !c->resident) continue;
        if (c->ready && !c->resident_pose) {
            /* This is the state-6 model-slot allocation, one field after
             * optical readiness. The previous crystal no longer backs a box. */
            bfm_village_mine_lake_fields_resident_allocation(fields, c->id);
            c->resident_pose = malloc(sizeof *c->resident_pose);
            if (!c->resident_pose) continue;
        }
        int at_gate = !f->crystal_visible && !c->ready;
        if (!c->approach) {
            BfmWoodsLardwickEvents fx = {0};
            BfmBinchoReveal phase = bfm_bincho_reveal_step(f, run, eye, &fx);
            if (phase == BFM_BINCHO_REVEAL_INVALID) return 0;
            c->glow_born = 1;
            event->sound = fx.sound;
            if (at_gate) c->ready = 1;
            if (phase == BFM_BINCHO_REVEAL_SPAWN) {
                c->resident_root = f->root;
                c->resident_root.yaw = f->soldier_yaw;
                event->reveal = 1;
            }
            if (phase == BFM_BINCHO_REVEAL_APPROACH) c->approach = event->approach = 1;
        }
        if (f->soldier_visible) {
            event->root = &c->resident_root;
            event->model = c->resident;
            event->resource = c->model_data;
            event->resource_bytes = c->model_bytes;
            event->resource_base = fields->resource_base;
            event->parts = c->parts;
            if (!c->claimed) {
                event->pose_tick = c->pose_age;
                if (!bfm_forest_actor_pose_decode(c->poses[c->pose_age], 8*c->parts, c->parts,
                                                  c->resident_pose)) return 0;
                c->pose_age = (c->pose_age + 1) % c->pose_count;
            }
        }
    }
    return 1;
}

const BfmVillageMineLakeFieldEvent *bfm_village_mine_lake_fields_events(
    const BfmVillageMineLakeFields *fields)
{
    return fields ? fields->events : NULL;
}

int bfm_village_mine_lake_fields_busy(const BfmVillageMineLakeFields *fields)
{
    if (!fields) return 0;
    for (unsigned i = 0; i < CAPTIVES; ++i) {
        const LakeCaptive *c = &fields->captives[i];
        if (!*c->registered && bfm_woods_lardwick_busy(&c->field)) return 1;
    }
    return 0;
}

void bfm_village_mine_lake_fields_claim_body(BfmVillageMineLakeFields *fields, unsigned id)
{
    if (!fields) return;
    for (unsigned i = 0; i < CAPTIVES; ++i) {
        LakeCaptive *c = &fields->captives[i];
        if (c->id == id && c->approach) c->claimed = 1;
    }
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
        if (!bfm_bedroom_figure_add_translucent(figure, view, &fixture, 2)) return 0;
    }
    for (unsigned i = 0; i < CAPTIVES; ++i) {
        const LakeCaptive *c = &fields->captives[i];
        const BfmWoodsLardwickField *f = &c->field;
        if (*c->registered) continue;
        if (f->crystal_visible && !bfm_woods_resident_glow_art_add(fields->glow, figure, view,
            f->root.xyz, f->root.yaw, f->light_pitch, f->light_yaw, f->flash,
            f->lights_active, f->burst_updates, f->glow_visible)) return 0;
        if (f->soldier_visible && !c->claimed) {
            BfmForestTransform root;
            bfm_forest_actor_root(c->resident_root.yaw, c->resident_root.xyz, &root);
            const BfmForestActor body = {.mesh=c->resident, .pose=c->resident_pose,
                .root=&root, .visible_parts=(UINT32_C(1) << c->parts)-1};
            if (!bfm_bedroom_figure_add_actor(figure, view, &body, NULL, NULL, 0)) return 0;
        }
        if (f->glow_scale && !bfm_woods_resident_release_glow_add(fields->glow, figure, view,
            &f->glow_transform, f->glow_scale)) return 0;
    }
    return 1;
}
