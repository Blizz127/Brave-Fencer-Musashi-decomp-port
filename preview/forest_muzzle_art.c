/* SPDX-License-Identifier: MIT */
#include "forest_muzzle_art.h"
#include "../pc_port/forest_player_bytes.h"
#include <limits.h>
#include <string.h>

static int down(int64_t n, int unit)
{
    return (int)(n / unit - (n % unit < 0));
}

/* This model uses the unlit sorter. Each flat-normal triangle or quad has
 * independent vertex inks; its normals never enter the lighting pipeline. */
static int decode(BfmForestMuzzleArt *art, const uint8_t *data, size_t bytes)
{
    if (bytes < 40 || u32le(data) != 0x41 || u32le(data + 4) || u32le(data + 8) != 1 ||
        u32le(data + 16) != BFM_FOREST_MUZZLE_POINTS ||
        u32le(data + 32) != BFM_FOREST_MUZZLE_FACES || u32le(data + 36)) return 0;
    const uint8_t *body = data + 12;
    size_t size = bytes - 12, at = u32le(body + 16), vertices = u32le(body);
    size_t normals = u32le(body + 8), normal_count = u32le(body + 12);
    if (at != 28 || at >= vertices || vertices > size ||
        BFM_FOREST_MUZZLE_POINTS * 8u > size - vertices ||
        normals != vertices + BFM_FOREST_MUZZLE_POINTS * 8u ||
        normal_count != (size - normals) / 8 || (size - normals) % 8) return 0;
    for (unsigned k = 0; k < BFM_FOREST_MUZZLE_POINTS; ++k) {
        for (unsigned axis = 0; axis < 3; ++axis)
            art->points[k][axis] = s16le(body + vertices + k * 8 + axis * 2);
    }
    for (unsigned f = 0; f < BFM_FOREST_MUZZLE_FACES; ++f) {
        if (vertices - at < 4) return 0;
        const uint8_t *p = body + at;
        unsigned n = p[3] == 0x22 ? 3 : p[3] == 0x2a ? 4 : 0;
        size_t length = 8u * n;
        if (!n || length > vertices - at || p[0] != 2*n || p[1] != 2*n-1 ||
            p[2] != 4 || (p[7] & 2) == 0) return 0;
        const uint8_t *indices = p + 4 * (n + 1);
        if (u16le(indices) >= normal_count) return 0;
        BfmForestMuzzleFace *face = &art->faces[f];
        face->count = n;
        for (unsigned k = 0; k < n; ++k) {
            face->vertex[k] = u16le(indices + 2 + 2*k);
            if (face->vertex[k] >= BFM_FOREST_MUZZLE_POINTS) return 0;
            face->rgb[k] = u32le(p + 4 + 4*k) & UINT32_C(0xffffff);
        }
        at += length;
    }
    return at == vertices;
}

int bfm_forest_muzzle_load(BfmForestMuzzleArt *art, const BfmForestAward *award)
{
    const uint32_t table_address = UINT32_C(0x800cc870);
    const uint8_t *table = bfm_forest_gunshot_bytes(award, table_address, 8);
    if (!art || !table || u32le(table + 4)) return 0;
    uint32_t model = u32le(table);
    if (model >= table_address || (model & 3)) return 0;
    size_t bytes = table_address - model;
    const uint8_t *data = bfm_forest_gunshot_bytes(award, model, bytes);
    BfmForestMuzzleArt loaded = {0};
    if (!data || !bfm_forest_bullet_art_load(&loaded.fan, award) || !decode(&loaded, data, bytes))
        return 0;
    *art = loaded;
    return 1;
}

int bfm_forest_muzzle_fire(BfmForestMuzzleArt *art, const BfmForestTransform *hero,
                           unsigned random, int16_t emitter[3])
{
    if (!art || !art->fan.loaded || !hero || !emitter) return 0;
    const int16_t zero[3] = {0};
    BfmForestTransform turn;
    bfm_forest_actor_root(random & 4095, zero, &turn);
    int sine = down(turn.rotation[0][2] * 6144, 4096);
    int minus_sine = down(-turn.rotation[0][2] * 6144, 4096);
    int cosine = down(turn.rotation[0][0] * 6144, 4096);
    /* The emitter uses a single rigid mesh on the resident identity pose. */
    BfmForestTransform fan = *hero, model = *hero;
    for (unsigned k = 0; k < 3; ++k) {
        int x = hero->rotation[k][0], y = hero->rotation[k][1], z = hero->rotation[k][2];
        int64_t center = (int64_t)hero->translation[k] + down(-6*y - 51*z, 4096);
        int64_t origin = (int64_t)hero->translation[k] + down(-24*y - 15*z, 4096);
        if (center < INT16_MIN || center > INT16_MAX || origin < INT16_MIN || origin > INT16_MAX)
            return 0;
        fan.translation[k] = (int32_t)center;
        model.translation[k] = (int32_t)origin;
        model.rotation[k][0] = (int16_t)down((int64_t)x*cosine + (int64_t)y*sine, 4096);
        model.rotation[k][1] = (int16_t)down((int64_t)x*minus_sine + (int64_t)y*cosine, 4096);
        model.rotation[k][2] = (int16_t)down(z*6144, 4096);
    }
    for (unsigned k = 0; k < 3; ++k) emitter[k] = (int16_t)model.translation[k];
    art->fan.flash = fan;
    art->model = model;
    art->light = 128;
    return 1;
}

void bfm_forest_muzzle_tick(BfmForestMuzzleArt *art)
{
    if (art) art->light = art->light == 128 ? 64 : 0;
}

size_t bfm_forest_muzzle_project(const BfmForestMuzzleArt *art, const BfmForestCamera *camera,
                                 BfmForestDustCard *cards, size_t capacity)
{
    BfmForestCamera local;
    if (!art || !art->light || !cards || !capacity ||
        !bfm_forest_effect_camera(camera, &art->model, &local)) return 0;
    unsigned gray = art->light * UINT32_C(0x010101);
    size_t drawn = bfm_forest_bullet_fan_project(art->fan.perimeter, &art->fan.flash, 1,
                                                 gray, camera, cards, capacity);
    for (unsigned i = 0; i < BFM_FOREST_MUZZLE_FACES && drawn < capacity; ++i) {
        const BfmForestMuzzleFace *face = &art->faces[i];
        BfmForestDustCard card = {.material = {.page=32, .translucent=1, .count=face->count}};
        int16_t xyz[4][3];
        unsigned depth_sum = 0;
        unsigned j = 0;
        for (; j < face->count; ++j) {
            memcpy(xyz[j], art->points[face->vertex[j]], sizeof xyz[j]);
            card.vertices[j].rgb = face->rgb[j];
            int64_t depth = 0;
            for (unsigned axis = 0; axis < 3; ++axis)
                depth += (int64_t)xyz[j][axis] * local.rotation[2][axis];
            int64_t z = (int64_t)down(depth,4096) + local.translation[2];
            if (z <= 0 || z > INT16_MAX) break;
            depth_sum += (unsigned)z;
        }
        if (j != face->count) continue;
        if (face->count == 3) memcpy(xyz[3], xyz[2], sizeof xyz[3]);
        if (!bfm_forest_project_effect_quad(&local, xyz, &card)) continue;
        const BfmForestRasterVertex *v = card.vertices;
        int area = (v[1].x-v[0].x)*(v[2].y-v[0].y) - (v[2].x-v[0].x)*(v[1].y-v[0].y);
        if (area <= 0) continue;
        card.bucket = depth_sum * (face->count == 3 ? 341u : 256u) / 4096u;
        cards[drawn++] = card;
    }
    return drawn;
}
