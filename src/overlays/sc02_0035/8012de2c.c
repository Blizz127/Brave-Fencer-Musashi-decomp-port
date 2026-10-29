/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u16 id;
    u8 pad2[0x10C - 2];
} Slot; /* 0x10C */

extern Slot D_801202A0[96];
extern Slot *D_801BD5BC;
extern void *D_801BD5B8;

/* Stateful slot search: resumes from the saved cursor (D_801BD5BC) and
 * returns the next occupied slot that is not the caller's own slot,
 * excluding it via D_801BD5B8. Returns 0 and resets the cursor once the
 * table is exhausted. */
void *func_8012DE2C(void *a0) {
    Slot *base = D_801202A0;
    Slot *end = base + 96;
    Slot *cur;
    Slot *adv;

    D_801BD5BC = base;
    D_801BD5B8 = a0;
    if (base != end) {
        do {
            cur = D_801BD5BC;
            if (cur->id != 0 && cur != a0) {
                D_801BD5BC = cur + 1;
                return cur;
            }
            adv = D_801BD5BC + 1;
            D_801BD5BC = adv;
        } while (adv != end);
    }
    D_801BD5BC = 0;
    return 0;
}
