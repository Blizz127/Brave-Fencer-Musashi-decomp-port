#include "forest_impact_effect.h"
#include <assert.h>
#include <stddef.h>

_Static_assert(offsetof(ForestImpactEffect, state) == 2, "state");
_Static_assert(offsetof(ForestImpactEffect, x) == 6, "x");
_Static_assert(offsetof(ForestImpactEffect, y) == 10, "y");
_Static_assert(offsetof(ForestImpactEffect, z) == 14, "z");
_Static_assert(offsetof(ForestImpactEffect, remaining) == 28, "remaining");

ForestImpactHandler D_80189B20[2] = {func_80168AE4, func_80168B70};
static ForestImpactEffect effect;
static unsigned emissions, allocations, clears, replacements;
static int expect_variant[2], expect_count[2], mutate_owner;

void func_80168BDC(ForestImpactEffect *owner, s32 kind, s16 variant, s16 count)
{
    assert(owner == &effect && kind == 9 && emissions < 2);
    assert(variant == expect_variant[emissions] && count == expect_count[emissions]);
    emissions++;
    if (mutate_owner) {
        owner->x = -123;
        owner->y = -32768;
        owner->z = 32767;
        owner->remaining = 0x2947385au;
    }
}

s32 func_80146A6C(s32 kind, void *owner, s32 x, s32 y, s32 z, s32 argument, s32 flags)
{
    assert(owner == &effect && kind == 3 && emissions == 2);
    assert(x == -123 && y == -32768 && z == 32767);
    assert(argument == -5 && (u32)flags == 0xc0009c00u);
    effect.state = 65535;
    effect.remaining = 19;
    allocations++;
    return -18;
}

void func_80146C3C(ForestImpactEffect *owner)
{
    assert(owner == &effect && emissions == 1);
    owner->kind = 0;
    clears++;
}

static void replacement(ForestImpactEffect *owner)
{
    assert(owner == &effect);
    replacements++;
}

static void update(u32 initial, u32 expected_timer, int variant, unsigned expected_clears)
{
    emissions = allocations = clears = 0;
    effect.kind = 31;
    effect.state = 1;
    effect.remaining = initial;
    expect_variant[0] = variant;
    expect_count[0] = 1;
    func_80168AA8(&effect);
    assert(emissions == 1 && allocations == 0 && clears == expected_clears);
    assert(effect.kind == (expected_clears ? 0 : 31) && effect.remaining == expected_timer);
}

int main(void)
{
    effect.kind = 31;
    effect.state = 0;
    D_80189B20[0] = replacement;
    func_80168AA8(&effect);
    assert(replacements == 1 && emissions == 0);
    D_80189B20[0] = func_80168AE4;

    mutate_owner = 1;
    expect_variant[0] = 3;
    expect_count[0] = 2;
    expect_variant[1] = 2;
    expect_count[1] = 1;
    func_80168AA8(&effect);
    assert(emissions == 2 && allocations == 1 && clears == 0);
    assert(effect.remaining == 1 && effect.state == 0 && effect.kind == 31);

    mutate_owner = 0;
    update(1, 0, 3, 0);
    update(0, 0xffffffffu, 2, 1);
    update(0xffffffffu, 0xfffffffeu, 3, 0);
    update(0x80000000u, 0x7fffffffu, 3, 0);
    mutate_owner = 1;
    update(1, 0x2947385au, 3, 0);
    update(0, 0x2947385au, 2, 1);
    return 0;
}
