/* Owner re-derivation from retail assembly (clean input set). */
/* Handler: while D_80114E78 is set, waits for func_8001ABBC on D_800AF218 to accept; then clears the flag and runs func_80011C10 (also run directly when the flag is clear). */
#include "psx_types.h"

extern s32 func_8001ABBC(s32 a0, s32 a1, void *a2, s32 a3, s32 a4);
extern void func_80011C10(void);
extern u8 D_800AF218[];
extern s32 D_80114E78;

void func_800CF47C(void)
{
    if (D_80114E78 != 0) {
        if (func_8001ABBC(0, 0, D_800AF218, 0, 0) == 0) {
            return;
        }
        D_80114E78 = 0;
    }
    func_80011C10();
}
