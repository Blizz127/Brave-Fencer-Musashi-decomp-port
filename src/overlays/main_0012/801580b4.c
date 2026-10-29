/* func_801580B4 [801580B4,801581AC) - owner re-derivation from retail assembly (clean input set). */
/* Returns retail's leftover v0 (see ledger). */
#include "psx_types.h"

typedef struct {
    u8 p0[0x44];
    u32 f44;
    u8 p48[0x70];
    u16 fB8;
    u8 pBA[0xF0];
    u8 f1AA;
    u8 p1AB[0x89];
    s32 f234;
} O;

/* no direct jal caller; retail leaves the last callee (or compare-constant) value in v0, returned here. */
extern void func_801477E8(O *o, s32 a);
extern void func_80148038(O *o, s32 a);
extern void func_80147460(O *o);
extern void func_80147AD4(O *o, s32 a, s32 b, s32 c);
extern void func_801473EC(O *o);
extern s32 func_80161418(O *o);
extern void func_80156648(O *o);
extern void func_80154274(O *o, void *a);
extern void func_80147324(s32 a);
extern s32 func_80146CA0(O *o);
extern s32 func_80155FF8(O *o, s32 a);
extern u8 D_800D524C;

s32 func_801580B4(O *o) {
    s32 pad[4]; /* unreferenced: retail frame keeps 16 bytes below the saves */
    s32 r;
    o->f44 |= 2;
    if (o->fB8 & 0x4000) {
        func_801477E8(o, -0xC0000);
        o->f234++;
    }
    if (o->f234 != 0) {
        func_80148038(o, 0x10000);
        func_80147460(o);
    }
    func_80147AD4(o, 0, 0, 0);
    func_801473EC(o);
    if (func_80161418(o) == 0) {
        if (o->fB8 == 0x8000) {
            func_80156648(o);
            o->f1AA = 0xF;
            func_80154274(o, &D_800D524C);
            func_80147324(0x455);
            r = func_80146CA0(o);
        } else {
            r = func_80155FF8(o, o->f1AA);
        }
        return r;
    }
    return 0x8000;
}
