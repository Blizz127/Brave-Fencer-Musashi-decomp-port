/* SPDX-License-Identifier: MIT */
#include "village_mine_corridor_408.h"
#include "village_mine_basket.h"
#include "forest_lift.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>

enum { ARMS = 4, PLACEMENT_BYTES = 20, ACTIVATION_DISTANCE = 2048 };
static const uint32_t base = UINT32_C(0x80128158);

typedef struct {
    BfmForestTransform pivot, deck;
    BfmMineBasketSway sway;
    int visible;
} Basket;

typedef struct {
    int16_t center[3], speed;
    uint16_t phase;
    BfmForestTransform wheel, stand;
    Basket basket[ARMS];
    int visible, audible;
} MineWheel;

struct BfmVillageMineWheels {
    MineWheel wheels[BFM_MINE_WHEEL_COUNT];
    MusashiForestPlayerMesh *wheel_mesh, *basket_mesh, *stand_mesh;
    BfmForestPose pose;
    BfmForestGuardBox box;
    int16_t arm[ARMS][3];
};

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t length)
{
    if (!scene || address < base || length > bytes || address - base > bytes - length) return NULL;
    return scene + (address - base);
}

void bfm_village_mine_wheels_free(BfmVillageMineWheels *wheels)
{
    if (wheels) {
        musashi_forest_player_mesh_free(wheels->wheel_mesh);
        musashi_forest_player_mesh_free(wheels->basket_mesh);
        musashi_forest_player_mesh_free(wheels->stand_mesh);
        free(wheels);
    }
}

static MusashiForestPlayerMesh *model(const uint8_t *scene, size_t bytes, uint32_t table)
{
    unsigned parts = 0;
    MusashiForestPlayerMesh *mesh = musashi_forest_player_mesh_from_table(scene, bytes, base, table, &parts);
    if (parts == 1) return mesh;
    musashi_forest_player_mesh_free(mesh);
    return NULL;
}

static int load_wheel(MineWheel *wheel, const uint8_t *row, const uint8_t *arms, int16_t speed)
{
    if (wheel->speed || !speed || u16le(row + 12)) return 0;
    wheel->speed = speed;
    for (unsigned axis = 0; axis < 3; ++axis) wheel->center[axis] = s16le(row + axis * 2);
    bfm_forest_actor_root(0, wheel->center, &wheel->wheel);
    int16_t stand[3] = {wheel->center[0], wheel->center[1] + 192, wheel->center[2] - 176};
    bfm_forest_actor_root(0, stand, &wheel->stand);
    for (unsigned i = 0; i < ARMS; ++i) {
        if (u16le(arms + i * 2) != 0) return 0;
        /* Spawned children inherit the parent position. Service replaces
         * each visible pivot with its point on the rim. */
        wheel->basket[i].pivot = wheel->wheel;
        bfm_mine_basket_body_root(&wheel->wheel, &wheel->basket[i].deck);
    }
    return 1;
}

BfmVillageMineWheels *bfm_village_mine_wheels_open(const uint8_t *scene, size_t bytes)
{
    const uint8_t *binding = mounted(scene, bytes, UINT32_C(0x80180c54), 4);
    if (!binding || u32le(binding) != UINT32_C(0x80184870)) return NULL;
    const uint8_t *room = mounted(scene, bytes, u32le(binding), 0xbc);
    const uint8_t *shape = mounted(scene, bytes, UINT32_C(0x801847b0), 16);
    const uint8_t *types = mounted(scene, bytes, UINT32_C(0x8018476c), BFM_MINE_WHEEL_COUNT * 8);
    const uint8_t *speeds = mounted(scene, bytes, UINT32_C(0x8018479c), BFM_MINE_WHEEL_COUNT * 2);
    const uint8_t *offset = mounted(scene, bytes, UINT32_C(0x801a04f0), 6);
    if (!room || !shape || u32le(shape) || !types || !speeds || !offset ||
        s16le(offset) <= 0 || s16le(offset + 2) || s16le(offset + 4)) return NULL;
    uint32_t first = u32le(room + 0xb4), end = u32le(room + 0xb8);
    if (end <= first || (end - first) % PLACEMENT_BYTES) return NULL;
    const uint8_t *rows = mounted(scene, bytes, first, end - first);
    if (!rows) return NULL;

    BfmVillageMineWheels *wheels = calloc(1, sizeof *wheels);
    if (!wheels) return NULL;
    int16_t radius = s16le(offset);
    wheels->arm[0][0] = radius;
    wheels->arm[1][1] = radius;
    wheels->arm[2][0] = -radius;
    wheels->arm[3][1] = -radius;
    for (unsigned i = 0; i < 3; ++i) {
        wheels->box.low[i] = s16le(shape + 4 + 4*i);
        wheels->box.high[i] = s16le(shape + 6 + 4*i);
        if (wheels->box.low[i] >= wheels->box.high[i]) goto fail;
    }
    for (size_t off = 0; off < end - first; off += PLACEMENT_BYTES) {
        const uint8_t *row = rows + off;
        if (!u16le(row + 6)) break;
        if (u16le(row + 6) != 408) continue;
        unsigned variant = u16le(row + 8);
        if (variant >= BFM_MINE_WHEEL_COUNT ||
            !load_wheel(&wheels->wheels[variant], row, types + variant * 8,
                        s16le(speeds + variant * 2))) goto fail;
    }
    for (unsigned i = 0; i < BFM_MINE_WHEEL_COUNT; ++i)
        if (!wheels->wheels[i].speed) goto fail;
    wheels->wheel_mesh = model(scene, bytes, UINT32_C(0x8019c5fc));
    wheels->basket_mesh = model(scene, bytes, UINT32_C(0x8019c94c));
    wheels->stand_mesh = model(scene, bytes, UINT32_C(0x8019da70));
    if (!wheels->wheel_mesh || !wheels->basket_mesh || !wheels->stand_mesh) goto fail;
    const int16_t origin[3] = {0};
    bfm_forest_actor_root(0, origin, &wheels->pose.local[0]);
    return wheels;
fail:
    bfm_village_mine_wheels_free(wheels);
    return NULL;
}

static void roll_root(int16_t roll, const int16_t position[3], BfmForestTransform *root)
{
    BfmForestTransform horizontal;
    bfm_forest_actor_root((unsigned)abs(roll), position, &horizontal);
    int16_t c = horizontal.rotation[0][0], s = horizontal.rotation[0][2];
    if (roll < 0) s = -s;
    *root = (BfmForestTransform){.rotation = {{c,-s,0},{s,c,0},{0,0,4096}},
                                 .translation = {position[0],position[1],position[2]}};
}

BfmMineWheelAudio bfm_village_mine_wheels_step(BfmVillageMineWheels *wheels,
    int16_t reference_x, int rider, int32_t foot_q16[3])
{
    BfmMineWheelAudio audio = {0};
    if (!wheels) return audio;
    for (unsigned i = 0; i < BFM_MINE_WHEEL_COUNT; ++i) {
        MineWheel *wheel = &wheels->wheels[i];
        int distance = abs(wheel->center[0] - reference_x);
        wheel->visible = distance <= ACTIVATION_DISTANCE;
        if (wheel->visible) {
            wheel->phase = (uint16_t)(wheel->phase + wheel->speed);
            roll_root((int16_t)wheel->phase, wheel->center, &wheel->wheel);
        }
        int fade = distance > 320 ? (distance - 320) / 4 : 0;
        int gain = fade < 127 ? 127 - fade : 0;
        audio.gain[i] = gain ? gain : -wheel->audible;
        wheel->audible = gain > 0;
        for (unsigned j = 0; j < ARMS; ++j) {
            Basket *basket = &wheel->basket[j];
            basket->visible = abs(basket->pivot.translation[0] - reference_x) <= ACTIVATION_DISTANCE;
            if (!basket->visible) continue;
            BfmForestTransform previous = basket->deck;
            int16_t point[3];
            bfm_forest_lift_world(&wheel->wheel, wheels->arm[j], point);
            roll_root(bfm_mine_basket_step_sway(&basket->sway), point, &basket->pivot);
            bfm_mine_basket_body_root(&basket->pivot, &basket->deck);
            bfm_mine_basket_carry(&basket->sway, &wheels->box, &previous, &basket->deck,
                                  rider == (int)(i * ARMS + j), foot_q16);
        }
    }
    return audio;
}

const BfmForestTransform *bfm_village_mine_wheels_deck(const BfmVillageMineWheels *wheels,
    unsigned index, const BfmForestGuardBox **box)
{
    if (!wheels || index >= BFM_MINE_WHEEL_DECKS) return NULL;
    const Basket *basket = &wheels->wheels[index / ARMS].basket[index % ARMS];
    if (!basket->visible) return NULL;
    if (box) *box = &wheels->box;
    return &basket->deck;
}

int bfm_village_mine_wheels_draw(void *context, BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    const BfmVillageMineWheels *wheels = context;
    if (!wheels || !figure || !view) return 0;
    BfmForestActor actor = {.pose = &wheels->pose, .visible_parts = 1};
    for (unsigned i = 0; i < BFM_MINE_WHEEL_COUNT; ++i) {
        const MineWheel *wheel = &wheels->wheels[i];
        actor.mesh = wheels->stand_mesh;
        actor.root = &wheel->stand;
        if (!bfm_bedroom_figure_add_actor(figure, view, &actor, NULL, NULL, 0)) return 0;
        if (wheel->visible) {
            actor.mesh = wheels->wheel_mesh;
            actor.root = &wheel->wheel;
            if (!bfm_bedroom_figure_add_actor(figure, view, &actor, NULL, NULL, 0)) return 0;
        }
        actor.mesh = wheels->basket_mesh;
        for (unsigned j = 0; j < ARMS; ++j) {
            if (!wheel->basket[j].visible) continue;
            actor.root = &wheel->basket[j].pivot;
            if (!bfm_bedroom_figure_add_actor(figure, view, &actor, NULL, NULL, 0)) return 0;
        }
    }
    return 1;
}
