#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "actor_33a_ring.h"

typedef struct {
    s16 initial_z;
    s16 expected_z;
    unsigned ring_index;
    u32 flags_from_callback;
} TestCase;

static const u32 model_handle[2] = {0x8b002030u, 0xc3112040u};
static const TestCase *current;
static Actor33A actor;
static Actor33ARingModel models[2];
static unsigned next_call;
static unsigned resolutions;

Actor33ARing D_80197B60[4];
Actor33APhase D_80186344[8] = {[6] = func_8018172C};

_Static_assert(sizeof(Actor33A) == 268, "actor slot size");
_Static_assert(sizeof(Actor33ARingModel) == 56, "secondary model slot size");
_Static_assert(sizeof(Actor33ARing) == 64, "room ring record stride");
_Static_assert(offsetof(struct Ring, pt) == 0x10, "first ring point");

static unsigned replacement_index(void)
{
    return (current->ring_index + 1) % 4;
}

static u16 timer(const Actor33A *target)
{
    u16 value;
    memcpy(&value, (const u8 *)target + 0xfc, sizeof(value));
    return value;
}

static void set_timer(Actor33A *target, u16 value)
{
    memcpy((u8 *)target + 0xfc, &value, sizeof(value));
}

Actor33ARingModel *bfm_room_33a_ring_model(u32 handle)
{
    ++resolutions;
    assert(handle == model_handle[0] || handle == model_handle[1]);
    return handle == model_handle[0] ? &models[0] : &models[1];
}

void func_8001CD50(Actor33ARingModelPrefix *model, void *shape)
{
    assert(next_call++ == 0);
    assert(model == &models[0].init);
    assert(shape == &D_80197B60[current->ring_index].shape);
    assert(actor.placed.z == current->expected_z);
    assert(timer(&actor) == 0x795a);

    /* The caller must keep its selected model and ring across all callbacks. */
    actor.placed.model_address = model_handle[1];
    actor.gate.parameter = (s16)replacement_index();
    models[0].effect.flags = 0x055aa055;
    models[0].effect.options = 0xf102;
}

void func_800233CC(struct Ring *shape, u16 radius)
{
    assert(next_call++ == 1);
    assert(shape == &D_80197B60[current->ring_index].shape);
    assert(radius == 0x80);
    assert(actor.placed.model_address == model_handle[1]);
    assert(actor.gate.parameter == (s16)replacement_index());
    shape->pt[0].x = -71;
    shape->pt[11].y = 903;
}

void func_80181844(Actor33A *target, struct Ring *shape)
{
    Actor33ARing *ring = &D_80197B60[current->ring_index];
    assert(next_call++ == 2);
    assert(target == &actor && shape == &ring->shape);
    assert(shape->pt[0].x == -71 && shape->pt[11].y == 903);
    assert(timer(target) == 0x795a);

    ring->control.intensity = 0x12345678;
    ring->control.secondary = 0x87654321;
    models[0].effect.flags = current->flags_from_callback;
    models[0].effect.options = 0x73a5;
    set_timer(target, 0x4fad);
}

void func_8012AD50(Actor334State *state)
{
    const Actor33ARing *ring = &D_80197B60[current->ring_index];
    assert(next_call++ == 3);
    assert(state == &actor.state && state->cnt == 6);
    assert(ring->control.intensity == 0xf0 && ring->control.secondary == 0);
    assert(models[0].effect.options == 0x8040);
    assert(models[0].effect.flags == (current->flags_from_callback | 0x50000000u));
    assert(timer(&actor) == 0);
    ++state->cnt;
    state->clr = 0;
}

static void run_case(const TestCase *test)
{
    Actor33A expected_actor;
    Actor33ARing expected_rings[4];
    Actor33ARingModel expected_models[2];

    current = test;
    next_call = 0;
    resolutions = 0;
    memset(&actor, 0x45, sizeof(actor));
    memset(models, 0xb6, sizeof(models));
    memset(D_80197B60, 0x93, sizeof(D_80197B60));
    actor.state.cnt = 6;
    actor.state.clr = 0xc901;
    actor.placed.model_address = model_handle[0];
    actor.placed.z = test->initial_z;
    actor.gate.parameter = (s16)test->ring_index;
    set_timer(&actor, 0x795a);

    memcpy(&expected_actor, &actor, sizeof(actor));
    expected_actor.placed.model_address = model_handle[1];
    expected_actor.placed.z = test->expected_z;
    expected_actor.gate.parameter = (s16)replacement_index();
    expected_actor.state.cnt = 7;
    expected_actor.state.clr = 0;
    set_timer(&expected_actor, 0);

    memcpy(expected_rings, D_80197B60, sizeof(expected_rings));
    expected_rings[test->ring_index].shape.pt[0].x = -71;
    expected_rings[test->ring_index].shape.pt[11].y = 903;
    expected_rings[test->ring_index].control.intensity = 0xf0;
    expected_rings[test->ring_index].control.secondary = 0;

    memcpy(expected_models, models, sizeof(expected_models));
    expected_models[0].effect.flags = test->flags_from_callback | 0x50000000u;
    expected_models[0].effect.options = 0x8040;

    func_801814B0(&actor);
    assert(next_call == 4 && resolutions == 1);
    assert(memcmp(&actor, &expected_actor, sizeof(actor)) == 0);
    assert(memcmp(models, expected_models, sizeof(models)) == 0);
    assert(memcmp(D_80197B60, expected_rings, sizeof(expected_rings)) == 0);
}

int main(void)
{
    static const TestCase cases[] = {
        {-1000, -936, 0, 0x02030405u},
        {32760, -32712, 1, 0x80003018u},
        {-32768, -32704, 2, 0x5f0040efu},
        {-64, 0, 3, 0u}
    };
    size_t i;
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        run_case(&cases[i]);
    }
    return 0;
}
