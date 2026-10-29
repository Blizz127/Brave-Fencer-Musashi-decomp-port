/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"
typedef struct { u8 b[8]; } B8_80173A60;
extern u8 D_80126B58[];
extern s16 D_80126B30;
extern u8 D_80126B38[];
extern s16 D_80126B3A;
extern s16 D_80126B3C;
extern s32 D_80127508;
extern s16 D_8011F9C8;
extern s16 D_80126938;
extern s32 D_8012750C;
extern s32 func_8014C118(s32 unused, s32 id, s16 v);
extern void func_80173B4C(void);
extern void func_80174684(void (*cb)(void));

void func_80173A60(s32 a, s32 b, s32 flag, s32 c, B8_80173A60 *src)
{
    u8 *st;

    st = D_80126B58;
    if (flag != 0) {
        D_80127508 = func_8014C118((s32)st, flag, c);
    } else {
        D_80127508 = 0;
    }
    (*(B8_80173A60 *)&D_80126B30) = *src;
    (*(u16 *)&D_80126B38) = a;
    (*(u16 *)&D_80126B3A) = b;
    (*(u16 *)&D_80126B3C) = *(u16 *)(*(u8 **)(st + 0x20) + 0x12);
    func_80174684(func_80173B4C);
    D_8011F9C8 = 0;
    D_80126938 = 0;
    D_8012750C = 0;
}
