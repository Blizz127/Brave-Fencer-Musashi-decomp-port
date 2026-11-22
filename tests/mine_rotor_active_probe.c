#include "mine_rotor_active.h"
#include "mine_actor_fields.h"

#include <assert.h>
#include <string.h>

typedef struct {
    Obj8014A2E4 actor;
    MineRotorModel models[2];
    unsigned model_calls, step_calls, sound_calls;
    unsigned refuse_model, refuse_step, refuse_sound;
    int change_context;
    uint16_t extra, sound_id, argument;
    int16_t row;
} World;

static const uint32_t first_key = UINT32_C(0xd3728f14);
static const uint32_t second_key = UINT32_C(0x4bde8a37);
static const uint32_t flags = UINT32_C(0x95249d27);
static const uint16_t angle = UINT16_C(0xf356);
static const uint16_t step = UINT16_C(0x2803);

static MineRotorModel *lookup(void *context, uint32_t key)
{
    World *world = context;
    ++world->model_calls;
    if (world->model_calls == world->refuse_model)
        return NULL;
    if (world->change_context && world->model_calls == 1)
        world->actor.probe_object_address = second_key;
    if (key == first_key)
        return &world->models[0];
    if (key == second_key)
        return &world->models[1];
    return NULL;
}

static int angular_step(void *context, int16_t row, uint16_t *result)
{
    World *world = context;
    ++world->step_calls;
    world->row = row;
    if (world->change_context) {
        mine_actor_store16(world->models[1].bytes + 20, 21);
        mine_actor_store16((uint8_t *)&world->actor + 0x70, 0xb7f3);
    }
    *result = step;
    return !world->refuse_step;
}

static int sound(void *context, uint16_t id, uint16_t argument, uint16_t extra)
{
    World *world = context;
    ++world->sound_calls;
    world->sound_id = id;
    world->argument = argument;
    world->extra = extra;
    return !world->refuse_sound;
}

static MineRotorActive setup(World *world, uint16_t x, int16_t player,
                             uint16_t option, uint16_t playing)
{
    memset(world, 0, sizeof(*world));
    memset(&world->actor, 0x5d, sizeof(world->actor));
    world->actor.probe_object_address = first_key;
    uint8_t *actor = (uint8_t *)&world->actor;
    mine_actor_store16(actor + 6, x);
    mine_actor_store16(actor + 0x70, option);
    mine_actor_store16(actor + 0x84, playing);
    for (unsigned i = 0; i < 2; ++i) {
        memset(&world->models[i], 0xea, sizeof(world->models[i]));
        mine_actor_store32(world->models[i].bytes + 4, flags);
        mine_actor_store16(world->models[i].bytes + 20, angle + i);
    }
    MineRotorActive api = {world, player, lookup, angular_step, sound};
    return api;
}

static void sweep(void)
{
    World world;
    /* Rotating the origin makes both signed and unsigned coordinate wraps occur. */
    const int16_t player = 32059;
    for (unsigned delta = 0; delta < 65536; ++delta) {
        const unsigned magnitude = delta <= 32768 ? delta : 65536 - delta;
        const int hidden = magnitude > 2048 && magnitude != 32768;
        const uint16_t option = (uint16_t)(delta * 171u);
        const unsigned volume = magnitude >= 828 ? 0 :
                                magnitude < 320 ? 127 : (831 - magnitude) / 4;
        for (unsigned starting = 0; starting < 2; ++starting) {
            uint16_t playing = starting ? 0xbd37 : 0;
            MineRotorActive api = setup(&world, (uint16_t)(delta + player),
                                        player, option, playing);
            Obj8014A2E4 expected_actor = world.actor;
            MineRotorModel expected = world.models[0];
            MineRotorModel untouched = world.models[1];
            if (!hidden) {
                mine_actor_store32(expected.bytes + 4, flags & UINT32_C(0x7fffffff));
                mine_actor_store16(expected.bytes + 20, (uint16_t)(angle + step));
            } else {
                mine_actor_store32(world.models[0].bytes + 4, flags & UINT32_C(0x7fffffff));
            }
            mine_actor_store16((uint8_t *)&expected_actor + 0x84, volume ? 1 : 0);
            assert(mine_rotor_update(&world.actor, &api));
            assert(!memcmp(&world.actor, &expected_actor, sizeof(expected_actor)));
            assert(!memcmp(&world.models[0], &expected, sizeof(expected)));
            assert(!memcmp(&world.models[1], &untouched, sizeof(untouched)));
            assert(world.model_calls == (hidden ? 1u : 2u));
            assert(world.step_calls == (unsigned)!hidden);
            if (!hidden)
                assert((uint16_t)world.row == option);
            assert(world.sound_calls == (unsigned)(volume || playing));
            if (world.sound_calls) {
                assert(world.sound_id == (volume ? 1757 : 4));
                assert(world.argument == (volume ? (0x1000 | volume) : 1757));
                assert(world.extra == option);
            }
        }
    }
}

static void boundaries(void)
{
    World world;
    MineRotorActive api = setup(&world, 100, 100, 0xffc7, 7);
    Obj8014A2E4 actor = world.actor;
    MineRotorModel model = world.models[0];
    assert(!mine_rotor_update(NULL, &api));
    assert(!mine_rotor_update(&world.actor, NULL));
    api.model = NULL;
    assert(!mine_rotor_update(&world.actor, &api));
    api.model = lookup;
    world.refuse_model = 1;
    assert(!mine_rotor_update(&world.actor, &api));
    assert(!memcmp(&world.actor, &actor, sizeof(actor)));
    assert(!memcmp(&world.models[0], &model, sizeof(model)));
    assert(!world.step_calls && !world.sound_calls);

    for (unsigned which = 0; which < 5; ++which) {
        api = setup(&world, 100, 100, 0xffc7, 7);
        if (which == 0) world.refuse_model = 2;
        if (which == 1) api.angular_step = NULL;
        if (which == 2) world.refuse_step = 1;
        if (which == 3) api.sound = NULL;
        if (which == 4) world.refuse_sound = 1;
        assert(!mine_rotor_update(&world.actor, &api));
        assert(!memcmp(&world.actor, &actor, sizeof(actor)));
        MineRotorModel expected = model;
        mine_actor_store32(expected.bytes + 4, flags & UINT32_C(0x7fffffff));
        if (which >= 3)
            mine_actor_store16(expected.bytes + 20, (uint16_t)(angle + step));
        assert(!memcmp(&expected, &world.models[0], sizeof(expected)));
        assert(world.model_calls == 2 && world.step_calls == (which >= 2 ? 1u : 0u));
        assert(world.sound_calls == (which == 4 ? 1u : 0u));
    }

    api = setup(&world, 3300, 0, 0xaace, 0xbd37);
    world.refuse_sound = 1;
    api.angular_step = NULL;
    assert(!mine_rotor_update(&world.actor, &api));
    assert(world.model_calls == 1 && !world.step_calls && world.sound_calls == 1);
    assert(world.sound_id == 4 && world.argument == 1757 && world.extra == 0xaace);
    assert(mine_actor_load16((uint8_t *)&world.actor + 0x84) == 0xbd37);
    api.sound = NULL;
    mine_actor_store16((uint8_t *)&world.actor + 0x84, 0);
    assert(mine_rotor_update(&world.actor, &api));
    assert(world.model_calls == 2 && world.sound_calls == 1);

    api = setup(&world, 100, 100, 0xffc7, 7);
    world.change_context = 1;
    assert(mine_rotor_update(&world.actor, &api));
    assert(world.row == -57 && world.extra == 0xb7f3);
    assert(world.sound_calls == 1 && world.model_calls == 2 && world.step_calls == 1);
    assert(world.actor.probe_object_address == second_key);
    assert(mine_actor_load16(world.models[0].bytes + 20) == angle);
    assert(mine_actor_load16(world.models[1].bytes + 20) == (uint16_t)(angle + 1 + step));
    assert(mine_actor_load32(world.models[0].bytes + 4) == (flags & UINT32_C(0x7fffffff)));
    assert(mine_actor_load32(world.models[1].bytes + 4) == flags);
}

int main(void)
{
    sweep();
    boundaries();
    return 0;
}
