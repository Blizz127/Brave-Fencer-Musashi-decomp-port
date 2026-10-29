/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad0[4];
    u16 h4;
    u16 h6;
    u16 h8;
    u16 hA;
    u16 hC;
    u16 hE;
    u8 pad10[0x78];
    u16 h88;
    u16 h8A;
    u16 h8C;
} O;

extern void func_80149020(O *o);
extern s32 func_80153800(O *o);
extern void func_80146D90(O *o);
extern void func_801522CC(O *o);
extern void func_80159B3C(O *o);
extern void func_80165718(O *o);

void func_80152254(O *o) {
    s32 pad[4];
    func_80149020(o);
    if (func_80153800(o) == 0) {
        o->h4 = 0;
        o->h8 = 0;
        o->hC = 0;
        o->h6 = o->h88;
        o->hA = o->h8A;
        o->hE = o->h8C;
        func_80146D90(o);
        func_801522CC(o);
        func_80159B3C(o);
        func_80165718(o);
    }
}