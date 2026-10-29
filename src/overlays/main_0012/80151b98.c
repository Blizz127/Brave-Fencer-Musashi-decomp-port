/* func_80151B98 - clean-room owner re-derivation (nmG). */
/* Per-tick state step: in the 0x8000 state runs the idle helpers; otherwise dispatches on func_801535F4
 * (1: idle helpers + func_8014C010, 2: func_8015BF48 + func_8014C010); every handled path ends in func_80165718.
 * The explicit `case 0: return;` gives gcc 2.7.2 three case nodes (root 1, both subtrees unbounded),
 * which is the retail beq 1 / slti 2 / beq 2 dispatch. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0xB8];
    u16 state; /* 0xB8 */
} Obj80151B98;

extern void func_80149020(Obj80151B98 *o);
extern void func_80151C54(Obj80151B98 *o);
extern void func_80159B3C(Obj80151B98 *o);
extern s32 func_801535F4(Obj80151B98 *o);
extern void func_8014C010(Obj80151B98 *o, s32 a);
extern void func_8015BF48(Obj80151B98 *o);
extern void func_80165718(Obj80151B98 *o);

void func_80151B98(Obj80151B98 *o)
{
    s32 r;

    func_80149020(o);
    if (o->state == 0x8000) {
        func_80151C54(o);
        func_80159B3C(o);
    } else {
        r = func_801535F4(o);
        switch (r) {
        case 0:
            return;
        case 1:
            func_80151C54(o);
            func_80159B3C(o);
            func_8014C010(o, 1);
            break;
        case 2:
            func_80151C54(o);
            func_8015BF48(o);
            func_8014C010(o, 1);
            break;
        default:
            return;
        }
    }
    func_80165718(o);
}
