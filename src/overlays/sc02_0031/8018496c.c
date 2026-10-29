/* Owner re-derivation from retail assembly (clean input set). */
/* Per-frame step: when the object's +0xFC value differs from the config halfword at +0x36 of the record at +0x64 runs func_8012C218; otherwise on +0xFE == 1 or 2 enters state 2, loads timer 0x20, applies the matching record (D_801AB608 or D_801AB710) via func_8012A828 and sets the +0x5C value. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x36];
    s16 h36;
} Cfg;

typedef struct {
    u8 pad0[2];
    u16 sel;
    u8 pad4[0x18];
    s32 w1C;
    u8 pad20[0x3C];
    u16 h5C;
    u8 pad5E[6];
    Cfg *cfg;
    u8 pad68[0x94];
    s16 hFC;
    s16 hFE;
} Obj;

extern u8 D_801AB608[];
extern u8 D_801AB710[];
extern void func_8012C218(Obj *o);
extern void func_8012A828(Obj *o, void *p);
extern void func_8002D4C8(s32 id, s32 v);

void func_8018496C(Obj *o)
{
    if (o->hFC != o->cfg->h36) {
        func_8012C218(o);
    } else if (o->hFE == 1) {
        o->sel = 2;
        o->w1C = 0x20;
        func_8012A828(o, D_801AB608);
        o->h5C = 0;
        func_8002D4C8(0x5BF, 0);
    } else if (o->hFE == 2) {
        o->sel = 2;
        o->w1C = 0x20;
        func_8012A828(o, D_801AB710);
        o->h5C = 0x8C00;
    }
}
