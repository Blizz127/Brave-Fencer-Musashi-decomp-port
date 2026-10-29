/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u16 used;
    u8 pad2[0x10A];
} Slot8012C284; /* 0x10C bytes */

extern u8 D_801202A0[];

Slot8012C284 *func_8012C284(Slot8012C284 *s) {
    Slot8012C284 *end = ((Slot8012C284 *)&D_801202A0) + 0x60;
    if (s == 0) {
        s = ((Slot8012C284 *)&D_801202A0);
    } else {
        s++;
    }
    for (; s != end; s++) {
        if (s->used == 0) {
            return s;
        }
    }
    return 0;
}
