/* Advance the actor until its contact check succeeds or its timer expires. */
#include "psx_types.h"

extern void func_8012AD80(u8 *actor);
extern s32 func_8012D5E4(s32 actor, s32 start, s32 end, s32 mode);
extern void func_8012C218(u8 *actor);
extern u8 D_8018B00C[], D_800D3918[];

void func_80184A00(u8 *actor)
{
    func_8012AD80(actor);
    if (func_8012D5E4((s32)(unsigned long)actor,
                      (s32)(unsigned long)D_8018B00C,
                      (s32)(unsigned long)D_800D3918, 10) ||
        --*(s32 *)(actor + 0x1c) == 0) {
        func_8012C218(actor);
    }
}
