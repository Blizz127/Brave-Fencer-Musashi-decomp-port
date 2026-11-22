/* Bind a text record and a numeric actor-kind record to the overlay display. */
#include "psx_types.h"

extern u8 D_801D8E08[];
extern void func_8001534C(s16 slot, s32 value, s16 x, s16 y, u8 flags, u8 extra);
extern void func_800153CC(s16 slot, s32 value, s16 x, s16 y, u8 flags, u8 extra);

void func_80178438(const u16 *actor_kind)
{
    func_8001534C(2, (s32)(unsigned long)D_801D8E08, 128, 136, 0, 0);
    func_800153CC(3, *actor_kind, 168, 136, 64, 0);
}
