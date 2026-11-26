/* SPDX-License-Identifier: MIT */
#include "steam_knight_flames.h"
#include "../pc_port/forest_player_bytes.h"
#include <math.h>
#include <string.h>

enum { GROWTH_TICKS = 16, RING_RADIUS = 88 };

/* Both sprite programs and the collision shape are data in the mounted hall. */
static const uint8_t *room_data(const uint8_t *scene, size_t bytes, uint32_t address, size_t n)
{
    const uint32_t origin = UINT32_C(0x80128158);
    if (address < origin || (address & 3) || n > bytes || address - origin > bytes - n)
        return NULL;
    return scene + (address - origin);
}

static int read_clip(const uint8_t *scene, size_t bytes, uint32_t address,
                     BfmSteamFlameSprite *frames, unsigned count, unsigned ending)
{
    const uint8_t *list = room_data(scene, bytes, address, 8 * (count + 1));
    if (!list || u32le(list + 8 * count) || u32le(list + 8 * count + 4) != ending)
        return 0;
    for (unsigned i = 0; i < count; ++i) {
        const uint8_t *entry = list + 8 * i;
        const uint8_t *s = room_data(scene, bytes, u32le(entry), 12);
        if (u32le(entry + 4) != 1 || !s || s[0] != 255) return 0;
        unsigned x = u16le(s + 4), y = u16le(s + 6);
        unsigned u = (x % 64) * 4, v = y % 256;
        if (x >= 1024 || y >= 512 || !s[2] || !s[3] || s[2] == 255 || s[3] == 255 ||
            u + s[2] > 255 || v + s[3] > 255) return 0;
        frames[i] = (BfmSteamFlameSprite){
            .page = (uint16_t)(x / 64 | (y / 256) << 4 | 1u << 5),
            .clut = (uint16_t)(((256u + s[1]) << 6) | 352u / 16),
            .u = (uint8_t)u, .v = (uint8_t)v,
            .width = (uint8_t)(s[2] + 1), .height = (uint8_t)(s[3] + 1),
            .left = s16le(s + 8), .top = s16le(s + 10)
        };
    }
    return 1;
}

int bfm_steam_knight_flames_load(BfmSteamKnightFlames *flames, const uint8_t *scene, size_t bytes)
{
    BfmSteamKnightFlames loaded = {0};
    if (!flames || !scene ||
        !read_clip(scene, bytes, UINT32_C(0x80196a60), loaded.burning, 16, 0xffff) ||
        !read_clip(scene, bytes, UINT32_C(0x80196ae8), loaded.falling, 8, 0xfffe))
        return 0;
    const uint8_t *shape = room_data(scene, bytes, UINT32_C(0x80196b58), 16);
    if (!shape || u32le(shape)) return 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        loaded.bounds.low[axis] = s16le(shape + 4 + 4 * axis);
        loaded.bounds.high[axis] = s16le(shape + 6 + 4 * axis);
        if (loaded.bounds.high[axis] <= loaded.bounds.low[axis]) return 0;
    }
    *flames = loaded;
    return 1;
}

void bfm_steam_knight_flames_start(BfmSteamKnightFlames *flames, const int16_t center[3])
{
    if (!flames || !flames->burning[0].width || !center) return;
    memcpy(flames->center, center, sizeof flames->center);
    flames->growth = 0;
    flames->frame = 14;
    flames->remaining = BFM_STEAM_FLAMES;
    for (unsigned i = 0; i < BFM_STEAM_FLAMES; ++i) {
        flames->children[i].lit = 1;
        flames->children[i].fade = 0;
        /* Each child is already at its own position on the first visible field. */
        double angle = (i + 0.5) * (6.28318530717958647693 / BFM_STEAM_FLAMES);
        flames->children[i].at[0] = center[0] + (int)floor(RING_RADIUS * sin(angle));
        flames->children[i].at[1] = center[1];
        flames->children[i].at[2] = center[2] + (int)floor(RING_RADIUS * cos(angle));
    }
}

void bfm_steam_knight_flames_tick(BfmSteamKnightFlames *flames, const BfmSteamFlameRay *rays,
                                  size_t ray_count)
{
    if (!flames || !flames->remaining) return;
    flames->frame = (flames->frame + 1) % 16;
    if (flames->growth < GROWTH_TICKS) ++flames->growth;
    for (unsigned i = 0; i < BFM_STEAM_FLAMES; ++i) {
        if (!flames->children[i].lit) continue;
        if (flames->children[i].fade) {
            if (++flames->children[i].fade == 8) {
                flames->children[i].lit = 0;
                --flames->remaining;
            }
            continue;
        }
        if (!rays) continue;
        BfmForestTransform root = {.rotation = {{4096,0,0},{0,4096,0},{0,0,4096}}};
        memcpy(root.translation, flames->children[i].at, sizeof root.translation);
        for (size_t j = 0; j < ray_count; ++j) {
            int16_t hit[3];
            if (bfm_forest_guard_contact(&flames->bounds, &root, rays[j].from, rays[j].to, hit)) {
                flames->children[i].fade = 1;
                break;
            }
        }
    }
}

unsigned bfm_steam_knight_flames_remaining(const BfmSteamKnightFlames *flames)
{
    return flames ? flames->remaining : 0;
}

static int64_t floor_div(int64_t n, int d)
{
    return n / d - (n % d < 0);
}

static int project_sprite(const BfmSteamFlameSprite *sprite, const int32_t world[3],
                          const BfmForestCamera *camera, unsigned growth, BfmForestDustCard *card)
{
    int64_t view[3];
    for (unsigned axis = 0; axis < 3; ++axis) {
        int64_t dot = 0;
        for (unsigned k = 0; k < 3; ++k) dot += (int64_t)camera->rotation[axis][k] * world[k];
        view[axis] = camera->translation[axis] + floor_div(dot, 4096);
        if (view[axis] < INT16_MIN || view[axis] > INT16_MAX) return 0;
    }
    if (view[2] <= 0 || view[2] / 4 + 1 >= 4096) return 0;
    int z = (int)view[2];
    int64_t reciprocal = ((int64_t)camera->projection * 65536 + z / 2) / z;
    if (reciprocal > 131071) reciprocal = 131071;
    /* The upright plume grows at its fixed world anchor. Its settled quad
     * measures about 30 by 62 pixels in the overhead view. */
    int64_t scale = reciprocal + 3 * reciprocal * growth / (2 * GROWTH_TICKS);
    int x = 160 + (int)floor_div(view[0] * reciprocal, 65536) +
                  (int)floor_div(sprite->left * scale, 65536);
    int y = 120 + (int)floor_div(view[1] * reciprocal, 65536) +
                  (int)floor_div(sprite->top * scale, 65536);
    int width = (int)((sprite->width - 1) * scale / 65536);
    int height = (int)((sprite->height - 1) * scale / 65536);
    if (!width || !height) return 0;
    card->bucket = (unsigned)(z / 4 + 1);
    card->material = (BfmForestRasterMaterial){
        .page = sprite->page, .clut = sprite->clut, .count = 4, .textured = 1, .translucent = 1
    };
    for (unsigned i = 0; i < 4; ++i) {
        int right = i & 1, bottom = i >> 1;
        card->vertices[i] = (BfmForestRasterVertex){
            x + right * width, y + bottom * height,
            (uint8_t)(sprite->u + right * (sprite->width - 1)),
            (uint8_t)(sprite->v + bottom * (sprite->height - 1)), 0x808080
        };
    }
    return 1;
}

size_t bfm_steam_knight_flames_project(const BfmSteamKnightFlames *flames,
                                      const BfmForestCamera *camera,
                                      BfmForestDustCard out[BFM_STEAM_FLAMES])
{
    if (!flames || !camera || !camera->projection || !out) return 0;
    size_t count = 0;
    for (unsigned i = 0; i < BFM_STEAM_FLAMES; ++i) {
        if (!flames->children[i].lit) continue;
        unsigned fade = flames->children[i].fade;
        const BfmSteamFlameSprite *sprite = fade ? &flames->falling[fade - 1]
                                                 : &flames->burning[(flames->frame + i) % 16];
        if (project_sprite(sprite, flames->children[i].at, camera, flames->growth, &out[count])) {
            out[count].slot = i;
            ++count;
        }
    }
    return count;
}
