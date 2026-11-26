/* SPDX-License-Identifier: MIT */
#include "tower_bell_impact.h"
#include "../pc_port/forest_player_bytes.h"
#include <limits.h>

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t len)
{
    const uint32_t origin = UINT32_C(0x80128158);
    if (!scene || address < origin) return NULL;
    size_t offset = address - origin;
    return offset <= bytes && len <= bytes - offset ? scene + offset : NULL;
}

static int image(const uint8_t *shape, const uint8_t *texture, unsigned palette, BfmBellImage *out)
{
    if (!shape || shape[0] != 255 || !shape[2] || !shape[3]) return 0;
    unsigned x = u16le(texture), y = u16le(texture + 2);
    unsigned u = (x % 64)*4, v = y % 256;
    int left = s16le(shape + 8), top = s16le(shape + 10);
    if (x >= 1024 || y >= 512 || u + shape[2] > 256 || v + shape[3] > 256 ||
        left < INT16_MIN/4 || left + shape[2] > INT16_MAX/4 ||
        top < INT16_MIN/4 || top + shape[3] > INT16_MAX/4) return 0;
    *out = (BfmBellImage){
        .page=(uint16_t)(32 | x/64 | (y/256)<<4),
        .clut=(uint16_t)((256+palette)*64 + (palette < 224 ? 352 : 256)/16),
        .u=(uint8_t)u, .v=(uint8_t)v, .width=shape[2], .height=shape[3],
        .left=(int16_t)left, .top=(int16_t)top
    };
    return 1;
}

int bfm_tower_bell_impact_load(const uint8_t *scene, size_t bytes, BfmTowerBellImpact *out)
{
    const uint8_t *loop = mounted(scene, bytes, UINT32_C(0x8018a838), 13*8);
    const uint8_t *shape = mounted(scene, bytes, UINT32_C(0x8018a8a8), 12);
    const uint8_t *burst = mounted(scene, bytes, UINT32_C(0x8018a8b4), 6*8);
    if (!out || !loop || !shape || !burst || u32le(loop+96) || u16le(loop+100) != UINT16_MAX)
        return 0;
    BfmTowerBellImpact art = {0};
    for (unsigned i = 0; i < 12; ++i) {
        const uint8_t *cell = mounted(scene, bytes, u32le(loop+8*i), 12);
        if (!cell || u32le(loop+8*i+4) != 2 || !image(cell, cell+4, cell[1], &art.flame[i]))
            return 0;
    }
    for (unsigned i = 0; i < 6; ++i) {
        const uint8_t *cell = burst + 8*i;
        if (u32le(cell+4) != (i == 5 ? 194u : 2u) || !image(shape, cell, 158, &art.ring[i]))
            return 0;
    }
    *out = art;
    return 1;
}

static int64_t floor_q12(int64_t n)
{
    return n/4096 - (n%4096 < 0);
}

static int flame_quad(const BfmBellImage *image, const BfmForestCamera *camera,
                      BfmForestDustCard *card)
{
    /* Billboard dimensions and pivot are projected separately at the
     * center's depth by the room sprite sorter. */
    int16_t points[4][3] = {{0}};
    for (unsigned i = 0; i < 3; ++i) {
        int64_t n = camera->translation[i] + floor_q12(-938 * (int64_t)camera->rotation[i][1]);
        if (n < INT16_MIN || n > INT16_MAX) return 0;
        points[0][i] = (int16_t)n;
    }
    points[1][0] = 3*image->left;
    points[1][1] = 3*image->top;
    points[2][0] = 3*image->width;
    points[2][1] = 3*image->height;
    for (unsigned i = 1; i < 4; ++i) points[i][2] = points[0][2];
    BfmForestCamera plane = {.rotation={{4096,0,0},{0,4096,0},{0,0,4096}},
                             .projection=camera->projection};
    if (!bfm_forest_project_effect_quad(&plane, points, card)) return 0;
    int ox = card->vertices[3].x, oy = card->vertices[3].y;
    int x = card->vertices[0].x + card->vertices[1].x - ox;
    int y = card->vertices[0].y + card->vertices[1].y - oy;
    int w = card->vertices[2].x - ox, h = card->vertices[2].y - oy;
    if (w <= 0 || h <= 0 || x >= 320 || x+w <= 0 || y >= 240 || y+h <= 0) return 0;
    for (unsigned i = 0; i < 4; ++i) {
        card->vertices[i].x = x + (int)(i%2)*w;
        card->vertices[i].y = y + (int)(i/2)*h;
    }
    return ++card->bucket < 4096;
}

static int ring_quad(const BfmBellImage *image, const BfmForestCamera *camera,
                     BfmForestDustCard *card)
{
    /* The ring is a four-times-scale horizontal plane, 48 units above
     * the flame. Its ordering depth averages all four projected corners. */
    int16_t points[4][3];
    int sum = 0;
    for (unsigned i = 0; i < 4; ++i) {
        points[i][0] = 4*(image->left + (int)(i%2)*image->width);
        points[i][1] = -986;
        points[i][2] = 4*(image->top + (int)(i/2)*image->height);
        int64_t dot = 0;
        for (unsigned j = 0; j < 3; ++j)
            dot += (int64_t)camera->rotation[2][j]*points[i][j];
        int64_t z = camera->translation[2] + floor_q12(dot);
        if (z <= 0 || z >= 16384) return 0;
        sum += (int)z;
    }
    if (!bfm_forest_project_effect_quad(camera, points, card)) return 0;
    card->bucket = (unsigned)sum/16 + 1;
    return card->bucket < 4096;
}

int bfm_tower_bell_impact_card(const BfmTowerBellImpact *art, unsigned room_age,
                               unsigned room_phase, unsigned phase_age,
                               const BfmForestCamera *camera, BfmForestDustCard *out)
{
    if (!art || !camera || !out || room_phase > 6 || !art->flame[0].width) return 0;
    BfmForestDustCard card = {0};
    const BfmBellImage *cell;
    /* The displayed card trails the room director by one update. The
     * impact-edge picture still shows the incoming flame. Each return
     * edge follows a spent ring and starts with an empty picture. */
    if (room_phase && room_phase%2 == 0 && phase_age) {
        if (phase_age > 12) return 0;
        cell = &art->ring[(phase_age-1)/2];
        if (!ring_quad(cell, camera, &card)) return 0;
    } else {
        if (room_phase >= 3 && room_phase%2 && !phase_age) return 0;
        cell = &art->flame[(room_age/2)%12];
        if (!flame_quad(cell, camera, &card)) return 0;
    }
    card.material = (BfmForestRasterMaterial){.count=4, .textured=1, .translucent=1,
                                               .page=cell->page, .clut=cell->clut};
    for (unsigned i = 0; i < 4; ++i) {
        card.vertices[i].u = (uint8_t)(cell->u + (i%2)*(cell->width-1));
        card.vertices[i].v = (uint8_t)(cell->v + (i/2)*(cell->height-1));
        card.vertices[i].rgb = UINT32_C(0x808080);
    }
    *out = card;
    return 1;
}
