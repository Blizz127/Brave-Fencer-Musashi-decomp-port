/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern void func_80147928(s32 o, s32 a);
extern void func_8014799C(s32 o, s32 a);
extern void func_80147A10(s32 o, s32 a);
extern void func_80147DC0(s32 o, s32 a);

void func_80147CC8(s32 o, s32 a, s32 b, s32 c, s32 d) {
    func_80147928(o, a);
    func_8014799C(o, b);
    func_80147A10(o, c);
    func_80147DC0(o, d);
}