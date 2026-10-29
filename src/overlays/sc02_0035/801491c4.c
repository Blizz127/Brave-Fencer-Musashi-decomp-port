/* Owner re-derivation from retail assembly (clean input set). */
/* Returns the s16 angle from func_8004CFEC of the deltas between the position words at +0x3C/+0x44 and +0x48/+0x50 of the record at D_801151D4. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x3C];
    s32 f3C;
    u8 pad40[4];
    s32 f44;
    s32 f48;
    u8 pad4C[4];
    s32 f50;
} Rec;

extern Rec *D_801151D4;
extern s32 func_8004CFEC(s32 y, s32 x);

s16 func_801491C4(void)
{
    return func_8004CFEC(D_801151D4->f3C - D_801151D4->f48, D_801151D4->f44 - D_801151D4->f50);
}
