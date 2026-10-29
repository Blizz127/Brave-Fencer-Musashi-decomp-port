/* Owner re-derivation from retail assembly (clean input set). */
/* Per-frame step: when the counter is below 2 and input or the +0xA value says so, clamps the +0x14 limit and increments the counter. */
#include "psx_types.h"
typedef struct { u8 pad0[0xA]; s16 fA; u8 pad[8]; s32 f14; u8 pad2[0xEA]; s16 f102; } O;
extern s32 func_8012CBF4(void);
void func_80142C9C(O *o) {
    s32 k;
    if (o->f102 < 2) {
        k = func_8012CBF4();
        if (o->f14 > 0x180000) o->f14 = 0x180000;
        if ((k & 0x6000) || o->fA > 0x100) {
            if (o->f102 == 0) o->f14 = 0xFFF80000;
            o->f102++;
        }
    }
}
