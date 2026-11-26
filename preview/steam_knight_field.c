/* SPDX-License-Identifier: MIT */
#include "steam_knight_field.h"
#include "scene_payload.h"
#include "forest_pose.h"
#include "steam_knight_roof_cells.h"
#include "tower_sky.h"
#include "../pc_port/forest_player_bytes.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

enum { MEMBER = 5, SCENE_LIMIT = 1 << 20 };
static const uint32_t scene_base = UINT32_C(0x80128158);
static const uint32_t descriptor_slot = UINT32_C(0x80190894);

struct BfmSteamKnightField {
    uint8_t *scene;
    size_t bytes;
    BfmForestTerrain *terrain;
    BfmTowerSky *sky;
    uint8_t *damaged_mesh;
    size_t damaged_size;
    unsigned open_gates;
    unsigned broken_houses;
    int crater_ready;
    int arena_ready;
    unsigned arena_round, arena_stage;
    int16_t stomp_origin_x, stomp_origin_z;
    int16_t town_body[3][3];
    double arena_radius;
    double town_radius_targets[3];
    unsigned town_radius_slot;
    unsigned west_turn, west_delay;
    int west_requested;
    BfmForestCamera arena_view;
    int32_t arena_focus[3];
    MusashiForestPlayerMesh *hero;
    RoomTransferDescriptor entrance;
};

static const uint8_t *scene_range(const BfmSteamKnightField *field, uint32_t address, size_t size)
{
    if (address < scene_base || (address & 3) || size > field->bytes ||
        address - scene_base > field->bytes - size) return NULL;
    return field->scene + (address - scene_base);
}

/* The audience hall's mesh follows its walk grid. The final occupied sector
 * ends with its face run, independently of the position of the grid. */
static size_t mesh_size(const uint8_t *mesh, size_t available)
{
    enum { CELLS = 64 * 64, HEADER = 8, DIRECTORY = CELLS * 4 };
    if (available < HEADER + DIRECTORY || u32le(mesh) != 0x42 || mesh[4]) return 0;
    const uint8_t *table = mesh + HEADER;
    size_t limit = available - HEADER;
    for (size_t i = CELLS; i > 0; --i) {
        size_t offset = u32le(table + 4 * (i - 1));
        if (!offset) continue;
        if (offset < DIRECTORY || (offset & 3) || offset > limit - 28) return 0;
        const uint8_t *cell = table + offset;
        size_t faces = u32le(cell + 20), count = u32le(cell + 24);
        if (faces < 28 || faces > limit - offset || count > (limit - offset - faces) / 12)
            return 0;
        return HEADER + offset + faces + count * 12;
    }
    return 0;
}

BfmSteamKnightField *bfm_steam_knight_field_open(const MusashiDiscMedia *disc)
{
    if (!disc) return NULL;
    BfmSteamKnightField *field = calloc(1, sizeof *field);
    if (!field) return NULL;
    field->scene = bfm_scene_unpack_pac0(disc, "SC02.CD", MEMBER, SCENE_LIMIT, SCENE_LIMIT,
                                         &field->bytes);
    if (!field->scene) goto failed;
    const uint8_t *slot = scene_range(field, descriptor_slot, 4);
    const uint8_t *room = slot ? scene_range(field, u32le(slot), 0xcc) : NULL;
    if (!room) goto failed;
    const uint8_t *entries = scene_range(field, u32le(room + 0xc8), 8);
    const uint8_t *first = entries ? scene_range(field, u32le(entries), sizeof field->entrance) : NULL;
    const uint8_t *out = entries ? scene_range(field, u32le(entries + 4), 4) : NULL;
    if (!first || !out || u32le(first) != 0x300e || u32le(out) != 0x3001) goto failed;
    memcpy(&field->entrance, first, sizeof field->entrance);
    const uint8_t *mesh = scene_range(field, u32le(room + 0x94), 8);
    if (!mesh) goto failed;
    size_t length = mesh_size(mesh, field->bytes - (size_t)(mesh - field->scene));
    if (!length) goto failed;
    field->terrain = bfm_forest_terrain_load_archive(disc, "SC02.CD", MEMBER, mesh, length);
    if (!field->terrain || !bfm_forest_terrain_set_lighting(field->terrain, room, 0x98))
        goto failed;
    field->hero = musashi_forest_player_mesh_load(disc);
    if (field->hero) return field;
failed:
    bfm_steam_knight_field_free(field);
    return NULL;
}

int bfm_steam_knight_field_open_roof(BfmSteamKnightField *field)
{
    if (!field) return 0;
    if (field->sky) return 1;
    const uint8_t *slot = scene_range(field, descriptor_slot, 4);
    const uint8_t *room = slot ? scene_range(field, u32le(slot), 0x98) : NULL;
    const uint8_t *mesh = room ? scene_range(field, u32le(room + 0x94), 8) : NULL;
    /* The first sparse group contains four replacement sectors followed by
     * their local material packets. The next resource starts at its end. */
    enum { ROOF_SIZE = 5160 };
    const uint8_t *replacement = scene_range(field, UINT32_C(0x801d91d4), ROOF_SIZE);
    if (!mesh || !replacement) return 0;
    size_t length = mesh_size(mesh, field->bytes - (size_t)(mesh - field->scene));
    size_t new_length = 0;
    uint8_t *changed = bfm_steam_knight_roof_mesh(mesh, length, replacement, ROOF_SIZE,
                                                  &new_length);
    if (!changed) return 0;
    BfmTowerSky *sky = bfm_steam_knight_sky_open(field->scene, field->bytes,
                                                 bfm_forest_terrain_tiles(field->terrain));
    int ready = sky && bfm_forest_terrain_replace_mesh(field->terrain, changed, new_length);
    if (!ready) {
        free(changed);
        bfm_tower_sky_free(sky);
        return 0;
    }
    field->sky = sky;
    field->damaged_mesh = changed;
    field->damaged_size = new_length;
    return 1;
}

static int install_damaged_mesh(BfmSteamKnightField *field, uint8_t *pending, size_t bytes)
{
    if (!bfm_forest_terrain_replace_mesh(field->terrain, pending, bytes)) {
        free(pending);
        return 0;
    }
    free(field->damaged_mesh);
    field->damaged_mesh = pending;
    field->damaged_size = bytes;
    return 1;
}

/* Each throw breaks a wall in the existing field: the hall door, the
 * approach to the town square, and the western wall beyond the houses. */
int bfm_steam_knight_field_break_gate(BfmSteamKnightField *field, unsigned gate)
{
    static const struct { uint32_t address, end; } gates[] = {
        {UINT32_C(0x801da5fc), UINT32_C(0x801db658)},
        {UINT32_C(0x801db658), UINT32_C(0x801dc180)},
        {UINT32_C(0x801de674), UINT32_C(0x801df3b4)}
    };
    if (!field || !field->damaged_mesh || !gate || gate > sizeof gates / sizeof gates[0])
        return 0;
    if (gate <= field->open_gates) return 1;

    const uint8_t *base = field->damaged_mesh;
    size_t bytes = field->damaged_size;
    uint8_t *pending = NULL;
    for (unsigned i = field->open_gates; i < gate; ++i) {
        size_t size = gates[i].end - gates[i].address, new_size = 0;
        const uint8_t *record = scene_range(field, gates[i].address, size);
        uint8_t *next = bfm_steam_knight_gate_mesh(base, bytes, record, size, &new_size);
        free(pending);
        if (!next) return 0;
        base = pending = next;
        bytes = new_size;
    }
    if (!install_damaged_mesh(field, pending, bytes)) return 0;
    field->open_gates = gate;
    return 1;
}

int bfm_steam_knight_field_break_house(BfmSteamKnightField *field, unsigned house)
{
    static const struct { uint32_t start, end; } houses[] = {
        {UINT32_C(0x801dc180), UINT32_C(0x801dcd04)},
        {UINT32_C(0x801dcd04), UINT32_C(0x801dd2d4)},
        {UINT32_C(0x801dd2d4), UINT32_C(0x801ddd30)},
        {UINT32_C(0x801ddd30), UINT32_C(0x801de674)}
    };
    if (!field || !field->damaged_mesh || house == 0 || house > sizeof houses / sizeof houses[0]) return 0;
    unsigned bit = 1u << (house - 1);
    if (field->broken_houses & bit) return 1;
    uint32_t start = houses[house - 1].start;
    size_t size = houses[house - 1].end - start;
    const uint8_t *record = scene_range(field, start, size);
    size_t bytes = 0;
    uint8_t *pending = bfm_steam_knight_house_mesh(field->damaged_mesh, field->damaged_size,
                                                  record, size, house, &bytes);
    if (!pending || !install_damaged_mesh(field, pending, bytes)) return 0;
    /* The final west-street record replaces the same four sectors and
     * already contains the rubble from the preceding landing. */
    if (house == 4) field->broken_houses |= 1u << 2;
    field->broken_houses |= bit;
    return 1;
}

int bfm_steam_knight_field_open_crater(BfmSteamKnightField *field)
{
    if (!field || !field->damaged_mesh) return 0;
    if (field->crater_ready) return 1;
    const size_t size = 2512;
    const uint8_t *record = scene_range(field, UINT32_C(0x801df3b4), size);
    size_t bytes = 0;
    uint8_t *pending = bfm_steam_knight_crater_mesh(field->damaged_mesh, field->damaged_size,
                                                     record, size, &bytes);
    if (!pending || !install_damaged_mesh(field, pending, bytes)) return 0;
    field->crater_ready = 1;
    return 1;
}

void bfm_steam_knight_field_free(BfmSteamKnightField *field)
{
    if (!field) return;
    musashi_forest_player_mesh_free(field->hero);
    free(field->damaged_mesh);
    bfm_tower_sky_free(field->sky);
    bfm_forest_terrain_free(field->terrain);
    free(field->scene);
    free(field);
}

const uint8_t *bfm_steam_knight_field_scene(const BfmSteamKnightField *field, size_t *size)
{
    if (size) *size = field ? field->bytes : 0;
    return field ? field->scene : NULL;
}

BfmForestTerrain *bfm_steam_knight_field_terrain(const BfmSteamKnightField *field)
{
    return field ? field->terrain : NULL;
}

const RoomTransferDescriptor *bfm_steam_knight_field_entrance(const BfmSteamKnightField *field)
{
    return field ? &field->entrance : NULL;
}

static void camera_at(BfmForestCamera *camera, const int16_t eye[3])
{
    for (unsigned i = 0; i < 3; ++i) {
        int32_t dot = 0;
        for (unsigned j = 0; j < 3; ++j) dot -= camera->rotation[i][j] * eye[j];
        camera->translation[i] = dot / 4096 - (dot % 4096 < 0);
    }
}

void bfm_steam_knight_aftermath_camera(BfmForestCamera *camera, int show_ledge)
{
    if (!camera) return;
    /* Photo-fitted views across the mounted pit. The second shot pulls back
     * and includes Musashi on the lip as he speaks. */
    int sine = show_ledge ? 3855 : 3852;
    int cosine = show_ledge ? 1383 : 1394;
    *camera = (BfmForestCamera){
        .rotation = {{0, 0, 4096}, {sine, cosine, 0}, {-cosine, sine, 0}},
        .projection = 279
    };
    const int16_t pit_eye[3] = {-2214, -1723, 2909};
    const int16_t ledge_eye[3] = {-1363, -2611, 2944};
    camera_at(camera, show_ledge ? ledge_eye : pit_eye);
}

void bfm_steam_knight_entrance_camera(BfmForestCamera *camera)
{
    if (!camera) return;
    /* Carpet, entry door and steps as Musashi calls to the princess. */
    *camera = (BfmForestCamera) {
        .rotation = {{-3849, -1, -1400}, {-248, 4032, 677}, {1378, 721, -3789}},
        .projection = 507
    };
    camera_at(camera, (const int16_t[3]){-334, -2348, -3826});
}

void bfm_steam_knight_hostage_camera(BfmForestCamera *camera)
{
    if (!camera) return;
    /* Reverse shot of Fillet and Rootrick at the foot of the steps. */
    *camera = (BfmForestCamera) {
        .rotation = {{3844, 22, -1415}, {-511, 3841, -1327}, {1319, 1422, 3607}},
        .projection = 501
    };
    camera_at(camera, (const int16_t[3]){-291, -2422, -4599});
}

void bfm_steam_knight_charge_camera(BfmForestCamera *camera)
{
    if (!camera) return;
    /* Looking down at the carpet and the flames around Musashi. */
    *camera = (BfmForestCamera) {
        .rotation = {{4096, -18, 1}, {11, 2095, -3520}, {15, 3520, 2095}},
        .projection = 510
    };
    camera_at(camera, (const int16_t[3]){-126, -3221, -4470});
}

void bfm_steam_knight_captor_close_camera(BfmForestCamera *camera)
{
    if (!camera) return;
    /* Rootrick and Fillet against the east hall wall. */
    *camera = (BfmForestCamera) {
        .rotation = {{2873, -1, -2919}, {488, 4039, 479}, {2878, -684, 2833}},
        .projection = 489
    };
    camera_at(camera, (const int16_t[3]){-415, -2125, -4073});
}

void bfm_steam_knight_fire_reverse_camera(BfmForestCamera *camera)
{
    if (!camera) return;
    /* Over Rootrick as the flames close around Musashi. */
    *camera = (BfmForestCamera) {
        .rotation = {{-4096, -14, 12}, {-6, 3538, 2064}, {-18, 2064, -3538}},
        .projection = 495
    };
    camera_at(camera, (const int16_t[3]){-126, -2464, -3368});
}

void bfm_steam_knight_princess_close_camera(BfmForestCamera *camera)
{
    if (!camera) return;
    /* Low view of Fillet speaking beside Rootrick. */
    *camera = (BfmForestCamera) {
        .rotation = {{2882, 10, -2911}, {961, 3863, 965}, {2748, -1362, 2715}},
        .projection = 494
    };
    camera_at(camera, (const int16_t[3]){-446, -2028, -3986});
}

void bfm_steam_knight_postfire_camera(BfmForestCamera *camera)
{
    if (!camera) return;
    /* Musashi answers across the carpet beside the steps. */
    *camera = (BfmForestCamera) {
        .rotation = {{-2898, 14, -2895}, {-8, 4096, 28}, {2895, 25, -2898}},
        .projection = 455
    };
    camera_at(camera, (const int16_t[3]){-424, -2151, -3531});
}

void bfm_steam_knight_hall_camera(BfmForestCamera *camera)
{
    if (!camera) return;
    /* A view along the carpet toward the far door. */
    *camera = (BfmForestCamera) {
        .rotation = {{4096, 5, -6}, {-6, 4029, -736}, {5, 736, 4029}},
        .projection = 549
    };
    camera_at(camera, (const int16_t[3]){-124, -2394, -4751});
}

void bfm_steam_knight_roof_camera(BfmForestCamera *camera)
{
    if (!camera) return;
    /* Musashi looks up at the approaching machine. */
    *camera = (BfmForestCamera) {
        .rotation = {{4096, 4, -7}, {0, 3565, 2017}, {8, -2017, 3565}},
        .projection = 553
    };
    camera_at(camera, (const int16_t[3]){-124, -1960, -4198});
}

void bfm_steam_knight_machine_camera(BfmForestCamera *camera)
{
    if (!camera) return;
    /* The machine stands between Musashi and the far door. */
    *camera = (BfmForestCamera) {
        .rotation = {{4096, 1, 12}, {-3, 4035, 706}, {-12, -706, 4035}},
        .projection = 557
    };
    camera_at(camera, (const int16_t[3]){-113, -2062, -4271});
}

/* The title circles the grounded machine once the landing smoke clears.
 * The display updates its angle every two fields. */
void bfm_steam_knight_title_camera(BfmForestCamera *camera, unsigned fields_since_landing)
{
    if (!camera) return;
    unsigned yaw = 919u - 5u * (fields_since_landing / 2u);
    BfmForestTransform pose;
    bfm_forest_actor_pitch_yaw(113, yaw, (const int16_t[3]){0, 0, 0}, &pose);
    for (unsigned i = 0; i < 3; ++i)
        for (unsigned j = 0; j < 3; ++j)
            camera->rotation[i][j] = pose.rotation[j][i];
    const int radius = 530;
    int16_t eye[3] = {
        (int16_t)(-128 + radius * camera->rotation[0][2] / 4096),
        -2114,
        (int16_t)(-2944 - radius * camera->rotation[0][0] / 4096)
    };
    camera->projection = 300;
    camera_at(camera, eye);
}

int bfm_steam_knight_field_draw(BfmSteamKnightField *field, const BfmForestCamera *camera,
    const BfmForestPose *pose, const BfmForestTransform *root, uint32_t visible_parts,
    const BfmForestBlade *blade, const BfmForestBladeFrame *blade_frame, unsigned buffer,
    const BfmForestActor *actors, size_t actor_count,
    const BfmForestDustCard *cards, size_t card_count, uint32_t pixels[240][320])
{
    if (!field || !camera || !pixels || (!!pose != !!root)) return 0;
    if (field->sky) {
        if (!bfm_tower_sky_draw(field->sky, camera, pixels)) return 0;
    } else {
        memset(pixels, 0, 320 * 240 * sizeof(uint32_t));
    }
    return bfm_forest_terrain_draw_entrance_with_effects(field->terrain, camera,
        pose ? field->hero : NULL, pose, root, visible_parts, blade, buffer, blade_frame,
        actors, actor_count, cards, card_count, pixels);
}

void bfm_steam_knight_duel_camera(BfmForestCamera *camera)
{
    if (!camera) return;
    *camera = (BfmForestCamera) {
        .rotation = {{2896, 0, 2896}, {2250, 2579, -2250}, {-1824, 3182, 1824}},
        .projection = 292
    };
    camera_at(camera, (const int16_t[3]){140, -2728, -3464});
}

void bfm_steam_knight_field_turn_west(BfmSteamKnightField *field)
{
    if (field && field->arena_ready && field->arena_round == 2 && !field->west_requested) {
        field->west_requested = 1;
        field->west_delay = 2;
    }
}

int bfm_steam_knight_field_faces_west(const BfmSteamKnightField *field)
{
    return field && field->west_requested && field->west_turn == 1024;
}

void bfm_steam_knight_field_track_arena(BfmSteamKnightField *field, unsigned round,
                                       unsigned stage, const int16_t hero[3],
                                       const int16_t body[3])
{
    if (!field || !hero || !body || round > 2) return;
    int town = round == 2;
    int stomp = town && stage == 6;
    int entering = !field->arena_ready || field->arena_round != round;
    if (entering) {
        bfm_forest_terrain_set_forward_culling(field->terrain, town);
        field->west_requested = 0;
        field->west_turn = 0;
    }
    if (stomp && (entering || field->arena_stage != stage ||
                  (field->west_requested && field->west_delay == 2)))
        field->stomp_origin_x = body[0];
    if (field->west_requested && field->west_turn < 1024) {
        if (field->west_delay) {
            --field->west_delay;
        } else {
            unsigned step = (1024 - field->west_turn) / 4;
            field->west_turn += step ? step : 1;
        }
    }
    if (stomp && (entering || field->arena_stage != stage))
        field->stomp_origin_z = body[2];

    /* The first two venues frame the machine from the diagonal. Closing
     * the distance to its exposed core also brings the camera closer. */
    double target;
    if (!town) {
        double dx = hero[0] - body[0], dz = hero[2] - body[2];
        target = fmin(900.0, 644.0 + hypot(dx, dz));
    } else if (stomp) {
        /* Pull back during each leap and keep the approach to the
         * landing in view as the machine shifts along the street. */
        target = 644.0 + fmax(0.0, -2050.0 - body[1]);
    } else {
        target = 1156.0;
    }
    int target_x = body[0], target_y = -2050;
    int target_z = stomp && !field->west_turn ? field->stomp_origin_z : body[2];
    if (town) {
        if (entering) {
            for (unsigned i = 0; i < 3; ++i) {
                field->town_radius_targets[i] = target;
                memcpy(field->town_body[i], body, sizeof field->town_body[i]);
            }
            field->town_radius_slot = 0;
        }
        /* The view reacts three updates after the machine changes height. */
        unsigned slot = field->town_radius_slot;
        /* During a leap keep the street-axis landing mark framed. The
         * northbound view receives the machine's cross-street move late. */
        if (stomp)
            target_x = field->west_requested ? field->stomp_origin_x : field->town_body[slot][0];
        if (field->west_requested && stage == 10) {
            /* The rolling body carries the camera target into its bounce. */
            target_x = field->town_body[slot][0];
            target_y = field->town_body[slot][1];
            target_z = field->town_body[slot][2];
        }
        memcpy(field->town_body[slot], body, sizeof field->town_body[slot]);
        double delayed = field->town_radius_targets[slot];
        field->town_radius_targets[slot] = target;
        field->town_radius_slot = (slot + 1) % 3;
        target = delayed;
    }
    field->arena_radius = entering ? target :
        field->arena_radius + (target - field->arena_radius) * 0.25;

    BfmForestCamera *view = &field->arena_view;
    BfmForestTransform axes;
    bfm_forest_actor_pitch_yaw((unsigned)-569, town ? 0u - field->west_turn : (unsigned)-512,
                               (const int16_t[3]){0, 0, 0}, &axes);
    for (unsigned i = 0; i < 3; ++i)
        for (unsigned j = 0; j < 3; ++j)
            view->rotation[i][j] = axes.rotation[j][i];
    /* Keep the small lateral and trailing offsets in camera-heading space
     * as the street turns west. The first two venues keep their diagonal framing. */
    int bias_x = -121, bias_z = -132;
    if (town) {
        bias_x = (int)lround((2.0 * view->rotation[0][0] + 5.0 * view->rotation[0][2]) / 4096.0);
        bias_z = (int)lround((2.0 * view->rotation[0][2] - 5.0 * view->rotation[0][0]) / 4096.0);
    }
    const int32_t focus[3] = {target_x + bias_x, target_y, target_z + bias_z};
    memcpy(field->arena_focus, focus, sizeof focus);
    int16_t eye[3];
    for (unsigned i = 0; i < 3; ++i)
        eye[i] = (int16_t)lround(focus[i] - field->arena_radius * view->rotation[2][i] / 4096.0);
    view->projection = 300;
    camera_at(view, eye);
    field->arena_round = round;
    field->arena_stage = stage;
    field->arena_ready = 1;
}

void bfm_steam_knight_field_arena_camera(const BfmSteamKnightField *field,
                                        BfmForestCamera *camera)
{
    if (!camera) return;
    if (field && field->arena_ready)
        *camera = field->arena_view;
    else
        bfm_steam_knight_duel_camera(camera);
}

static void restore_duel_points(BfmForestStatus *status)
{
    /* The arena restores health and BP while retaining the earned wallet,
     * play clock, equipment and story state. */
    memcpy(status->record + 0x3c, status->record + 0x3a, 2);
    memcpy(status->record + 0x40, status->record + 0x3e, 2);
}

BfmRetryScreen *bfm_steam_knight_field_defeat(BfmSteamKnightField *field,
    const MusashiDiscMedia *disc, BfmGameplayRun *run, const int16_t feet[3],
    unsigned yaw, uint32_t visible_parts)
{
    if (!field || !field->arena_ready) return NULL;
    const uint8_t *data = scene_range(field, UINT32_C(0x80192c50), BFM_RETRY_ROOM_DATA_BYTES);
    BfmRetryMountedRoom room = {.terrain = field->terrain, .hero = field->hero,
        .data = data, .bytes = BFM_RETRY_ROOM_DATA_BYTES, .restore_status = restore_duel_points};
    return bfm_retry_screen_open_mounted(disc, &room, run, &field->arena_view,
                                         field->arena_focus, feet, yaw, visible_parts);
}
