/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"
extern void func_80149020(u8 *o);
extern void func_80147A84(u8 *o);
extern void func_801473EC(u8 *o);
extern void func_80148038(u8 *o, s32 a);
extern void func_80147460(u8 *o);
extern s32 func_801535F4(u8 *o);
extern void func_801525F4(u8 *o);
extern void func_80159B3C(u8 *o);
extern void func_8015BF48(u8 *o);
extern void func_8014C010(u8 *o, s32 a);
extern void func_80165718(u8 *o);

void func_80152500(u8 *o)
{
    s32 t;

    func_80149020(o);
    t = *(s32 *)(o + 0x1C);
    if (t == 0) {
        if (*(u16 *)(o + 0xB8) == 0x8000) {
            func_801525F4(o);
            func_80159B3C(o);
            func_80165718(o);
            return;
        }
    } else {
        *(s32 *)(o + 0x1C) = t - 1;
    }
    func_80147A84(o);
    func_801473EC(o);
    func_80148038(o, 0x8000);
    func_80147460(o);
    switch (func_801535F4(o)) {
    case 0:
        break;
    case 1:
        func_801525F4(o);
        func_80159B3C(o);
        func_8014C010(o, 1);
        func_80165718(o);
        break;
    case 2:
        func_801525F4(o);
        func_8015BF48(o);
        func_8014C010(o, 1);
        func_80165718(o);
        break;
    }
}
