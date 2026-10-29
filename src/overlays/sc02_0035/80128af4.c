/* Owner re-derivation from retail assembly (clean input set). */
/* State machine tick on D_800B99F6: state 0 starts a thing via func_8002AF08() (latched into
 * D_801BD598) then func_80011C10(); state 1 runs the D_801844B0 handler then func_80011C10();
 * state 2 runs the D_801844B4 handler and, if it returns nonzero, branches on whether
 * D_801BD598 == 1 to pick a func_800CFBE8()/func_800CFBBC() combo (func_80011B7C(0x13 or 4)) or
 * else func_80010DE0(), then always finishes with func_8002AEF8(); any other state is a no-op. */
#include "psx_types.h"

extern u16 D_800B99F6;
extern s32 D_801BD598;
extern void (*D_801844B0)(void);
extern s16 (*D_801844B4)(void);
extern s32 func_8002AF08(void);
extern void func_80011C10(void);
extern s32 func_800CFBE8(void);
extern void func_800CFBBC(void);
extern void func_80011B7C(s32 a);
extern void func_80010DE0(void);
extern void func_8002AEF8(void);

void func_80128AF4(void)
{
    u16 state;

    state = D_800B99F6;

    switch (state) {
    case 0:
        D_801BD598 = func_8002AF08();
        func_80011C10();
        break;
    case 1:
        D_801844B0();
        func_80011C10();
        break;
    case 2: {
        s16 r;

        r = D_801844B4();
        if (r != 0) {
            if (D_801BD598 == 1) {
                if (func_800CFBE8() != 0) {
                    func_800CFBBC();
                    func_80011B7C(0x13);
                } else {
                    func_80011B7C(4);
                }
            } else {
                func_80010DE0();
            }
            func_8002AEF8();
        }
        break;
    }
    }
}
