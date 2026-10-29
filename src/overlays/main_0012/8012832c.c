/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"
extern s16 D_800B9A08;
extern s32 func_80029504(void);
extern s32 func_800CF854(void);
extern s32 func_801289F0(void);
extern s32 func_80128998(void);
extern s32 func_801288E8(void);
extern s32 func_80128940(void);
extern s32 func_801288B0(void);
extern u8 func_80029178(s32 a);
extern void func_80011C10(void);

void func_8012832C(void)
{
    s32 r;
    s32 v;

    r = func_80029504();
    switch (D_800B9A08) {
    case 0x300E:
        if (func_800CF854() == 0) {
            v = func_80128998();
        } else {
            v = func_801289F0();
        }
        break;
    case 0x30A0:
        if (r < 0x640) {
            v = func_801288E8();
        } else {
            v = func_80128940();
        }
        break;
    case 0x3014:
        if (r != 0x4B0) {
            v = 1;
        } else if (func_80029178(0x138) != 0) {
            v = 1;
        } else {
            v = func_801288B0();
        }
        break;
    default:
        func_80011C10();
        return;
    }
    if (v != 0) {
        func_80011C10();
    }
}
