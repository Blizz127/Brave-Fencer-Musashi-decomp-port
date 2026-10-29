/* Owner re-derivation from retail assembly (clean input set). */
/* Attack hit-box table init: unless D_80115128 is set, write six scripted
 * hit-box records (D_80185C14/34/38/3C/40/44) via func_800D2CA8 (angle
 * lookup, second arg unused by the callee but still set up by the caller)
 * and func_800D2D10 (record/offset/slot write), plus two extra direct
 * func_8005C540 and func_8002AAB4 calls for the first record. */
#include "psx_types.h"

extern s32 func_80029FD4(void);
extern s32 func_80029FE4(void);
extern s32 func_8002A26C(void);
extern s32 func_8002A2B0(void);
extern s32 func_8002A4B8(void);
extern s32 func_8002A4FC(void);
extern s32 func_8002A728(void);
extern s32 func_8002A76C(void);
extern s32 func_8002A998(void);
extern s32 func_8002A9DC(void);
extern s32 func_8002AAB4(void);
extern void func_8005C540(s32 a0, s32 a1);
extern s32 func_800D2CA8(s32 a0, s32 a1);
extern void func_800D2D10(s32 a0, s32 a1, s32 a2, s32 a3);

extern s16 D_80115128;
extern s32 D_80185C14;
extern s32 D_80185C34;
extern s32 D_80185C38;
extern s32 D_80185C3C;
extern s32 D_80185C40;
extern s32 D_80185C44;

void func_80140F00(void)
{
    s32 base;

    if (D_80115128 == 0) {
        base = D_80185C14;
        func_800D2D10(func_800D2CA8(func_80029FE4() + 1, 0x18), 2, base + 4, 1);
        func_8005C540(base + 0xA, func_8002AAB4());

        base = D_80185C34;
        func_800D2D10(func_800D2CA8(func_8002A26C() + 1, 0x18), 2, base + 0x12, 1);
        func_800D2D10(func_800D2CA8(func_8002A2B0(), 0x18), 2, base + 0x22, -1);

        base = D_80185C38;
        func_800D2D10(func_800D2CA8(func_8002A4B8() + 1, 0x18), 2, base + 0x12, 1);
        func_800D2D10(func_800D2CA8(func_8002A4FC(), 0x18), 2, base + 0x22, -1);

        base = D_80185C3C;
        func_800D2D10(func_800D2CA8(func_8002A998() + 1, 0x18), 2, base + 0x12, 1);
        func_800D2D10(func_800D2CA8(func_8002A9DC(), 0x18), 2, base + 0x22, -1);

        base = D_80185C40;
        func_800D2D10(func_800D2CA8(func_8002A728() + 1, 0x18), 2, base + 0x12, 1);
        func_800D2D10(func_800D2CA8(func_8002A76C(), 0x18), 2, base + 0x22, -1);

        base = D_80185C44;
        func_800D2D10(func_800D2CA8(func_80029FD4(), 4), 7, base + 0x18, -1);
    }
}
