#include "game_option_values.h"

extern s16 D_801853F2;
extern s16 func_8017BFF8(void);
extern s32 func_80028D9C(void);
extern void func_8002D4C8(u16 command, u16 argument);
extern void func_80145934(void);
extern void func_800D1D24(void);

void func_8017BF70(void)
{
    D_801853F2 = func_8017BFF8();
    if (D_801853F2 < 0) {
        func_8002D4C8(33, func_80028FBC());
        func_8002D4C8(34, func_80029000());
        func_8002D4C8(func_80028D9C() == 0 ? 12 : 11, 0);
        func_80145934();
        func_800D1D24();
    }
}
