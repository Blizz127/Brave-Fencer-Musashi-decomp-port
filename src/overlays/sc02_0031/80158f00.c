/* Owner re-derivation from retail assembly (clean input set). */
/* Advances o->sub->f12 by 0x16 (wrapped to 0xFFF) always; if D_801B644C is still 0,
 * also picks func_8005C49C()%11, looks it up in D_80186FE0, runs func_8013767C on it,
 * bumps o's counter (func_80146CA0) and stashes the lookup's return in o->f198. void,
 * no caller found in any region. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x12];
    u16 f12;       /* 0x12 */
} Sub;

typedef struct {
    u8 pad0[0x20];
    Sub *sub;      /* 0x20 */
    u8 pad24[0x198 - 0x24];
    s32 f198;      /* 0x198 */
} Obj;

extern s32 D_801B644C;
extern s32 D_80186FE0[];
extern s32 func_8005C49C(void);
extern u32 func_8013767C(u32 a);
extern void func_80146CA0(Obj *o);

void func_80158F00(Obj *o)
{
    Sub *s;
    s32 r;

    s = o->sub;
    s->f12 = (s->f12 + 0x16) & 0xFFF;
    if (D_801B644C != 0) {
        return;
    }

    r = func_8005C49C();
    r = r % 11;
    o->f198 = func_8013767C((u32)D_80186FE0[r]);
    func_80146CA0(o);
}
