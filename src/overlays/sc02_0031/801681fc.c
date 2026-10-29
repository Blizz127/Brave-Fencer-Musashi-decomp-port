/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

/* Dispatch-table handler (D_80181330-class tables, reached only via a jalr
 * through func_80166618/func_80166654); confirmed void: none of the jalr
 * dispatchers in any overlay image (sc01_0000, sc01_0005, sc01_0077,
 * sc02_0031, sc02_0035, sc03_0001, main_0012) read v0 after the call. */

typedef struct {
    u8 p0[2];
    u16 f2;
    u8 p4[0x18];
    s32 f1C;
    u8 p20[8];
    u16 f28;
    u16 f2A;
    s32 f2C;
    s32 f30;
} O;

extern s32 func_80029178(s32 a);
extern void func_80166F58(O *o, s32 a, s32 b, s32 c);
extern u8 D_80187BD4[];
extern u8 D_80187BD5[];
extern u8 D_80187C04[];

void func_801681FC(O *o) {
    s32 idx;
    s32 raw;
    s32 idx2;
    s32 v;
    s32 flag;

    o->f28 = (u16)(func_80029178(0x81) & 0xFF);
    idx = o->f2C;
    o->f30 = D_80187C04[idx];
    raw = o->f2C;
    idx2 = (raw << 17) >> 16;
    o->f2A = D_80187BD4[idx2];
    v = D_80187BD5[idx2];
    flag = o->f2C & 1;
    o->f2C = flag;
    o->f1C = v;
    func_80166F58(o, 4, 4, flag);
    o->f2 = o->f2 + 1;
}
