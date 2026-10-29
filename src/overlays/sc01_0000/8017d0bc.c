/* Owner re-derivation from retail assembly (clean input set). */
/* Copies six s16 values (D_80182698..D_801826A4) into the s32 globals D_80126984..D_80126998, registers func_8017D1B4 through func_8012A018 with 1, then runs func_8012A094 and func_8017D1B4 on the record D_80126948. */
#include "psx_types.h"

extern s16 D_80182698;
extern s16 D_8018269A;
extern s16 D_8018269C;
extern s16 D_801826A0;
extern s16 D_801826A2;
extern s16 D_801826A4;
extern s32 D_80126984;
extern s32 D_80126988;
extern s32 D_8012698C;
extern s32 D_80126990;
extern s32 D_80126994;
extern s32 D_80126998;
extern u8 D_80126948[];
extern void func_8012A018(void (*fn)(u8 *), s32 a);
extern void func_8012A094(u8 *p);
extern s32 func_8017D1B4(u8 *p);

/* Returns retail's leftover v0: func_8017D1B4's result. */
s32 func_8017D0BC(void)
{
    D_80126984 = D_80182698;
    D_80126988 = D_8018269A;
    D_8012698C = D_8018269C;
    D_80126990 = D_801826A0;
    D_80126994 = D_801826A2;
    D_80126998 = D_801826A4;
    func_8012A018((void (*)(u8 *))func_8017D1B4, 1);
    func_8012A094(D_80126948);
    return func_8017D1B4(D_80126948);
}
