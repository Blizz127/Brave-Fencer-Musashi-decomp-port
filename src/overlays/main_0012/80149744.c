/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 _0[0x44];
    u32 f44;
    u8 _48[0x64];
    u16 fAC;
} Obj;

extern u8 D_80078EBF;

s32 func_80149744(Obj *o) {
    if (!(o->f44 & 4) && D_80078EBF >= 0x80 && (o->fAC & 0x80)) {
        return 1;
    }
    return 0;
}
