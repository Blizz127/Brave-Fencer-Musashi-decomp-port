/* func_80145B24 [80145B24,80145BF8) - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

/* callers read v0: no (both callers ignore it; retail sets none); void.
 * func_80165A50 is passed &D_80126B58 here (retail loads a0 before the call); its owner prototype is
 * void(void) -- disagreement, same code. The final store targets D_800B99DA via D_800AF630 + 0xA3DA. */
extern s32 func_80029504(void); /* its result is the s compared with 0x6A4 (callers' v0 ignored) */
extern s32 func_800291DC(s32);
extern void func_800291C8(s32, s16);
extern void func_80162120(void);
extern void func_80029124(s32 a, s32 b);
extern void func_80165A50(char *o);
extern void func_80029514(s32 v);
extern u8 D_800AF630[];
extern u8 D_80078EC0;
extern u8 D_80126B58[];

void func_80145B24(void) {
    s32 s;
    s32 v;
    u8 *base;

    s = func_80029504();
    v = func_800291DC(8);
    base = D_800AF630;
    if ((v & 0x8000) != 0) {
        func_800291C8(8, 0);
        func_80162120();
    }
    if (s >= 0x6A4) {
        func_80029124(0x1A, 0);
        func_80029124(0x8E, 0);
        func_80029124(0x143, 0);
        if ((D_80078EC0 & 0x7F) != 0) {
            D_80078EC0 = 1;
        }
        func_80165A50(((char *)&D_80126B58));
        func_80029514(0x4B0);
    }
    *(u16 *)(base + 0xA3DA) = 0x3000;
}
