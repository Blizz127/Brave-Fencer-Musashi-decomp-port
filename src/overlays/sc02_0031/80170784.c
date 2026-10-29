/* Owner re-derivation from retail assembly (clean input set). */
/* func_801719A4(o, 1); dispatches through D_8011F738 (no args set, so it is treated here as a
 * void(void) handler); func_80171990(o); func_801707D4(o). */
#include "psx_types.h"

extern void func_801719A4(void *o, s32 a);
extern void (*D_8011F738)(void);
extern void func_80171990(void *o);
extern void func_801707D4(u8 *o);

void func_80170784(u8 *o)
{
    func_801719A4(o, 1);
    D_8011F738();
    func_80171990(o);
    func_801707D4(o);
}
