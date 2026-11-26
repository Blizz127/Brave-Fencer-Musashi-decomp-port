/* SPDX-License-Identifier: MIT */
#include "tower_interior_fixtures.h"
#include "../pc_port/forest_player_bytes.h"
#include <math.h>

static const uint8_t *record(const uint8_t *data, size_t size, uint32_t address, size_t length)
{
    const uint32_t origin = UINT32_C(0x80128158);
    if (address < origin) return NULL;
    size_t offset = address - origin;
    return offset <= size && length <= size - offset ? data + offset : NULL;
}

int bfm_tower_interior_fixtures_load(const uint8_t *scene, size_t bytes,
                                     BfmTowerInteriorFixtures *fixtures)
{
    if (!scene || !fixtures) return 0;
    BfmTowerInteriorFixtures loaded = {0};
    const uint8_t *placements = record(scene, bytes, UINT32_C(0x801a3060) + 180, 4);
    const uint8_t *sprites = record(scene, bytes, UINT32_C(0x8018abf8), 24);
    if (!placements || !sprites) return 0;
    const uint8_t *row = record(scene, bytes, u32le(placements), 20);
    if (!row) return 0;
    for (;;) {
        if ((size_t)(scene + bytes - row) < 20) return 0;
        unsigned kind = u16le(row + 6);
        if (!kind) break;
        if (kind == 21) {
            if (loaded.count == BFM_TOWER_HANGING_LIGHTS) return 0;
            BfmTowerLight *light = &loaded.light[loaded.count++];
            for (unsigned axis = 0; axis < 3; ++axis) light->root[axis] = s16le(row + 2*axis);
            light->style = u16le(row + 8) & 1u;
        }
        row += 20;
    }
    if (loaded.count != BFM_TOWER_HANGING_LIGHTS) return 0;
    for (unsigned i = 0; i < 2; ++i) {
        const uint8_t *image = sprites + 12*i;
        unsigned x = u16le(image + 4), y = u16le(image + 6);
        unsigned u = 4*(x % 64), v = y % 256;
        if (image[0] != 255 || !image[2] || !image[3] || x >= 1024 || y >= 512 ||
            u + image[2] > 256 || v + image[3] > 256 ||
            s16le(image+8) < INT16_MIN/3 || s16le(image+8) > INT16_MAX/3 ||
            s16le(image+10) < INT16_MIN/3 || s16le(image+10) > INT16_MAX/3) return 0;
        loaded.sprite[i] = (BfmTowerLightSprite){
            .u=(uint8_t)u, .v=(uint8_t)v, .width=image[2], .height=image[3],
            .left=s16le(image + 8), .top=s16le(image + 10),
            .ink={.count=4, .textured=1, .page=(uint16_t)(x/64 | (y/256)<<4),
                  .clut=(uint16_t)((256u+image[1])*64u + (image[1]<224 ? 352u : 256u)/16u)}
        };
    }
    *fixtures = loaded;
    return 1;
}

void bfm_tower_interior_fixtures_start(BfmTowerInteriorFixtures *fixtures, BfmGameplayRun *run)
{
    if (!fixtures || !run || fixtures->count != BFM_TOWER_HANGING_LIGHTS || fixtures->started) return;
    for (unsigned i = 0; i < fixtures->count; ++i)
        fixtures->light[i].phase = bfm_gameplay_random(run) & 4095;
    fixtures->started = 1;
}

static int64_t floor_q12(int64_t value)
{
    return value/4096 - (value%4096 < 0);
}

static int body(const BfmTowerLightSprite *sprite, const int16_t root[3],
                const BfmForestCamera *camera, BfmForestDustCard *card)
{
    int16_t position[4][3] = {{0}};
    for (unsigned row = 0; row < 3; ++row) {
        int64_t sum = 0;
        for (unsigned j = 0; j < 3; ++j) sum += (int64_t)camera->rotation[row][j] * root[j];
        int64_t coordinate = camera->translation[row] + floor_q12(sum);
        if (coordinate < INT16_MIN || coordinate > INT16_MAX) return 0;
        position[0][row] = (int16_t)coordinate;
    }
    /* The lamp's billboard is three times the authored sprite size. */
    position[1][0] = 3*sprite->left;
    position[1][1] = 3*sprite->top;
    position[2][0] = 3*sprite->width;
    position[2][1] = 3*sprite->height;
    for (unsigned i = 1; i < 4; ++i) position[i][2] = position[0][2];
    BfmForestCamera plane = {.rotation={{4096,0,0},{0,4096,0},{0,0,4096}},
                             .projection=camera->projection};
    BfmForestDustCard quad = {0};
    if (!bfm_forest_project_effect_quad(&plane, position, &quad) || ++quad.bucket >= 4096) return 0;
    int ox = quad.vertices[3].x, oy = quad.vertices[3].y;
    int x = quad.vertices[0].x + quad.vertices[1].x - ox;
    int y = quad.vertices[0].y + quad.vertices[1].y - oy;
    int w = quad.vertices[2].x - ox, h = quad.vertices[2].y - oy;
    if (w < 1 || h < 1 || x >= 320 || y >= 240 || x+w <= 0 || y+h <= 0) return 0;
    quad.material = sprite->ink;
    for (unsigned i = 0; i < 4; ++i) {
        int right = i%2, bottom = i/2;
        quad.vertices[i] = (BfmForestRasterVertex){x+right*w, y+bottom*h,
            (uint8_t)(sprite->u+right*(sprite->width-1)),
            (uint8_t)(sprite->v+bottom*(sprite->height-1)), 0x808080};
    }
    *card = quad;
    return 1;
}

static size_t chain(const int16_t root[3], const BfmForestCamera *camera,
                    BfmForestDustCard *cards, size_t capacity)
{
    const BfmForestRasterMaterial links = {.count=4, .textured=1, .page=28, .clut=332*64+352/16};
    size_t count = 0;
    for (int y = root[1]; y >= INT16_MIN+64 && count < capacity; y -= 64) {
        int16_t line[4][3] = {{root[0],y-64,root[2]}, {root[0],y-64,root[2]},
                              {root[0],y,root[2]}, {root[0],y-64,root[2]}};
        BfmForestDustCard quad = {0};
        if (!bfm_forest_project_effect_quad(camera, line, &quad) ||
            quad.bucket <= camera->projection/8 || quad.bucket >= 4096) break;
        BfmForestRasterVertex top = quad.vertices[0], bottom = quad.vertices[2];
        if (bottom.y < 1) break;
        if (top.y >= 240) continue;
        int width = 8*(int)camera->projection / (4*(int)quad.bucket);
        if (width < 2 || (top.x+width < 0 && bottom.x+width < 0) ||
            (top.x-width >= 320 && bottom.x-width >= 320)) continue;
        quad.material = links;
        for (unsigned i = 0; i < 4; ++i) {
            const BfmForestRasterVertex *p = i < 2 ? &top : &bottom;
            quad.vertices[i] = (BfmForestRasterVertex){p->x + ((i%2) ? width : -width),
                p->y, (uint8_t)(240 + 7*(i%2)), (uint8_t)(63*(i/2)), 0x808080};
        }
        cards[count++] = quad;
    }
    return count;
}

size_t bfm_tower_interior_fixture_cards(const BfmTowerInteriorFixtures *fixtures, unsigned age,
                                        const BfmForestCamera *camera, BfmForestDustCard *out,
                                        size_t capacity)
{
    if (!fixtures || !fixtures->started || !age || !camera || !out) return 0;
    size_t count = 0;
    for (unsigned i = 0; i < fixtures->count && count < capacity; ++i) {
        const BfmTowerLight *light = &fixtures->light[i];
        int16_t root[3] = {light->root[0], light->root[1], light->root[2]};
        unsigned phase = (light->phase + 16u*age) & 4095;
        int sine = (int)lround(4096 * sin(phase * (6.28318530717958647693 / 4096)));
        int y = root[1] + (int)floor_q12(32 * sine);
        if (y < INT16_MIN || y > INT16_MAX) continue;
        root[1] = (int16_t)y;
        count += body(&fixtures->sprite[light->style], root, camera, out + count);
        count += chain(root, camera, out + count, capacity - count);
    }
    return count;
}
