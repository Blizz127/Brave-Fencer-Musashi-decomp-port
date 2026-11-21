/* Write the item first, then stamp its inventory slot with the current day. */
#include "psx_types.h"

extern s16 D_80078EAC;
extern void func_800291A0(s32 index, s32 value);
extern void func_800291C8(s32 index, s16 value);

void func_800D0F0C(u32 slot, u32 item)
{
    func_800291A0(slot, (u8)item);
    func_800291C8((s32)(2u * slot - 0x4au), D_80078EAC);
}
