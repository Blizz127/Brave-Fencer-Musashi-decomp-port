/* Owner re-derivation from retail assembly (clean input set). */
/* Picks D_8011F73C or D_8011F740 depending on func_80029178(D_8011F738)&0xFF, then runs
 * func_801718AC(picked). void abi: sampled caller's next instr is a plain epilogue restore. */
#include "psx_types.h"

extern u32 *D_8011F738;
extern u32 *D_8011F73C;
extern u32 *D_8011F740;
extern s32 func_80029178(void *a);
extern void func_801718AC(u32 *p);

void func_80170DE0(void)
{
    s32 flag = func_80029178(D_8011F738) & 0xFF;
    u32 *val;

    if (flag != 0) {
        val = D_8011F740;
    } else {
        val = D_8011F73C;
    }
    func_801718AC(val);
}
