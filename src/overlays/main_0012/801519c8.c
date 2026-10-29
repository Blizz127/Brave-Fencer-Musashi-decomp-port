/* func_801519C8 [801519C8,801519E4+) - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 p0[0x12];
    s16 f12;
} S;

typedef struct {
    u8 p0[0x20];
    S *f20;
    u8 p24[0xB9];
    u8 fDD;
    u8 fDE;
    u8 pDF[0x13];
    s16 fF2;
    s16 fF4;
} O;

/* callers read v0: none-direct; void. f20 holds a guest address. */
extern void func_80019064(void *p);
extern void func_8014ACE8(O *o, s32 a, s32 b);
extern void func_80151204(O *o, s32 a);
extern s16 func_80012A60(s16 a, s16 b);
extern void func_80154274(O *o, void *p);
extern void func_80154A74(O *o, s32 a);
extern void func_80149020(O *o);
extern void func_80147324(s32 a);
extern void func_80146CA0(O *o);
extern u8 D_80062BF4[];
extern u8 D_800D5880[];
extern u8 D_800D58AC[];

void func_801519C8(O *o) {
    u8 t;
    u8 u;
    s16 x;

    func_80019064(D_80062BF4);
    t = o->fDD;
    if (t != 1 && t == 0x11) {
        x = o->fF2;
        if (x != 0) {
            func_8014ACE8(o, 5, x & 0x7FFF);
        }
    } else {
        func_80151204(o, o->fF2);
    }
    if (func_80012A60(o->f20->f12, o->fF4) > 0x400) {
        func_80154274(o, D_800D5880);
    } else {
        func_80154274(o, D_800D58AC);
    }
    func_80154A74(o, 0x16);
    func_80149020(o);
    func_80147324(0x451);
    u = o->fDE;
    if (u == 2 || u == 3 || u == 0xB) {
        func_80146CA0(o);
    }
    func_80146CA0(o);
}
