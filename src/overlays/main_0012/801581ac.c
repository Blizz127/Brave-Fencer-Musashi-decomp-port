/* Owner re-derivation from retail assembly (clean input set). */
/* Returns retail's leftover v0 (see ledger). */
/* Enters a state: sets flag bit 1, runs the setup helpers and picks the follow-up by the +0xB8 value; unused frame locals keep the retail 0x28 frame. */
#include "psx_types.h"
typedef struct { u8 p0[0x44]; u32 f44; u8 p48[0x70]; u16 fB8; u8 pBA[0xF0]; u8 f1AA; } O;
extern void func_80148038(O *o, s32 a);
extern void func_80147460(O *o);
extern void func_80147AD4(O *o, s32 a, s32 b, s32 c);
extern void func_801473EC(O *o);
extern s32 func_80161418(O *o);
extern void func_801599A4(O *o);
extern s32 func_80159B3C(O *o);
extern s32 func_80155FF8(O *o, s32 a);
s32 func_801581AC(O *o) {
    s32 pad[4];
    s32 r;
    o->f44 |= 2;
    func_80148038(o, 0x10000);
    func_80147460(o);
    func_80147AD4(o, 0, 0, 0);
    func_801473EC(o);
    if (func_80161418(o) == 0) {
        if (o->fB8 == 0x8000) {
            func_801599A4(o);
            r = func_80159B3C(o);
        } else {
            r = func_80155FF8(o, o->f1AA);
        }
        return r;
    }
    return 0x8000;
}
