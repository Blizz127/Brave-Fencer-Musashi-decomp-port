/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x12];
    u16 f12;      /* 0x12 */
} Sub;

typedef struct {
    u8 pad0[0x20];
    Sub *f20;     /* 0x20 */
} Obj;

extern s16 D_80126B3C;
extern s32 func_80012B04(s16 a0, s16 a1, s32 a2);
extern u16 D_8011F9C8;
extern void func_80171A1C(Obj *o);
extern void func_80174650(Obj *o);

void func_801741EC(Obj *o) {
    s32 r = func_80012B04((s16)o->f20->f12, D_80126B3C, 4);

    if ((s16)r == 0) {
        D_8011F9C8 = 1;
        func_80171A1C(o);
        func_80174650(o);
    }
    o->f20->f12 = (o->f20->f12 + r) & 0xFFF;
}
