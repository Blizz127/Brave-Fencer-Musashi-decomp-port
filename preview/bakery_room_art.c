/* SPDX-License-Identifier: MIT */
#include "bakery_room_art.h"

#include "bedroom_record.h"
#include "scene_tiles.h"
#include "village_field.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>
#include <string.h>

enum { BAKERY_MEMBER = 74, SHOPKEEPER_PARTS = 7, IDLE_KEYS = 10 };
static const uint32_t scene_origin = UINT32_C(0x80128158);
static const uint32_t event_origin = UINT32_C(0x801ef468);
static const uint32_t room_address = UINT32_C(0x801f0128);

/* The entrance camera faces the counter from just inside the door. */
const BfmVillageCamera bfm_bakery_counter_camera = {
    .eye = {11, -169, -561}, .target = {0, -184, -432}, .focal = 400
};

struct BfmBakeryRoomArt {
    uint8_t scene[BFM_FIELD_SCENE_CAPACITY];
    size_t size;
    uint16_t (*tiles)[1024];
    BfmVillageField field;
    MusashiForestPlayerMesh *keeper;
    BfmForestTransform station;
    const uint8_t *poses[IDLE_KEYS];
};

static const uint8_t *mounted(const BfmBakeryRoomArt *art, uint32_t address, size_t count)
{
    if (address < scene_origin || count > art->size ||
        address - scene_origin > art->size - count) return NULL;
    return art->scene + (address - scene_origin);
}

static int mount_room(BfmBakeryRoomArt *art, const MusashiDiscMedia *disc)
{
    size_t resident = bfm_read_archive_field_scene(disc, "SC03.CD", 1, art->scene);
    if (!resident || (resident + 3u) / 4u * 4u != event_origin - scene_origin) return 0;
    size_t offset = event_origin - scene_origin, size;
    uint8_t *event = bfm_village_event_data(disc, BAKERY_MEMBER, &size);
    if (!event) return 0;
    if (size <= sizeof art->scene - offset) {
        memcpy(art->scene + offset, event, size);
        art->size = offset + size;
    }
    free(event);
    const uint8_t *room = mounted(art, room_address, 0xcc);
    const uint8_t *entries = room ? mounted(art, u32le(room + 0xc8), 4) : NULL;
    const uint8_t *first = entries ? mounted(art, u32le(entries), 36) : NULL;
    return first && u32le(first) == 0x2015;
}

static int mount_keeper(BfmBakeryRoomArt *art)
{
    const uint8_t *room = mounted(art, room_address, 0xcc);
    uint32_t first = u32le(room + 0xb4), end = u32le(room + 0xb8);
    if (end < first || end - first != 40) return 0;
    const uint8_t *placed = mounted(art, first, 40);
    if (!placed || u16le(placed + 6) != 211 || u16le(placed + 8) ||
        u16le(placed + 26)) return 0;

    /* The shopkeeper takes her position from her room script and her
     * idle clip from the shared village model bank. */
    const uint8_t *place = mounted(art, event_origin + 0xb84, 6);
    const uint8_t *animation = mounted(art, UINT32_C(0x801c3890), (IDLE_KEYS + 1u) * 8u);
    if (!place || !animation || u32le(animation + 8 * IDLE_KEYS) ||
        u32le(animation + 8 * IDLE_KEYS + 4) != 0xffff) return 0;
    for (unsigned i = 0; i < IDLE_KEYS; ++i) {
        const uint8_t *key = animation + 8*i;
        uint32_t tag = u32le(key);
        if (tag >> 24 != 0x81 || (tag & 3) || u32le(key + 4) != 1) return 0;
        art->poses[i] = mounted(art, tag & ~UINT32_C(0x01000000), SHOPKEEPER_PARTS * 8);
        if (!art->poses[i]) return 0;
    }
    int16_t xyz[3] = {s16le(place), s16le(place + 2), s16le(place + 4)};
    bfm_forest_actor_root(u16le(placed + 12), xyz, &art->station);
    unsigned parts;
    art->keeper = musashi_forest_player_mesh_from_table(art->scene, art->size, scene_origin,
                                                        event_origin + 0x6a7c, &parts);
    return art->keeper && parts == SHOPKEEPER_PARTS;
}

void bfm_bakery_room_art_free(BfmBakeryRoomArt *art)
{
    if (art) {
        musashi_forest_player_mesh_free(art->keeper);
        bfm_village_field_clear(&art->field);
        free(art);
    }
}

BfmBakeryRoomArt *bfm_bakery_room_art_open(const MusashiDiscMedia *disc,
                                          uint16_t tiles[512][1024])
{
    if (!disc || !tiles) return NULL;
    BfmBakeryRoomArt *art = calloc(1, sizeof *art);
    if (!art) return NULL;
    if (!mount_room(art, disc) || !mount_keeper(art) ||
        !bfm_village_field_load_lower(&art->field, disc, art->scene, art->size, room_address) ||
        !bfm_scene_read_tiles(disc, "MAIN.CD", 10, tiles) ||
        !bfm_scene_read_tiles(disc, "SC03.CD", 1, tiles) ||
        !bfm_scene_read_tiles(disc, "SC03.CD", BAKERY_MEMBER, tiles) ||
        !bfm_scene_read_palettes(disc, "SC03.CD", BAKERY_MEMBER, tiles) ||
        !bfm_bedroom_field_set_focal(art->field.field, bfm_bakery_counter_camera.focal)) {
        bfm_bakery_room_art_free(art);
        return NULL;
    }
    art->tiles = tiles;
    bfm_bedroom_field_viewpoint(art->field.field, bfm_bakery_counter_camera.eye, bfm_bakery_counter_camera.target, 1, 1);
    return art;
}

const uint8_t *bfm_bakery_room_art_scene(const BfmBakeryRoomArt *art, size_t *size, uint32_t *room)
{
    if (!art || !size || !room) return NULL;
    *size = art->size;
    *room = room_address;
    return art->scene;
}

const BfmBedroomView *bfm_bakery_room_art_view(const BfmBakeryRoomArt *art)
{
    return art ? bfm_bedroom_field_view(art->field.field) : NULL;
}

int bfm_bakery_room_art_draw(BfmBakeryRoomArt *art, const BfmBedroomRoot *root,
                            const BfmBedroomPose *pose, const uint8_t flags[64],
                            unsigned age, uint32_t pixels[240][320])
{
    if (!art || !root || !pose || !pose->pose || !flags || !pixels) return 0;
    for (unsigned y = 0; y < 240; ++y)
        for (unsigned x = 0; x < 320; ++x) pixels[y][x] = UINT32_C(0xff000000);
    if (!bfm_village_field_prepare(&art->field, &bfm_bakery_counter_camera, root, pose, flags, age,
                                   art->tiles, &pixels[0][0])) return 0;
    BfmForestPose idle;
    if (!bfm_forest_actor_pose_decode(art->poses[age % IDLE_KEYS], SHOPKEEPER_PARTS * 8,
                                      SHOPKEEPER_PARTS, &idle)) return 0;
    BfmForestActor jam = {.mesh = art->keeper, .pose = &idle, .root = &art->station,
                          .visible_parts = (1u << SHOPKEEPER_PARTS) - 1u};
    if (!bfm_bedroom_figure_add_actor(art->field.hero, bfm_bakery_room_art_view(art),
                                     &jam, NULL, NULL, 0)) return 0;
    bfm_bedroom_field_draw(art->field.field, art->tiles, &pixels[0][0],
                           bfm_bedroom_figure_draw_to, art->field.hero);
    return 1;
}
