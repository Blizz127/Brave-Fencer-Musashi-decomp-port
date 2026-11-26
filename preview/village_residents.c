/* SPDX-License-Identifier: MIT */
#include "village_residents.h"
#include "bedroom_visitor.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>

enum { IDLE_KEYS = 8, BODY_PARTS = 10, PLACEMENT_BYTES = 20 };
static const uint32_t scene_origin = UINT32_C(0x80128158);

struct BfmVillageResidents {
    BfmBedroomVisitor *body;
    int position[3], heading;
    uint32_t poses[IDLE_KEYS];
};

static const uint8_t *record(const uint8_t *scene, size_t bytes, uint32_t address, size_t size)
{
    if (address < scene_origin || size > bytes || address - scene_origin > bytes - size)
        return NULL;
    return scene + (address - scene_origin);
}

static int has_hillside_resident(const uint8_t *scene, size_t bytes, const uint8_t *room)
{
    uint32_t list = u32le(room + 0xb4), end = u32le(room + 0xb8);
    if (end <= list || (end - list) % PLACEMENT_BYTES) return 0;
    const uint8_t *placements = record(scene, bytes, list, end - list);
    if (!placements) return 0;
    for (size_t offset = 0; offset < end - list; offset += PLACEMENT_BYTES) {
        unsigned kind = u16le(placements + offset + 6);
        if (kind == 160) return 1;
        if (!kind) break;
    }
    return 0;
}

BfmVillageResidents *bfm_village_residents_load(const uint8_t *scene, size_t bytes,
                                                 uint32_t room, uint32_t shadow)
{
    if (!scene || room != UINT32_C(0x8018c794)) return NULL;
    const uint8_t *descriptor = record(scene, bytes, room, 0xbc);
    if (!descriptor || !has_hillside_resident(scene, bytes, descriptor)) return NULL;

    /* The hillside resident's daytime standing station contains position and
     * bearing. Its model and idle clip belong to the following resource kit. */
    const uint8_t *station = record(scene, bytes, UINT32_C(0x80189850), 8);
    const uint8_t *kit = record(scene, bytes, UINT32_C(0x80189860), 8);
    if (!station || !kit) return NULL;
    const uint8_t *clip = record(scene, bytes, u32le(kit + 4), (IDLE_KEYS + 1) * 8);
    if (!clip || u32le(clip + IDLE_KEYS * 8) || u32le(clip + IDLE_KEYS * 8 + 4) != 0xffffu)
        return NULL;
    uint32_t poses[IDLE_KEYS];
    for (unsigned i = 0; i < IDLE_KEYS; ++i) {
        uint32_t tagged = u32le(clip + i * 8);
        poses[i] = tagged & ~UINT32_C(0x01000000);
        if (tagged >> 24 != 0x81 || u32le(clip + i * 8 + 4) != 1 ||
            !record(scene, bytes, poses[i], BODY_PARTS * 8)) return NULL;
    }

    BfmVillageResidents *residents = calloc(1, sizeof(*residents));
    if (!residents) return NULL;
    uint32_t model = u32le(kit);
    residents->body = bfm_bedroom_visitor_load(scene, bytes, &model, 1, shadow);
    if (!residents->body) {
        free(residents);
        return NULL;
    }
    for (unsigned i = 0; i < IDLE_KEYS; ++i) residents->poses[i] = poses[i];
    for (unsigned i = 0; i < 3; ++i) residents->position[i] = s16le(station + i * 2);
    residents->heading = s16le(station + 6);
    return residents;
}

void bfm_village_residents_free(BfmVillageResidents *residents)
{
    if (!residents) return;
    bfm_bedroom_visitor_free(residents->body);
    free(residents);
}

int bfm_village_residents_add(const BfmVillageResidents *residents, BfmBedroomFigure *figure,
                              const BfmBedroomView *view, unsigned age)
{
    return residents && bfm_bedroom_visitor_add(residents->body, figure, view, 0,
                           residents->position, residents->heading, residents->poses[age % IDLE_KEYS]);
}
