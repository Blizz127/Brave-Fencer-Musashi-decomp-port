/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern int func_8012CC88(void *obj, s32 mode, void *tbl);

int func_8012CC64(void *obj, void *tbl) {
    return func_8012CC88(obj, 0x11, tbl);
}
