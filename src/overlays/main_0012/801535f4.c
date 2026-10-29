/* Owner re-derivation from retail assembly (clean input set). */
/* Input classifier: returns 2/1/4 for the pressed-button bits, else 1 if a pending timer hit, else 0. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x34];
    s32 timer; /* 0x34 */
} Obj801535F4;

extern s32 func_8014ED28(Obj801535F4 *o);
extern void func_80146D90(Obj801535F4 *o);
extern s32 func_8014CC28(Obj801535F4 *o);
extern s32 func_8014FA04(Obj801535F4 *o);

s32 func_801535F4(Obj801535F4 *o)
{
    s32 hit;
    s32 keys;
    s32 k2;

    hit = 0;
    if (func_8014ED28(o) != 0) {
        func_80146D90(o);
    }
    if (o->timer > 0) {
        if (func_8014CC28(o) != 0) {
            hit = 1;
        }
    }
    keys = func_8014FA04(o);
    k2 = keys;
    if (keys & 0x4000) {
        return 2;
    }
    if (keys & 0x2000) {
        return 1;
    }
    if (k2 & 0x8000) {
        return 4;
    }
    return hit;
}
