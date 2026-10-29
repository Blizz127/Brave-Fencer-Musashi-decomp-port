/* Owner re-derivation from retail assembly (clean input set). */
/* Returns the halfword at offset 0xAC of the object. */
#include "psx_types.h"

u16 func_80148800(u8 *obj) {
    return *(u16 *)(obj + 0xAC);
}
