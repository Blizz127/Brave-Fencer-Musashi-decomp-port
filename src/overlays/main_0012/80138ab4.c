/* func_80138AB4 [80138AB4,80138B88) - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 p0[4];
    s16 f4;
    u8 p6[2];
    u32 f8;
    u8 pC[0x38];
    u16 f44;
    u8 p46[2];
    u16 f48;
} O;

/* callers read v0: no (the direct caller ignores it); void.
 * D_80127540 is s16[] in the owner sources; retail reads it with lhu, so it is viewed as u16 at the use site. */
extern s16 D_80127540[];
extern s32 func_80139D04(u8 *buf, s32 a);
extern s32 func_80138DE0(O *o, s32 ch, s32 a);
extern void func_80139B18(O *o);

void func_80138AB4(O *o) {
    u8 buf[8];
    s32 n;
    s32 cont;

    n = func_80139D04(buf, *(u16 *)&D_80127540[o->f48]) & 0xFFFF;
    do {
        cont = 0;
        if (o->f44 < n) {
            cont = func_80138DE0(o, (buf[o->f44] + 0x30) & 0xFF, 0);
            if ((o->f8 & 0x80220) == 0) {
                cont = 0;
            }
        } else {
            if (o->f48 < 3) {
                o->f48 = o->f48 + 1;
            }
            o->f4 = 2;
        }
    } while (cont != 0);
    func_80139B18(o);
}
