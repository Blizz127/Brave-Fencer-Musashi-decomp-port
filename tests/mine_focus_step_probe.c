#include "mine_focus_step.h"
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

_Static_assert(offsetof(FacingActor, focus.x) == 0x48, "focus x");
_Static_assert(offsetof(FacingActor, focus.y) == 0x4c, "focus y");
_Static_assert(offsetof(FacingActor, focus.z) == 0x50, "focus z");
_Static_assert(offsetof(Obj801508F8, active) == 0x1bf, "caller eligibility byte");

MineFocusParameters D_8019F170;
u32 D_80180D74;
FacingActor D_80126948;
extern s32 func_801508F8(Obj801508F8 *actor);

static Obj801508F8 actor;
static s32 expected_x, expected_y, expected_z, desired_x, desired_z;
static s32 next_x, next_z, clip_result;
static unsigned visits, passes;
static int mutate;

static void set_coord(unsigned offset, s32 value)
{
    u8 *p = (u8 *)&actor + offset;
    p[0] = (u8)value;
    p[1] = (u8)((u32)value >> 8);
}
static s32 get_coord(unsigned offset)
{
    const u8 *p = (const u8 *)&actor + offset;
    return (s32)(((u32)p[0] | (u32)p[1] << 8) ^ 0x8000u) - 0x8000;
}
s32 func_80021174(u32 clip, s32 *p)
{
    MineFocusParameters params;
    assert(clip == D_80180D74);
    memcpy(&params, (const u8 *)p + 0x30, sizeof(params));
    assert(!memcmp(&params, &D_8019F170, sizeof(params)));
    assert(p[0] == get_coord(6) && p[1] == D_80126948.focus.y && p[2] == get_coord(14));
    if (!visits)
        assert(p[0] == expected_x && p[1] == expected_y && p[2] == expected_z);
    visits++;
    if (mutate) {
        set_coord(6, next_x);
        set_coord(14, next_z);
        D_80126948.focus.x = desired_x;
        D_80126948.focus.z = desired_z;
    }
    return passes && visits > passes ? 1 : clip_result;
}

static s32 narrowed(s32 v)
{
    return (s32)(((u32)v & 0xffffu) ^ 0x8000u) - 0x8000;
}
static s32 step(s32 from, s32 toward)
{
    u32 offset = (u32)toward - (u32)from;
    if (offset == 0x80000000u)
        return from;
    if (offset >= 5u && offset <= 0x7fffffffu)
        return narrowed(from + 4);
    if (offset >= 0x80000001u && offset <= 0xfffffffbu)
        return narrowed(from - 4);
    return from;
}
static void start(s32 x, s32 z)
{
    memset(&actor, 0x53, sizeof(actor));
    actor.type = 1;
    actor.active = 1;
    expected_x = x;
    expected_z = z;
    expected_y = 13 * x - z;
    set_coord(6, x);
    set_coord(14, z);
    D_80126948.focus.y = expected_y;
    visits = passes = 0;
    mutate = 0;
    clip_result = 0;
}

int main(void)
{
    unsigned i;
    D_80180D74 = UINT32_C(0x94fd662a);
    for (i = 0; i < 4; i++)
        D_8019F170.field[i] = (i + 71u) * 0x81902cu;
    for (i = 0; i <= 65535; i++) {
        s32 x = narrowed((s32)i);
        s32 z = narrowed((s32)(i * 19u));
        s32 dx = (s32)(i % 13u) - 6, dz = 6 - (s32)(i % 13u);
        start(x, z);
        D_80126948.focus.x = x + dx;
        D_80126948.focus.z = z + dz;
        assert(func_8015094C(&actor) == 1 && visits == 1);
        assert(get_coord(6) == step(x, x + dx) && get_coord(14) == step(z, z + dz));
        start(x, z);
        D_80126948.focus.x = 2;
        D_80126948.focus.z = -2;
        mutate = 1;
        next_x = narrowed(x + 911);
        next_z = narrowed(z - 173);
        desired_x = (i & 1) ? INT_MIN : INT_MAX;
        desired_z = (i & 1) ? INT_MAX : INT_MIN;
        clip_result = (i & 2) ? -17 : 0;
        assert(func_8015094C(&actor) == !clip_result && visits == 1);
        assert(get_coord(6) == (clip_result ? next_x : step(next_x, desired_x)));
        assert(get_coord(14) == (clip_result ? next_z : step(next_z, desired_z)));
    }
    start(0, -1);
    D_80126948.focus.x = INT_MIN;
    D_80126948.focus.z = INT_MAX;
    assert(func_8015094C(&actor) && !get_coord(6) && get_coord(14) == -1);
    start(0, 0);
    D_80126948.focus.x = 64;
    D_80126948.focus.z = -64;
    passes = 7;
    (void)func_801508F8(&actor);
    assert(visits == 8 && get_coord(6) == 28 && get_coord(14) == -28);
    actor.active = 0;
    (void)func_801508F8(&actor);
    actor.active = 1;
    actor.type = 0x1a;
    (void)func_801508F8(&actor);
    assert(visits == 8);
    return 0;
}
