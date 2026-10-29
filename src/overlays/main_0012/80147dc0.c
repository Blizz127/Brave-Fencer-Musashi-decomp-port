/* func_80147DC0 - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern void func_80013F3C(void *m);
extern void func_800126C4(void *m, s32 a);
extern void func_80012558(void *m, s32 a);
extern void func_800123F0(void *m, s32 a);
extern void func_800484EC(void *, void *, void *);

void func_80147DC0(u8 *o, void *out) {
    s32 unused[4]; /* 16 unreferenced frame bytes below the matrix */
    s32 m[8];
    u8 *r;
    r = (u8 *)*(u32 *)(o + 0x20);
    func_80013F3C(m);
    func_800126C4(m, *(s16 *)(r + 0x10));
    func_80012558(m, *(s16 *)(r + 0x12));
    func_800123F0(m, *(s16 *)(r + 0x14));
    func_800484EC(m, o + 0x24, out);
}
