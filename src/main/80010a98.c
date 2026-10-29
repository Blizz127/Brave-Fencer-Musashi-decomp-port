/* Owner re-derivation from retail assembly (clean input set). */
/* Registers func_8001099C with func_800595FC and func_8001096C with func_80042610. */
#include "psx_types.h"

extern void func_8001099C(void);
extern void func_8001096C(void);
extern void func_800595FC(void (*fn)(void));
extern void func_80042610(void (*fn)(void));

void func_80010A98(void) {
    func_800595FC(func_8001099C);
    func_80042610(func_8001096C);
}
