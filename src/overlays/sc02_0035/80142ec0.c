/* Owner re-derivation from retail assembly (clean input set). */
/* Grabs a func_8012C1B8() slot into o->f20. If none, detaches via func_8012CAE4(o). Otherwise
 * packs o->f70's low nibble into the slot's +0x18/+0x1A fields, clears o->fFC, loads the
 * D_80185AF4 table entry for o->f70's low nibble as a plain value arg, and runs
 * func_8012A828(o, value); func_80142B2C(o). */
#include "psx_types.h"

typedef struct {
    u8 pad00[0x20];
    u32 f20;  /* 0x20 */
    u8 pad24[0x70 - 0x24];
    u16 f70;  /* 0x70 */
    u8 pad72[0xFC - 0x72];
    u16 fFC;  /* 0xFC */
} Obj;

typedef struct {
    u8 pad00[0x18];
    u16 f18; /* 0x18 */
    u16 f1A; /* 0x1A */
} SlotObj;

extern u32 func_8012C1B8(void);
extern void func_8012CAE4(Obj *o);
extern s32 func_8001CA1C(SlotObj *s, s32 a);
extern u32 D_80185AF4[];
extern s32 func_8012A828(Obj *o, u32 a);
extern void func_80142B2C(Obj *o);

void func_80142EC0(Obj *o)
{
    u32 raw;
    SlotObj *s;

    raw = func_8012C1B8();
    s = (SlotObj *)raw;
    o->f20 = raw;
    if (raw == 0) {
        func_8012CAE4(o);
        return;
    }

    func_8001CA1C(s, 0);

    {
        u32 idx = o->f70 & 0xF;
        s32 packed = idx * 3;
        packed <<= 9;
        packed += 0xC00;
        s->f1A = packed;
        s->f18 = packed;
    }

    {
        u32 idx = o->f70 & 0xF;
        o->fFC = 0;
        func_8012A828(o, D_80185AF4[idx]);
    }
    func_80142B2C(o);
}
