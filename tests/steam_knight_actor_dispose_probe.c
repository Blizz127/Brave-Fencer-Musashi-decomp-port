#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "steam_knight_actor_dispose.h"

enum { GAME_BASE = 0x00100000, ACTOR = 272, MODEL = 1024, WORLD_SIZE = 2048 };
static _Alignas(8) unsigned char room[WORLD_SIZE];
u16 D_801274E4[1];

void *bfm_tower_spawn_pointer(u32 token)
{
    assert(token >= GAME_BASE && token < GAME_BASE + WORLD_SIZE);
    return room + (token - GAME_BASE);
}

static void test_model(unsigned model, unsigned kind, unsigned flags)
{
    unsigned char expected[WORLD_SIZE];
    u32 address = model ? GAME_BASE + model : 0;
    u16 type = (u16)kind;
    for (unsigned i = 0; i < sizeof(room); ++i)
        room[i] = (unsigned char)(3 * i + 29);
    memcpy(room + ACTOR + 32, &address, sizeof(address));
    if (model)
        memcpy(room + model, &type, sizeof(type));
    memcpy(expected, room, sizeof(expected));
    if (model && type == 1)
        memset(expected + model, 0, 132);
    if (model && type == 2)
        memset(expected + model, 0, 56);
    memset(expected + ACTOR, 0, 268);

    D_801274E4[0] = (u16)flags;
    assert(func_8012CAE4(room + ACTOR) == -1);
    assert(D_801274E4[0] == (u16)(flags | 1u));
    assert(memcmp(room, expected, sizeof(room)) == 0);
}

int main(void)
{
    const unsigned models[] = {0, MODEL, MODEL + 2, ACTOR, ACTOR + 4,
                               ACTOR + 264, ACTOR - 128};
    const unsigned kinds[] = {0, 1, 2, 3, 0xffff};
    const unsigned flags[] = {0, 1, 2, 0x8000, 0xffff};
    for (unsigned i = 0; i < sizeof(models) / sizeof(models[0]); ++i)
        for (unsigned j = 0; j < sizeof(kinds) / sizeof(kinds[0]); ++j)
            for (unsigned k = 0; k < sizeof(flags) / sizeof(flags[0]); ++k)
                test_model(models[i], kinds[j], flags[k]);
    return 0;
}
