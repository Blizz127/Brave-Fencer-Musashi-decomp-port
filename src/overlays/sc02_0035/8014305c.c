/* Owner re-derivation from retail assembly (clean input set). */
/* Grabs a func_8012C1B8() slot into o->f20. If none, detaches via func_8012CAE4(o). Otherwise
 * runs func_8001CA1C(slot, &D_80185B14), packs o->f70's low nibble into the slot's +0x18/+0x1A
 * fields, clears o->fFC, then func_8012A828(o, (u32)&D_80185B74); func_80142B2C(o). Same shape
 * as the matched func_80142EC0/func_8012A828 pair, but the second operand is a fixed data
 * address rather than a D_80185AF4 table lookup. */
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
extern s32 func_8001CA1C(SlotObj *s, void *a);
extern u8 D_80185B14[];
extern u8 D_80185B74[];
extern s32 func_8012A828(Obj *o, u32 a);
extern void func_80142B2C(Obj *o);

void func_8014305C(Obj *o)
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

    func_8001CA1C(s, D_80185B14);

    {
        u32 idx = o->f70 & 0xF;
        s32 packed = idx * 3;
        packed <<= 9;
        packed += 0xC00;
        s->f1A = packed;
        s->f18 = packed;
    }

    o->fFC = 0;
    func_8012A828(o, (u32)D_80185B74);
    func_80142B2C(o);
}
