#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "door_15d.h"

typedef struct {
    s16 unk0, unk2;
    u8 pad4[0x1c];
    s32 unk20, unk24;
} DoorModelList;

static DoorModelList available_model;
static Door15DActor actor;
static int available;
static unsigned calls;
static u16 service_phase;
const u8 D_80182DF0[1] = {0};

_Static_assert(sizeof(Door15DActor) == 268, "actor stride");
_Static_assert(offsetof(Door15DActor, phase) == 2, "phase offset");
_Static_assert(offsetof(Door15DActor, model_address) == 32, "model offset");
_Static_assert(offsetof(Door15DActor, shape_address) == 88, "shape offset");
_Static_assert(offsetof(Door15DActor, contact_flags) == 92, "contact offset");

void *func_8012C1B8(void)
{
    assert(calls == 0);
    ++calls;
    return available ? &available_model : NULL;
}

void func_8001C214(DoorModelList *model, s32 *list)
{
    assert(calls == 1 && available);
    assert(model == &available_model && list == NULL);
    assert(actor.model_address == (u32)(uintptr_t)model);
    actor.phase = service_phase;
    ++calls;
}

s32 func_8012CAE4(u8 *record)
{
    assert(calls == 1 && !available);
    assert(record == (u8 *)&actor && actor.model_address == 0);
    actor.tail[5] = 17;
    ++calls;
    return -721;
}

static s32 idle(Door15DActor *door)
{
    assert(door == &actor && door->phase == 1);
    ++calls;
    return -718;
}

s32 (*D_80182E28[])(Door15DActor *) = {func_80132144, idle};

static void check_case(int has_model, u16 phase, int dispatch)
{
    Door15DActor expected;
    s32 result;
    memset(&actor, 0x59, sizeof(actor));
    actor.type = 0x15d;
    actor.phase = dispatch ? 0 : 3;
    available = has_model;
    service_phase = phase;
    calls = 0;
    expected = actor;
    if (has_model) {
        expected.model_address = (u32)(uintptr_t)&available_model;
        expected.phase = (u16)(phase + 1);
        expected.shape_address = (u32)(uintptr_t)D_80182DF0;
        expected.contact_flags = 0x80;
    } else {
        expected.model_address = 0;
        expected.tail[5] = 17;
    }
    result = dispatch ? func_80131F64(&actor) : func_80132144(&actor);
    assert(calls == 2 && result == (has_model ? phase + 1 : -721));
    assert(memcmp(&actor, &expected, sizeof(actor)) == 0);
}

int main(void)
{
    check_case(0, 0, 0);
    check_case(1, 0, 0);
    check_case(1, 0xffff, 0);
    check_case(0, 0, 1);
    check_case(1, 0, 1);
    {
        Door15DActor before = actor;
        assert(func_80131F64(&actor) == -718);
        assert(calls == 3 && memcmp(&actor, &before, sizeof(actor)) == 0);
    }
    return 0;
}
