#include "mine_emitter_setup.h"
#include <assert.h>
#include <stddef.h>
#include <string.h>

typedef struct {
    MineEmitterSetup actor;
    u8 tail[0x10c - sizeof(MineEmitterSetup)];
} SetupFixture;

_Static_assert(offsetof(MineEmitterSetup, timer) == 0x1c, "timer offset");
_Static_assert(offsetof(MineEmitterSetup, model) == 0x20, "model offset");
_Static_assert(offsetof(MineEmitterSetup, initial_delay) == 0xfc, "signed initial delay offset");
_Static_assert(offsetof(MineEmitterSetup, index) == 0xfe, "sequence index offset");
_Static_assert(offsetof(MineEmitterSetup, variant) == 0x102, "variant offset");
_Static_assert(sizeof(SetupFixture) == 0x10c, "actor slot size");

static SetupFixture current, expected;
static u32 acquired;
static s16 selected_delay;
static unsigned int calls;

static void check_actor(void)
{
    assert(memcmp(&current, &expected, sizeof(current)) == 0);
}

u32 bfm_mine_emitter_acquire_model(void)
{
    assert(calls++ == 0);
    check_actor();
    current.actor.initial_delay = expected.actor.initial_delay = 995;
    current.actor.model = expected.actor.model = 0xabcd0001u;
    current.tail[0] = expected.tail[0] = 0x74;
    return acquired;
}

void bfm_mine_emitter_bind_model(u32 model)
{
    assert(calls++ == 1 && model == acquired && acquired != 0);
    expected.actor.model = acquired;
    check_actor();
    current.actor.initial_delay = expected.actor.initial_delay = selected_delay;
    current.actor.index = expected.actor.index = -3;
    current.actor.variant = expected.actor.variant = 0x9090;
    current.actor.model = expected.actor.model = 0xfee00132u;
}

s32 func_8012CAE4(u8 *actor)
{
    assert(calls++ == 1 && acquired == 0 && actor == (u8 *)&current.actor);
    expected.actor.model = 0;
    check_actor();
    current.actor.timer = expected.actor.timer = -193;
    current.actor.index = expected.actor.index = 7;
    return 37;
}

void func_8012AD50(MineEmitterStateActor *actor)
{
    assert(calls++ == 2 && acquired != 0 && (void *)actor == (void *)&current.actor);
    expected.actor.timer = selected_delay;
    expected.actor.index = 0;
    expected.actor.variant = 0;
    check_actor();
    current.actor.variant = expected.actor.variant = 0xeeee;
    current.actor.timer = expected.actor.timer = -571;
}

static void run(u32 model, s16 delay, u8 seed)
{
    unsigned int i;
    for (i = 0; i < sizeof(current); ++i)
        ((u8 *)&current)[i] = (u8)(seed + 13 * i);
    current.actor.index = -32768;
    current.actor.timer = 0x12345678;
    current.actor.initial_delay = -1234;
    memcpy(&expected, &current, sizeof(current));
    acquired = model;
    selected_delay = delay;
    calls = 0;
    func_8017F050(&current.actor);
    assert(calls == (model ? 3u : 2u));
    check_actor();
}

int main(void)
{
    run(0, 0, 0x43);
    run(0xffffffffu, -32768, 0xdd);
    run(0x80230004u, 32767, 0x02);
    run(0x1, 0, 0x79);
    run(0x80000000u, -1, 0x11);
    return 0;
}
