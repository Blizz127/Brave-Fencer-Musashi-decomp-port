#include <assert.h>
#include <limits.h>
#include <stddef.h>

#include "forest_settle_effect.h"

_Static_assert(offsetof(ForestSettleEffect, state) == 2, "effect state");
_Static_assert(offsetof(ForestSettleEffect, remaining) == 0x1c, "effect countdown");

static ForestSettleEffect first, second, *active;
static int events, emitted, expired, result, expected;
static s32 next_random[] = {INT_MAX, -1, 0x345, INT_MIN};

void func_8012AD44(u8 *object, u16 state)
{
    ForestSettleEffect *effect = (ForestSettleEffect *)object;
    assert(effect == active && state == 1 && effect->remaining == 16 && events++ == 0);
    effect->state = (u16)state;
}

s32 func_8005C49C(void)
{
    assert(emitted < 4 && events++ == 2 * emitted);
    return next_random[emitted];
}

void func_8017EB2C(s32 random, s32 remaining)
{
    assert(random == next_random[emitted] && remaining == expected && events++ == 2 * emitted + 1);
    ++emitted;
    active->remaining = ++expected;
}

s32 func_8012BEE8(u8 *object)
{
    ForestSettleEffect *effect = (ForestSettleEffect *)object;
    assert(effect == active && events++ == 8 && emitted == 4);
    return result;
}

void func_8012C218(u8 *object)
{
    ForestSettleEffect *effect = (ForestSettleEffect *)object;
    assert(effect == active && events++ == 9 && result == 1);
    ++expired;
}

typedef void (*ForestSettleStep)(ForestSettleEffect *);
ForestSettleStep D_8018A7B8[] = {func_8017EA98, func_8017EAC0};

int main(void)
{
    const int returns[] = {-1, 0, 1, 2};
    for (unsigned i = 0; i < sizeof returns / sizeof returns[0]; ++i) {
        active = i & 1 ? &first : &second;
        *active = (ForestSettleEffect){0};
        active->type = 37;
        active->remaining = -313;
        events = emitted = expired = 0;
        func_8017EC40(active);
        assert(events == 1 && active->state == 1 && active->type == 37);
        expected = active->remaining;
        result = returns[i];
        events = 0;
        func_8017EC40(active);
        assert(expired == (result == 1) && events == 9 + expired && active->remaining == 20);
    }
    return 0;
}
