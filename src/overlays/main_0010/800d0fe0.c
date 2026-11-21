/* Empty the inventory slot. The acquisition stamp is stored separately. */
#include "psx_types.h"

extern void func_800291A0(s32 index, s32 value);

void func_800D0FE0(s32 slot)
{
    func_800291A0(slot, 0);
}
