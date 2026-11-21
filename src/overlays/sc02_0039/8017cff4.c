#include "scene_countdown.h"

extern u32 D_80182B64[];
extern s32 func_8016EE40(u32 *record, u32 flags);

void func_8017CFF4(void)
{
    func_8016EE40(D_80182B64, 0x01000000u);
}
