/* SPDX-License-Identifier: MIT */
#include "hells_valley_view.h"
#include "village_lighting.h"
#include "valley_knitter.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>
#include <string.h>

struct BfmHellsValleyView {
    BfmVillageField field;
    BfmVillageDaylight daylight;
    BfmVillageCamera camera, submitted;
    uint16_t (*tiles)[1024];
    const uint8_t *distances;
    unsigned zoom;
};

BfmHellsValleyView *bfm_hells_valley_view_open(const MusashiDiscMedia *disc, const uint8_t *scene,
    size_t bytes, unsigned phase, uint16_t tiles[512][1024])
{
    const uint32_t base = UINT32_C(0x80128158), room = UINT32_C(0x80186620);
    const size_t binding = UINT32_C(0x8018231c) - base;
    const size_t distances = UINT32_C(0x80185fbc) - base;
    if (!scene || !disc || !tiles || bytes < room - base + 0xcc ||
        u32le(scene + binding) != room || !u16le(scene + distances) ||
        !u16le(scene + distances + 2)) return NULL;

    BfmHellsValleyView *view = calloc(1, sizeof *view);
    if (!view) return NULL;
    if (!bfm_village_field_load_room(&view->field, disc, scene, bytes, room, UINT32_C(0x80184194)) ||
        !bfm_hells_valley_daylight_begin(&view->daylight, scene, bytes, phase, tiles)) {
        bfm_hells_valley_view_free(view);
        return NULL;
    }
    view->tiles = tiles;
    view->distances = scene + distances;
    return view;
}

void bfm_hells_valley_view_free(BfmHellsValleyView *view)
{
    if (view) {
        bfm_village_field_clear(&view->field);
        free(view);
    }
}

BfmVillageField *bfm_hells_valley_view_field(BfmHellsValleyView *view)
{
    return view ? &view->field : NULL;
}

const BfmVillageCamera *bfm_hells_valley_view_submitted(const BfmHellsValleyView *view)
{
    return view ? &view->submitted : NULL;
}

const BfmVillageCamera *bfm_hells_valley_view_camera(const BfmHellsValleyView *view)
{
    return view ? &view->camera : NULL;
}

const BfmBedroomView *bfm_hells_valley_projection(const BfmHellsValleyView *view)
{
    return view ? bfm_bedroom_field_view(view->field.field) : NULL;
}

static int approach(int from, int to)
{
    int delta = to - from;
    int step = delta / 4;
    if (!step && delta) step = delta < 0 ? -1 : 1;
    return from + step;
}

static int limit(int value, int lower, int upper)
{
    if (value < lower) return lower;
    return value > upper ? upper : value;
}

void bfm_hells_valley_view_follow_captive(BfmHellsValleyView *view, const int16_t foot[3],
    int entering, int shoulder_pressed, int shake_y, BfmValleyKnitter *knitter)
{
    if (!view || !foot) return;
    BfmVillageCamera *c = &view->camera;
    if (entering) {
        view->zoom = 0;
        *c = (BfmVillageCamera){.focal = 300, .yaw = 2048, .following = 1,
                                .radius = u16le(view->distances)};
        c->target[1] = foot[1];
    }
    if (shoulder_pressed) view->zoom ^= 1;
    c->radius = approach(c->radius, u16le(view->distances + 2 * view->zoom));
    c->target[0] = limit(foot[0], -160, 2560);
    c->target[1] = approach(c->target[1], foot[1]);
    c->target[2] = limit(foot[2], -2224, 2048) + 256;
    /* The field orbit takes the eye-to-target bearing, opposite the camera arm. */
    if (bfm_bedroom_field_orbit(view->field.field, c->target, 0, c->radius, 682, 1, 1))
        memcpy(c->eye, bfm_bedroom_field_view(view->field.field)->eye, sizeof c->eye);
    view->submitted = *c;
    if (knitter) bfm_valley_knitter_camera(knitter, &view->submitted);
    view->submitted.eye[1] += shake_y;
    view->submitted.target[1] += shake_y;
    /* The boulders use this field's submitted camera even if its draw is skipped. */
    bfm_bedroom_field_viewpoint(view->field.field, view->submitted.eye, view->submitted.target, 1, 1);
}

void bfm_hells_valley_view_follow(BfmHellsValleyView *view, const int16_t foot[3],
                                  int entering, int shoulder_pressed, int shake_y)
{
    bfm_hells_valley_view_follow_captive(view, foot, entering, shoulder_pressed, shake_y, NULL);
}

void bfm_hells_valley_view_light(BfmHellsValleyView *view, unsigned phase)
{
    if (view) bfm_village_daylight_step(&view->daylight, phase, view->tiles);
}

int bfm_hells_valley_view_draw(BfmHellsValleyView *view, const BfmBedroomRoot *root,
    const BfmBedroomPose *pose, const uint8_t flags[64], unsigned age, unsigned scale_q12, int hide_body,
    uint32_t pixels[240][320],
    int (*submit)(void *, BfmBedroomFigure *, const BfmBedroomView *), void *actor)
{
    if (!view || !root || !pose || !flags || !pixels) return 0;
    /* This descriptor has no sky bands. The textured gorge is drawn on black. */
    for (unsigned y = 0; y < 240; ++y)
        for (unsigned x = 0; x < 320; ++x) pixels[y][x] = UINT32_C(0xff000000);
    if (!bfm_bedroom_figure_lights(view->field.hero, view->daylight.lamps) ||
        !bfm_village_field_prepare_scaled(&view->field, &view->submitted, root, pose, flags,
                                          age, hide_body ? 0 : bfm_village_field_visible(pose, flags),
                                          scale_q12, view->tiles, &pixels[0][0])) return 0;
    const BfmBedroomView *projection = bfm_bedroom_field_view(view->field.field);
    if (submit && !submit(actor, view->field.hero, projection)) return 0;
    bfm_bedroom_field_draw(view->field.field, view->tiles, &pixels[0][0],
                           bfm_bedroom_figure_draw_to, view->field.hero);
    return 1;
}
