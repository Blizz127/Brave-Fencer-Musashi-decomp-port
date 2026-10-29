/* func_8014AA28 [8014AA28,8014AB7C) - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u16 f0;
    u8 p2[0x1C6];
    u16 f1C8;
    u16 f1CA;
    u16 f1CC;
    u16 f1CE;
    u16 f1D0;
    u8 p1D2[2];
    u32 f1D4;
    u32 f1D8;
    u8 p1DC[0x47];
    u8 f223;
} O;

/* callers read v0: none-direct (jal; nop); void. */
extern void func_801599A4(O *o);
extern void func_80159B3C(O *o);
extern void func_80165A20(O *o);
extern void func_8014AB7C(s32 id);
extern void func_8014AC10(s32 id);
extern u8 D_80126B58[];
extern u8 D_80078EC1;

void func_8014AA28(void) {
    O *o;
    s32 u;

    o = (O *)D_80126B58;
    u = D_80078EC1;
    if ((u32)(o->f0 - 0x19) >= 2 && (u32)(o->f0 - 0x1D) >= 2) {
        func_801599A4(o);
        func_80159B3C(o);
        if (u == 0xC || u == 0x18 || u == 0xA) {
            func_80165A20(o);
        }
    }
    o->f1C8 = 0;
    o->f1CA = 0;
    o->f1CC = 0;
    o->f1CE = 0;
    o->f1D0 = 0;
    o->f1D4 = 0;
    o->f1D8 = 0;
    func_8014AB7C(5);
    func_8014AB7C(6);
    func_8014AB7C(7);
    func_8014AB7C(8);
    func_8014AB7C(9);
    func_8014AC10(0x26);
    func_8014AC10(0x27);
    func_8014AC10(0x39);
    func_8014AC10(0x3A);
    func_8014AB7C(0x10);
    func_8014AB7C(0x11);
    func_8014AB7C(0x12);
    func_8014AB7C(0x13);
    func_8014AB7C(0x14);
    func_8014AC10(0x3B);
    func_8014AC10(0x3C);
    o->f223 = 0x10;
}
