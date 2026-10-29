/* Owner re-derivation from retail assembly (clean input set). */
/* Handler: starts func_80011A3C when func_80019A24 accepts the D_800AE890 entry with D_80072C74. */
#include "psx_types.h"

extern s32 func_80019A24(void *loc, s32 dest, s32 size, s32 wait);
extern void func_80011A3C(void);
extern u8 D_800AE890[];
extern s32 D_80072C74;

void func_800CF94C(void)
{
    if (func_80019A24(D_800AE890, D_80072C74, 0, 0) != 0) {
        func_80011A3C();
    }
}
