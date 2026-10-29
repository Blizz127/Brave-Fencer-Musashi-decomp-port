/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct Obj Obj;

extern s16 D_80126B3A;
extern s16 D_80126B38;
extern s16 D_8011F9C8;
extern s32 func_800D0EC4(void);
extern void func_8013E588(void);
extern void func_80174650(Obj *o);
extern void func_80171A34(Obj *o, s32 a1);
extern void func_80171A1C(Obj *o);

void func_80173BC0(Obj *o) {
    if (D_80126B3A != 0 && D_80126B3A < 0x6F && func_800D0EC4() == 0) {
        if (D_80126B38 & 0x8000) {
            func_8013E588();
        } else {
            func_80174650(o);
            D_8011F9C8 = 2;
            func_80171A34(o, 7);
            return;
        }
    }
    func_80171A1C(o);
}
