/* Owner re-derivation from retail assembly (clean input set). */
/* Dispatch-table handler (main_0012.s D_80180750 .word slot). Bumps a global state machine
 * past value 4, optionally fires a one-shot effect + counter when D_801B6448 is armed, and
 * advances o->f20's angle field by 0x16 (wrapped to 0xFFF). */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x12];
    u16 f12;
} Sub;

typedef struct {
    u8 pad0[0x20];
    Sub *f20;
} Obj;

extern s32 D_801B6440;
extern s32 D_801B6448;
extern u8 D_800AE6BD;
extern void func_80145BF8(void);
extern void func_80146CA0(void *o);

#ifdef __mips__
#define RETURN_NO_VALUE return
#else
#define RETURN_NO_VALUE return 0
#endif

s32 func_8015934C(Obj *o)
{
    if (D_801B6440 == 4) {
        D_801B6440 = 5;
    }
    if (D_801B6448 != 0) {
        func_80145BF8();
        func_80146CA0(o);
        D_800AE6BD++;
    }
    o->f20->f12 = (o->f20->f12 + 0x16) & 0xFFF;
    RETURN_NO_VALUE;
}
