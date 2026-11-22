#include "forest_transition.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

FloorPoint D_801152B0;

typedef struct {
    u32 before;
    ForestActionPlayer actor;
    u32 after;
} Fixture;

static Fixture actual, expected;
static const char *calls;
static u16 selected_kind;
static s32 expected_x, expected_z;

_Static_assert(sizeof(FloorPoint) == 8 && offsetof(FloorPoint, z) == 4, "normal layout");
_Static_assert(offsetof(ForestActionPlayer, step) == 0x234, "first motion word");
_Static_assert(offsetof(ForestActionPlayer, field_23c) == 0x23c, "last motion word");
_Static_assert(offsetof(Fixture, after) == offsetof(Fixture, actor) + sizeof(ForestActionPlayer), "trailing canary");

static void observe(ForestActionPlayer *actor, char call)
{
    assert(actor == &actual.actor && *calls == call);
    ++calls;
}

void func_80147078(ForestActionPlayer *actor, u16 kind)
{
    observe(actor, 'K');
    assert(kind == selected_kind);
    actor->kind = kind;
    actor->action = 0;
}

s32 func_8015BE94(ForestActionPlayer *actor)
{
    observe(actor, 'B');
    assert(actor->kind == 3 && actor->action == 0);
    return -0x1234567;
}

s32 func_80154150(ForestActionPlayer *actor, s32 animation)
{
    observe(actor, 'A');
    assert(animation == 17 && actor->kind == selected_kind);
    assert(actor->action == (selected_kind == 4 ? 0 : 0xffff));
    assert(actor->step == (u32)expected_x && actor->counter == 0x80000u);
    assert(actor->field_23c == (u32)expected_z);
    return 37;
}

void func_801553A8(ForestActionPlayer *actor)
{
    observe(actor, 'F');
    assert(actor->flags & 0x20000000u);
    actor->flags &= ~0x20000000u;
}

s32 func_80146CA0(ForestActionPlayer *actor)
{
    s32 incremented;
    observe(actor, 'I');
    assert(!(actor->flags & 0x20000000u));
    incremented = (s32)actor->action + 1;
    actor->action = (u16)incremented;
    return incremented;
}

static void reset(void)
{
    memset(&actual, 0x5b, sizeof(actual));
    actual.actor.kind = 9;
    actual.actor.action = 0xffff;
    actual.actor.flags = 0xb456789au;
    memcpy(&expected, &actual, sizeof(expected));
}

static void check_motion(int through_kind, FloorPoint normal, s32 x, s32 z)
{
    reset();
    D_801152B0 = normal;
    expected_x = x;
    expected_z = z;
    selected_kind = through_kind ? 4 : actual.actor.kind;
    calls = through_kind ? "KAFI" : "AFI";
    expected.actor.kind = selected_kind;
    expected.actor.action = through_kind ? 1 : 0;
    expected.actor.step = (u32)x;
    expected.actor.counter = 0x80000u;
    expected.actor.field_23c = (u32)z;
    expected.actor.flags &= ~0x20000000u;

    s32 result = through_kind ? func_8015BF48(&actual.actor) : func_8015C0C4(&actual.actor);
    assert(result == (through_kind ? 1 : 65536));
    assert(*calls == '\0' && memcmp(&actual, &expected, sizeof(actual)) == 0);
    assert(memcmp(&normal, &D_801152B0, sizeof(normal)) == 0);
}

int main(void)
{
    reset();
    selected_kind = 3;
    calls = "KB";
    expected.actor.kind = 3;
    expected.actor.action = 0;
    assert(func_8015BDD0(&actual.actor) == -0x1234567);
    assert(*calls == '\0' && memcmp(&actual, &expected, sizeof(actual)) == 0);

    for (int through_kind = 0; through_kind <= 1; ++through_kind) {
        check_motion(through_kind, (FloorPoint){(u16)-3, (u16)-11, (u16)-5, -19}, -192, -320);
        check_motion(through_kind, (FloorPoint){0x8000, 123, 0x7fff, -4}, -2097152, 2097088);
        check_motion(through_kind, (FloorPoint){0x7fff, 321, 0x8000, 4}, 2097088, -2097152);
    }
    return 0;
}
