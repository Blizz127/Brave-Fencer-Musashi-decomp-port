/* Owner re-derivation from retail assembly (clean input set). */
/* Scene start: runs func_800D1E58 when D_800EC684 is set, then func_800CFFEC, clears D_800B9A18 and D_800EC6A0, runs
 * func_800CFB3C on the record D_800DA48C, sets D_800EC6A4 and runs func_800118AC. */
#include "psx_types.h"

extern s32 D_800EC684;
extern s32 D_800B9A18;
extern s32 D_800EC6A0;
extern s32 D_800EC6A4;
extern u16 D_800DA48C[];
extern void func_800D1E58(void);
extern void func_800CFFEC(void);
extern void func_800CFB3C(u16 *p);
extern void func_800118AC(void);

void func_800CF010(void)
{
    if (D_800EC684 != 0) {
        func_800D1E58();
    }
    func_800CFFEC();
    D_800B9A18 = 0;
    D_800EC6A0 = 0;
    func_800CFB3C(D_800DA48C);
    D_800EC6A4 = 1;
    func_800118AC();
}
