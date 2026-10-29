/* Owner re-derivation from retail assembly (clean input set). */
/* func_8012F68C [8012F68C,8012F75C) - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

/* callers read v0: none-direct (no jal caller); void (every path ends in a callee's v0, none set here).
 * func_8012CBCC is passed o (retail leaves a0 = o); its owner prototype takes an int. */
extern int func_8012CBCC(int a0);
extern void func_80131E00(void *o, s32 a);
extern int func_8012BEE8(u8 *o);
extern void func_80131C78(void *o);
extern s32 func_80131CA8(char *o, s32 a);

void func_8012F68C(u8 *o) {
    s32 f;
    s32 t;

    f = 0;
    if ((*(u32 *)(o + 0xC4) & 2) == 0 && (*(u32 *)(o + 0xB4) & 1) != 0) {
        if ((func_8012CBCC((int)o) & 0xFF) == 2) {
            func_80131E00(o, 0x12);
            return;
        }
        if (*(u8 *)(o + 0xC2) != 0) {
            t = func_8012BEE8(o);
        } else {
            t = *(u16 *)(o + 0x72) & 0x4000;
        }
        if (t != 0) {
            f = 1;
        }
        if (f != 0) {
            func_80131C78(o);
        }
    }
    func_80131CA8((char *)o, 5);
}
