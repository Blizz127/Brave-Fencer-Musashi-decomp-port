/* Owner re-derivation from retail assembly (clean input set). */
/* Unless func_8016F1AC() or the 0x8 flag at 0x1F4 blocks it, converts the vectors at o+4 and o+0x98 (y zeroed) into an angle and passes it to func_80029ED4. */
#include "psx_types.h"

typedef struct {
    s16 v[4];
} Vec8014bde8;

typedef struct {
    u8 pad0[4];
    s16 pos[3]; /* 0x4 */
    u8 pad1[0x98 - 0xA];
    Vec8014bde8 target; /* 0x98 */
} Obj8014bde8;

extern s32 func_8016F1AC(void);
extern s32 func_8017267C(Obj8014bde8 *o);
extern u16 func_80015978(void *src, void *dst);
extern s32 func_80013294(void *a, void *b);
extern void func_80029ED4(s32 angle);

void func_8014BDE8(Obj8014bde8 *o)
{
    Vec8014bde8 a;
    Vec8014bde8 b;

    if (func_8016F1AC() == 0 && func_8017267C(o) == 0) {
        a = o->target;
        func_80015978(&o->pos, &b);
        b.v[1] = 0;
        a.v[1] = 0;
        func_80029ED4((s16)func_80013294(&a, &b));
    }
}
