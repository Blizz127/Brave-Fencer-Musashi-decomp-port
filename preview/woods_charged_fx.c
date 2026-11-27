/* SPDX-License-Identifier: MIT */
#include "woods_charged_fx.h"
#include "bedroom_field.h"
#include "../pc_port/forest_player_bytes.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

enum { WAVE_FACES = 96, UNIT = 4096 };

typedef struct {
    int horizontal, vertical, growth, yaw, fading;
} Wave;

typedef struct {
    int16_t center[3];
    int scale, speed;
    unsigned age;
} BfmChargedRing;

static void ring_start(BfmChargedRing *ring, const int16_t tip[3])
{
    *ring = (BfmChargedRing){.center = {tip[0], (int16_t)(tip[1] - 64), tip[2]},
                             .scale = 4096, .speed = 1792};
}

static void ring_tick(BfmChargedRing *ring)
{
    if (!ring->scale) return;
    ++ring->age;
    if (ring->speed >= 0) {
        ring->speed -= 96;
        ring->scale += ring->speed;
    } else if (ring->age > 20) {
        ring->scale = 0;
    }
}

static unsigned ring_light(unsigned initial, unsigned age)
{
    return age * 16 < initial ? initial - age * 16 : 0;
}

static int ring_fraction(int difference)
{
    int product = 3276 * difference;
    return product / 4096 - (product % 4096 < 0);
}

static unsigned ring_project(const BfmChargedRing *ring, const BfmForestCamera *camera,
                             BfmForestDustCard cards[12])
{
    if (!ring->scale) return 0;
    /* A quarter turn about world X lays the generated circle in the XZ plane.
     * Compose with the camera before multiplying the local rim coordinates. */
    BfmForestTransform horizontal = {
        .rotation = {{ring->scale, 0, 0}, {0, 0, -4096}, {0, ring->scale, 0}},
        .translation = {ring->center[0], ring->center[1], ring->center[2]}
    };
    BfmForestCamera local;
    if (!bfm_forest_effect_camera(camera, &horizontal, &local)) return 0;
    int16_t points[4][3] = {{0}};
    BfmForestDustCard projected = {0};
    if (!bfm_forest_project_effect_quad(&local, points, &projected) || projected.bucket + 1 >= 4096)
        return 0;
    unsigned bucket = projected.bucket + 1;
    BfmForestRasterVertex center = projected.vertices[0], outer[12], inner[12];
    int16_t rim[12][2];
    for (unsigned i = 0; i <= 3; ++i) {
        double angle = (6.28318530717958647693 / 4096) * (341 * i);
        rim[i][0] = (int16_t)(96 * lround(4096 * sin(angle)) / 4096);
        rim[i][1] = (int16_t)(96 * lround(4096 * cos(angle)) / 4096);
    }
    for (unsigned i = 4; i <= 6; ++i) {
        rim[i][0] = rim[6 - i][0];
        rim[i][1] = (int16_t)-rim[6 - i][1];
    }
    for (unsigned i = 7; i < 12; ++i) {
        rim[i][0] = (int16_t)-rim[12 - i][0];
        rim[i][1] = rim[12 - i][1];
    }

    unsigned warm = ring_light(240, ring->age);
    uint32_t rgb = warm | warm << 8 | ring_light(192, ring->age) << 16;
    for (unsigned i = 0; i < 12; i += 4) {
        for (unsigned k = 0; k < 4; ++k) {
            points[k][0] = rim[i+k][0];
            points[k][1] = rim[i+k][1];
        }
        if (!bfm_forest_project_effect_quad(&local, points, &projected)) {
            /* Only the center gates the draw. The display keeps saturated
             * screen coordinates when a rim point passes the near plane. */
            BfmBedroomView view = {.focal = local.projection};
            for (unsigned row = 0; row < 3; ++row) {
                view.translation[row] = local.translation[row];
                for (unsigned axis = 0; axis < 3; ++axis)
                    view.axes[row][axis] = local.rotation[row][axis];
            }
            for (unsigned k = 0; k < 4; ++k) {
                int screen[3];
                bfm_bedroom_project(&view, points[k], screen);
                projected.vertices[k].x = screen[0];
                projected.vertices[k].y = screen[1];
            }
        }
        for (unsigned k = 0; k < 4; ++k) {
            BfmForestRasterVertex p = projected.vertices[k];
            outer[i+k] = (BfmForestRasterVertex){.x = p.x, .y = p.y};
            inner[i+k] = (BfmForestRasterVertex){.x = center.x + ring_fraction(p.x - center.x),
                .y = center.y + ring_fraction(p.y - center.y), .rgb = rgb};
        }
    }
    for (unsigned k = 0; k < 12; ++k) {
        unsigned next = (k + 1) % 12;
        cards[k] = (BfmForestDustCard){.vertices = {outer[k], outer[next], inner[k], inner[next]},
            .material = {.page = 32, .count = 4, .translucent = 1}, .bucket = bucket};
    }
    return 12;
}

struct BfmWoodsChargedFx {
    MusashiForestPlayerMesh *mesh;
    unsigned delay, duration, divisor;
    BfmForestSweep sweep;
    Wave wave[2];
    BfmChargedRing ring;
    int16_t center[3];
    int brightness, alive, testing;
};

BfmWoodsChargedFx *bfm_woods_charged_fx_open(const uint8_t *scene, size_t scene_bytes,
                                             const uint8_t *main, size_t main_bytes)
{
    const size_t timing = 0x8018a796u - 0x80128158u;
    const size_t length = 0x8018a7b7u - 0x80128158u;
    const size_t lights = 0x8018a3dcu - 0x80128158u;
    if (!scene || !main || scene_bytes <= length || !scene[timing] ||
        !scene[timing+1] || !scene[length]) return NULL;
    /* This pair is lit solely by its changing ambient channel. */
    for (unsigned i = 0; i < 3; ++i)
        if (scene[lights+16*i+12] || scene[lights+16*i+13] || scene[lights+16*i+14])
            return NULL;
    if (s16le(scene+lights+54) != -2) return NULL;
    BfmWoodsChargedFx *fx = calloc(1, sizeof *fx);
    if (!fx) return NULL;
    unsigned parts = 0;
    fx->mesh = musashi_forest_player_mesh_from_table(main, main_bytes, UINT32_C(0x800cedf8),
                                                     UINT32_C(0x800df2e0), &parts);
    const MusashiForestPlayerPart *part = musashi_forest_player_mesh_part(fx->mesh, 0);
    if (!part || parts != 1 || part->face_count != WAVE_FACES) goto fail;
    for (size_t i = 0; i < part->face_count; ++i)
        if (part->faces[i].mode != 0x32 || part->faces[i].corners != 3) goto fail;
    fx->delay = scene[timing];
    fx->duration = scene[timing+1];
    fx->divisor = scene[length];
    return fx;
fail:
    bfm_woods_charged_fx_free(fx);
    return NULL;
}

void bfm_woods_charged_fx_free(BfmWoodsChargedFx *fx)
{
    if (fx) musashi_forest_player_mesh_free(fx->mesh);
    free(fx);
}

static int step_sweep(BfmForestSweep *s, int active, const BfmForestSweepEdge *edge)
{
    if (!s->energy) return 1;
    if (s->delay && !active) {
        s->energy = 0;
        return 1;
    }
    if (s->delay > 1) {
        --s->delay;
        return 1;
    }
    if (!s->delay && (!active || s->duration <= 1))
        return bfm_forest_sweep_append(s, NULL);
    if (!edge) return 0;
    if (s->delay) s->delay = 0;
    else --s->duration;
    return bfm_forest_sweep_append(s, edge);
}

static void step_waves(BfmWoodsChargedFx *fx)
{
    if (!fx->alive) return;
    for (unsigned i = 0; i < 2; ++i) {
        Wave *w = &fx->wave[i];
        w->yaw = (w->yaw + (i ? -113 : 113)) & 4095;
        if (!w->fading) {
            w->growth += 512;
            w->vertical += w->growth;
            w->horizontal += i ? 512 : 256;
            w->fading = w->vertical > (i ? 8192 : 16384);
        } else if (!i) {
            w->horizontal += 256;
            fx->brightness -= 8;
            if (fx->brightness < -128) {
                fx->alive = 0;
                return;
            }
        } else {
            w->horizontal += 64;
            w->vertical = w->vertical > 512 ? w->vertical-512 : 0;
        }
    }
    fx->testing = 1;
}

int bfm_woods_charged_fx_step(BfmWoodsChargedFx *fx, int entry, int sync,
                              int swing_active, const BfmForestSweepEdge *edge)
{
    if (!fx || (sync && !edge)) return 0;
    if (entry)
        fx->sweep = (BfmForestSweep){.delay=fx->delay, .duration=fx->duration,
                                     .divisor=fx->divisor, .energy=244};
    else if (!step_sweep(&fx->sweep, swing_active, edge)) return 0;
    fx->testing = 0;
    if (sync) {
        memcpy(fx->center, edge->point[0], sizeof fx->center);
        Wave initial = {.horizontal=UNIT};
        fx->wave[0] = fx->wave[1] = initial;
        fx->brightness = 128;
        fx->alive = 1;
        ring_start(&fx->ring, fx->center);
    } else {
        step_waves(fx);
        ring_tick(&fx->ring);
    }
    return 1;
}

int bfm_woods_charged_fx_wave_center(const BfmWoodsChargedFx *fx, int16_t xyz[3])
{
    if (!fx || !fx->testing || !xyz) return 0;
    memcpy(xyz, fx->center, sizeof fx->center);
    return 1;
}

unsigned bfm_woods_charged_fx_wave_yaw(const BfmWoodsChargedFx *fx)
{
    return fx ? (unsigned)fx->wave[0].yaw : 0;
}

static int q12(int a, int b)
{
    int product = a*b;
    return product / UNIT - (product % UNIT < 0);
}

static size_t wave_project(const BfmWoodsChargedFx *fx, const Wave *wave,
                           const BfmForestCamera *camera, BfmForestDustCard *out)
{
    BfmForestTransform model;
    bfm_forest_actor_root((unsigned)wave->yaw, fx->center, &model);
    for (unsigned row = 0; row < 3; ++row)
        for (unsigned col = 0; col < 3; ++col)
            model.rotation[row][col] = (int16_t)q12(model.rotation[row][col],
                                            col == 1 ? wave->vertical : wave->horizontal);
    BfmForestCamera composed;
    if (!bfm_forest_effect_camera(camera, &model, &composed)) return 0;
    BfmBedroomView view = {.focal=composed.projection};
    for (unsigned a = 0; a < 3; ++a) {
        view.translation[a] = composed.translation[a];
        for (unsigned k = 0; k < 3; ++k) view.axes[a][k] = composed.rotation[a][k];
    }
    const MusashiForestPlayerPart *part = musashi_forest_player_mesh_part(fx->mesh,0);
    unsigned light = fx->brightness > 0 ? (unsigned)fx->brightness : 0;
    size_t count = 0;
    for (size_t i = 0; i < part->face_count; ++i) {
        const MusashiForestPlayerFace *face = &part->faces[i];
        BfmForestDustCard card = {.material={.page=32, .count=3, .translucent=1}};
        unsigned k, depth = 0;
        for (k = 0; k < 3; ++k) {
            const MusashiForestPlayerVector *v = &part->vertices[face->vertex[k]];
            const int16_t pos[3] = {v->x, v->y, v->z};
            int pixel[3];
            if (!bfm_bedroom_project(&view,pos,pixel) || pixel[2] <= view.focal / 2) break;
            depth += (unsigned)pixel[2];
            card.vertices[k].x = pixel[0];
            card.vertices[k].y = pixel[1];
            for (unsigned c = 0; c < 3; ++c)
                card.vertices[k].rgb |= (face->corner_rgb[k][c] * light / 256u) << (8*c);
        }
        if (k != 3) continue;
        const BfmForestRasterVertex *v = card.vertices;
        if ((v[1].x-v[0].x)*(v[2].y-v[0].y) <= (v[1].y-v[0].y)*(v[2].x-v[0].x)) continue;
        card.vertices[3] = v[2];
        card.bucket = depth * 341u / 4096u;
        card.slot = (unsigned)i;
        out[count++] = card;
    }
    return count;
}

size_t bfm_woods_charged_fx_project(const BfmWoodsChargedFx *fx, const BfmForestCamera *camera,
                                   BfmForestDustCard cards[BFM_WOODS_CHARGED_CARDS])
{
    if (!fx || !camera || !cards) return 0;
    size_t count = bfm_forest_sweep_project(&fx->sweep,camera,cards);
    /* Lumina's ribbon uses the opposite red and blue channels from Fusion. */
    for (size_t i = 0; i < count; ++i) {
        for (unsigned k = 0; k < 4; ++k) {
            uint32_t rgb = cards[i].vertices[k].rgb;
            cards[i].vertices[k].rgb = (rgb >> 16) | (rgb & 0xff00) | ((rgb & 255) << 16);
        }
    }
    if (fx->alive) {
        for (unsigned i = 0; i < 2; ++i)
            count += wave_project(fx, &fx->wave[i], camera,cards+count);
    }
    return count + ring_project(&fx->ring,camera,cards+count);
}
