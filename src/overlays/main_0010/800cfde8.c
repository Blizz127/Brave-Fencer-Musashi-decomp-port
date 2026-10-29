/* Owner re-derivation from retail assembly (clean input set). */
/* Starts the record in the D_800AE830 slot named by the 12-bit field of mode id's entry in D_800D34AC (func_8001ABBC with kind 2) when that slot is in use. */
#include "psx_types.h"

typedef struct {
    s32 w0;
    s32 w1;
} Slot800AE830;

extern s32 func_8001ABBC(s32 a0, s32 a1, void *a2, s32 a3, s32 a4);
extern u32 D_800D34AC[];
extern Slot800AE830 D_800AE830[];

void func_800CFDE8(s32 id)
{
    s32 idx;

    idx = D_800D34AC[id & 0xFFFF0FFF] & 0xFFF;
    if (idx >= 0 && D_800AE830[idx].w0 != 0) {
        func_8001ABBC(2, 0, &D_800AE830[idx], 0, 0);
    }
}
