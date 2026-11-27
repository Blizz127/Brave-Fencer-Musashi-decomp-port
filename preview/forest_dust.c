/* SPDX-License-Identifier: MIT */
#include "forest_dust.h"

#include "../pc_port/forest_player_archive.h"
#include "../pc_port/forest_player_bytes.h"
#include <math.h>

static int32_t floor_units(int64_t n, unsigned bits)
{
    int64_t unit = INT64_C(1) << bits;
    return (int32_t)(n >= 0 ? n / unit : -((-n + unit - 1) / unit));
}

int bfm_forest_dust_load(BfmForestDust *dust, const MusashiDiscMedia *disc)
{
    ForestPlayerArchive archive;
    uint8_t data[12 + 8 * BFM_FOREST_DUST_CELLS];
    if (!dust || !forest_player_archive_open(disc, &archive) ||
        !forest_player_archive_read(&archive, UINT32_C(0x800d387c), data, sizeof data))
        return 0;
    BfmForestDust parsed = {0};
    parsed.width = data[2];
    parsed.height = data[3];
    parsed.left = s16le(data + 8);
    parsed.top = s16le(data + 10);
    if (!parsed.width || !parsed.height) return 0;
    for (unsigned i = 0; i < BFM_FOREST_DUST_CELLS; ++i) {
        const uint8_t *entry = data + 12 + 8 * i;
        unsigned x = u16le(entry), y = u16le(entry + 2);
        unsigned u = (x % 64) * 4, v = y % 256;
        unsigned end = i + 1 == BFM_FOREST_DUST_CELLS ? 192 : 0;
        if (x >= 1024 || y >= 512 || u + parsed.width > 256 ||
            v + parsed.height > 256 || u32le(entry + 4) != (end | 1u))
            return 0;
        parsed.page[i] = (uint16_t)(x / 64 | (y / 256) << 4 | 1u << 5);
        parsed.u[i] = (uint8_t)u;
        parsed.v[i] = (uint8_t)v;
    }
    *dust = parsed;
    return 1;
}

static void advance(BfmForestDustParticle *p, int damp)
{
    for (unsigned i = 0; i < 3; ++i)
        p->position[i] += p->velocity[i];
    if (damp) {
        p->velocity[0] = floor_units((int64_t)p->velocity[0] * 3, 2);
        p->velocity[2] = floor_units((int64_t)p->velocity[2] * 3, 2);
    }
    ++p->cell;
}

int bfm_forest_foot_dust_start(BfmForestDustParticle *p, BfmForestDustEmission event,
                               const int16_t player[3], const int32_t eye[3], int yaw)
{
    if (!p || !player || !eye ||
        (event != BFM_FOREST_DUST_TRAIL && event != BFM_FOREST_DUST_BRAKE)) return 0;

    /* Shift the spawn toward the field camera by 64 world units, using the
     * same quantized normalization as the field camera's fixed-point axes. */
    int64_t delta[3];
    uint64_t magnitude = 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        delta[axis] = (int64_t)eye[axis] - player[axis];
        magnitude += (uint64_t)(delta[axis] * delta[axis]);
    }
    if (!magnitude) return 0;
    p->cell = 0;
    unsigned shift = 9;
    while (magnitude > 255) { magnitude >>= 2; ++shift; }
    int gain = (int)sqrt((double)((UINT32_C(1) << 30) / magnitude));
    for (unsigned axis = 0; axis < 3; ++axis) {
        int offset = floor_units(delta[axis] * gain, shift);
        p->position[axis] = ((int64_t)player[axis] + offset) * 65536;
    }
    double angle = ((unsigned)yaw & 4095u) * (6.28318530717958647693 / 4096);
    int direction = event == BFM_FOREST_DUST_TRAIL ? 256 : -256;
    p->velocity[0] = (int32_t)lround(4096 * sin(angle)) * direction;
    p->velocity[1] = -65536;
    p->velocity[2] = (int32_t)lround(4096 * cos(angle)) * direction;
    return 1;
}

int bfm_forest_foot_dust_advance(BfmForestDustParticle *p)
{
    if (!p) return 0;
    if (p->cell == BFM_FOREST_DUST_CELLS - 1) {
        p->cell = 0;
        return 0;
    }
    advance(p, 0);
    /* The room rounds the half and quarter velocities separately. */
    for (unsigned i = 0; i < 3; i += 2)
        p->velocity[i] = floor_units(p->velocity[i],1) + floor_units(p->velocity[i],2);
    return 1;
}

void bfm_forest_dust_tick(BfmForestDust *dust, BfmForestDustEmission event,
                          const int16_t player[3], const int32_t eye[3], int yaw)
{
    if (!dust || !dust->width) return;
    BfmForestDustParticle *available = NULL;
    for (unsigned i = 0; i < BFM_FOREST_FOOT_DUST_CARDS; ++i) {
        BfmForestDustParticle *p = &dust->particles[i];
        if (p->cell == BFM_FOREST_DUST_CELLS - 1) p->cell = 0;
        else if (p->cell) advance(p, 1);
        if (!p->cell && !available) available = p;
    }
    if (available && bfm_forest_foot_dust_start(available,event,player,eye,yaw))
        advance(available, 1);
}

void bfm_forest_dust_recoil_start(BfmForestDust *dust, const int32_t root[3], int attacker_yaw)
{
    if (!dust || !dust->width || !root) return;
    BfmForestDustParticle *p = &dust->particles[BFM_FOREST_FOOT_DUST_CARDS];
    double angle = ((unsigned)attacker_yaw & 4095u) * (6.28318530717958647693 / 4096);
    for (unsigned axis = 0; axis < 3; ++axis) p->position[axis] = (int64_t)root[axis] * 65536;
    p->velocity[0] = (int32_t)lround(4096 * sin(angle)) * -256;
    p->velocity[1] = 0;
    p->velocity[2] = (int32_t)lround(4096 * cos(angle)) * -256;
    p->cell = 0;
    advance(p, 0);
}

void bfm_forest_dust_recoil_advance(BfmForestDust *dust)
{
    if (!dust) return;
    BfmForestDustParticle *p = &dust->particles[BFM_FOREST_FOOT_DUST_CARDS];
    if (p->cell == BFM_FOREST_DUST_CELLS - 1) p->cell = 0;
    else if (p->cell) advance(p, 0);
}

static int project_particle(const BfmForestDust *dust, const BfmForestDustParticle *p,
                            const BfmForestCamera *camera, unsigned slot, int size,
                            BfmForestDustCard *card)
{
    if (!p->cell || p->cell >= BFM_FOREST_DUST_CELLS) return 0;
    int32_t world[3];
    int64_t view[3];
    for (unsigned axis = 0; axis < 3; ++axis)
        world[axis] = floor_units(p->position[axis], 16);
    for (unsigned axis = 0; axis < 3; ++axis) {
        const int16_t *r = camera->rotation[axis];
        int64_t dot = (int64_t)r[0] * world[0] + (int64_t)r[1] * world[1] +
                      (int64_t)r[2] * world[2];
        view[axis] = (int64_t)floor_units(dot, 12) + camera->translation[axis];
    }
    if (view[2] <= 0 || view[2] / 4 + 1 >= 4096 ||
        view[0] < INT16_MIN || view[0] > INT16_MAX ||
        view[1] < INT16_MIN || view[1] > INT16_MAX) return 0;
    int z = (int)view[2];
    int64_t reciprocal = ((int64_t)camera->projection * 65536 + z / 2) / z;
    if (reciprocal > 131071) reciprocal = 131071;
    int scale = (int)(size * reciprocal / 16);
    int x = 160 + floor_units(view[0] * reciprocal, 16) +
                  floor_units((int64_t)dust->left * scale, 12);
    int y = 120 + floor_units(view[1] * reciprocal, 16) +
                  floor_units((int64_t)dust->top * scale, 12);
    int width = dust->width * scale / 4096;
    int height = dust->height * scale / 4096;
    card->bucket = (unsigned)(z / 4 + 1);
    card->slot = slot;
    card->material = (BfmForestRasterMaterial){
        .page = dust->page[p->cell], .clut = (412u << 6) | 352u / 16,
        .count = 4, .textured = 1, .translucent = 1
    };
    for (unsigned vertex = 0; vertex < 4; ++vertex) {
        int right = vertex & 1, bottom = (vertex >> 1) & 1;
        card->vertices[vertex] = (BfmForestRasterVertex){
            x + right * width, y + bottom * height,
            (uint8_t)(dust->u[p->cell] + right * (dust->width - 1)),
            (uint8_t)(dust->v[p->cell] + bottom * (dust->height - 1)), 0x808080
        };
    }
    return 1;
}

int bfm_forest_foot_dust_project(const BfmForestDust *dust, const BfmForestDustParticle *p,
                                const BfmForestCamera *camera, unsigned slot, BfmForestDustCard *card)
{
    return dust && p && camera && camera->projection && card &&
           project_particle(dust,p,camera,slot,2,card);
}

unsigned bfm_forest_dust_project(const BfmForestDust *dust, const BfmForestCamera *camera,
                                 BfmForestDustCard out[BFM_FOREST_FOOT_DUST_CARDS])
{
    if (!dust || !camera || !out || !camera->projection) return 0;
    unsigned count = 0;
    for (unsigned i = 0; i < BFM_FOREST_FOOT_DUST_CARDS; ++i)
        count += project_particle(dust, &dust->particles[i], camera, i, 2, &out[count]);
    return count;
}

unsigned bfm_forest_dust_recoil_project(const BfmForestDust *dust, const BfmForestCamera *camera,
                                        BfmForestDustCard *out)
{
    unsigned i = BFM_FOREST_FOOT_DUST_CARDS;
    return dust && camera && camera->projection && out &&
           project_particle(dust, &dust->particles[i], camera, i, 3, out);
}
