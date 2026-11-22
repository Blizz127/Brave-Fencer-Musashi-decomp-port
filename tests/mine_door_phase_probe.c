#include "mine_door_phase.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

struct Record { u16 state; u8 rest[0x82]; };
typedef struct {
    s16 unk0, unk2;
    u8 pad4[0x1c];
    s32 unk20, unk24;
} DoorListModel;

u8 D_8017F65C[1];
static MineDoorActor *subject;
static struct Record model;
static int available;
static int allocated, initialized, released, ticked;

struct Record *func_8001CFDC(int first, int limit)
{
    assert(first == 38 && limit == 133);
    ++allocated;
    return available ? &model : 0;
}

void func_8001C214(DoorListModel *entry, s32 *list)
{
    assert((void *)entry == &model && list == 0);
    assert(subject->model == (u32)(uintptr_t)&model);
    ++initialized;
    subject->phase = 0xffff;
}

s32 func_8012CAE4(u8 *actor)
{
    assert(actor == (u8 *)subject && subject->model == 0);
    ++released;
    return -753;
}

static s32 active(MineDoorActor *actor)
{
    assert(actor == subject);
    ++ticked;
    return -932;
}

MineDoorPhase D_8017F684[] = {func_8013221C, active};

int main(void)
{
    MineDoorActor actor, expected;
    _Static_assert(offsetof(MineDoorActor, model) == 0x20, "model slot");
    _Static_assert(offsetof(MineDoorActor, shape) == 0x58, "shape slot");
    _Static_assert(offsetof(MineDoorActor, contact_flags) == 0x5c, "contact flags");
    memset(&actor, 0x95, sizeof(actor));
    subject = &actor;
    actor.phase = 1;
    memcpy(&expected, &actor, sizeof(expected));
    assert(func_80131FDC(&actor) == -932);
    assert(ticked == 1 && allocated == 0 && !memcmp(&expected, &actor, sizeof(actor)));

    actor.phase = 0;
    memcpy(&expected, &actor, sizeof(expected));
    expected.model = 0;
    assert(func_80131FDC(&actor) == -753);
    assert(allocated == 1 && released == 1 && initialized == 0);
    assert(!memcmp(&expected, &actor, sizeof(actor)));

    available = 1;
    expected.model = (u32)(uintptr_t)&model;
    expected.shape = (u32)(uintptr_t)D_8017F65C;
    expected.contact_flags = 0x80;
    assert(func_80131FDC(&actor) == 65536);
    assert(allocated == 2 && released == 1 && initialized == 1);
    assert(!memcmp(&expected, &actor, sizeof(actor)));
    return 0;
}
