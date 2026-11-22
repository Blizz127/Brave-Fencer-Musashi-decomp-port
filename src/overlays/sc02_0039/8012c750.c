/* Choose the first free actor in placement order, then return the initializer's result. */
#include "mine_initial_placement.h"
#include "../main_0012/scene_object_probes.h"

extern s32 func_8012C890(void *row, u8 *slot, u8 *owner);

#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
/* The historical linker places this symbol one slot before the pool. */
extern Obj8014A2E4 D_80120194[];
#endif

s32 func_8012C750(MineInitialPlacement *row)
{
    Obj8014A2E4 *slot;
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
    Obj8014A2E4 *bound;
    Obj8014A2E4 *current;
    s32 result;

    if (row->flags & 0x800) {
        bound = D_80120194;
        slot = bound + 96;
        for (; slot != bound; --slot) {
            if (slot->active == 0)
                goto found;
        }
        slot = 0;
    } else {
        bound = D_80120194;
        slot = 0;
        if (slot == 0)
            goto forward;
    hit:
        slot = current;
        goto found;
    forward:
        slot = bound + 97;
        for (current = D_801202A0; current != slot; ++current) {
            if (current->active == 0)
                goto hit;
        }
        slot = 0;
    }
found:
    if (slot == 0) {
        result = 0;
    } else {
        row->flags |= 0x8000;
        result = func_8012C890(row, (u8 *)slot, 0);
    }
    return result;
#else
    unsigned int n;
    for (n = 0; n < 96; ++n) {
        unsigned int index = (row->flags & 0x800) ? 95 - n : n;
        slot = &D_801202A0[index];
        if (slot->active == 0) {
            row->flags |= 0x8000;
            return func_8012C890(row, (u8 *)slot, 0);
        }
    }
    return 0;
#endif
}
