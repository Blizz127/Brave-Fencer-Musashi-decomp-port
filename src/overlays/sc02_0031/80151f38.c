/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x64];
    u16 f64;      /* 0x64 */
    u8 pad66[0xDE - 0x66];
    u8 fDE;       /* 0xDE */
} Obj;

extern void func_80149020(Obj *o);
extern void func_80151FB4(Obj *o);
extern void func_80159B3C(Obj *o);
extern void func_80165718(Obj *o);
extern void func_80172630(Obj *o);

void func_80151F38(Obj *o) {
    func_80149020(o);
    o->f64 = o->f64 - ((s32)o->fDE << 6);
    o->fDE = o->fDE + 8;
    if ((s16)o->f64 < 0x1000) {
        func_80151FB4(o);
        func_80159B3C(o);
        func_80165718(o);
    } else {
        func_80172630(o);
    }
}
