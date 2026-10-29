/* func_80130C08 [80130C08,80130D0C) - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 p0[0x72];
    u16 f72;
    u8 p74[0x40];
    u32 fB4;
    u8 pB8[0xA];
    u8 fC2;
    u8 pC3;
    u32 fC4;
} O;

/* callers read v0: none-direct (no jal caller); void.
 * func_8012CBCC is passed o (retail leaves a0 = o); owner prototype takes an int. */
extern int func_8012CBCC(int a0);
extern s32 func_8012BEE8(O *o);
extern void func_80131E00(O *o, s32 a);
extern void func_80131C78(O *o);
extern void func_80131CA8(O *o, s32 a);

void func_80130C08(O *o) {
    s32 f;
    s32 r;
    u32 t;

    f = 0;
    if ((o->fC4 & 2) == 0) {
        t = o->fB4;
        if (t & 0x20000) {
            if (t & 1) {
                if ((func_8012CBCC((int)o) & 0xFF) == 2) {
                    func_80131E00(o, 0x12);
                    return;
                }
                if (o->fC2 != 0) {
                    r = func_8012BEE8(o);
                } else {
                    r = o->f72 & 0x4000;
                }
                if (r) {
                    f = 1;
                }
                if (f) {
                    func_80131C78(o);
                }
            }
            func_80131CA8(o, 5);
        }
    }
    if (o->fC4 & 2) {
        func_80131CA8(o, 5);
    } else {
        func_80131CA8(o, 0x38);
    }
}
