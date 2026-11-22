#include "mine_award_controller.h"
#include "../main_0012/scene_object_probes.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

_Static_assert(sizeof(Obj8014A2E4) == 268, "room actor stride");
_Static_assert(offsetof(Obj8014A2E4, pad05E) == 0x5e, "actor identifier layout");

Obj8014A2E4 D_801202A0[96];

static void set_key(unsigned index, u16 kind, s16 number)
{
    Obj8014A2E4 *slot = &D_801202A0[index];
    u16 bits = (u16)number;
    size_t field = 0x70 - offsetof(Obj8014A2E4, pad05E);
    slot->active = kind;
    slot->pad05E[field] = (u8)bits;
    slot->pad05E[field + 1] = (u8)(bits >> 8);
}

int main(void)
{
    u8 controller = 0;
    Obj8014A2E4 before[96];
    memset(D_801202A0, 0x5c, sizeof D_801202A0);
    for (unsigned i = 0; i < 96; ++i)
        set_key(i, 0x8123, (s16)((int)i - 50));
    for (unsigned i = 0; i < 96; ++i)
        assert(func_8014C118(&controller, 0x8123, (s16)((int)i - 50)) == &D_801202A0[i]);

    set_key(0, 0, -32768);
    set_key(36, 0xffff, 32767);
    set_key(95, 0xffff, 32767);
    memcpy(before, D_801202A0, sizeof before);
    assert(func_8014C118(NULL, 0, -32768) == D_801202A0);
    assert(func_8014C118(NULL, 0xffff, 32767) == &D_801202A0[36]);
    assert(func_8014C118(NULL, -1, 32767) == NULL);
    assert(func_8014C118(NULL, 0x10000, -32768) == NULL);
    assert(func_8014C118(NULL, 0xffff, -1) == NULL);
    assert(memcmp(D_801202A0, before, sizeof before) == 0);
    set_key(36, 0xffff, -1);
    assert(func_8014C118(NULL, 0xffff, 32767) == &D_801202A0[95]);
    return 0;
}
