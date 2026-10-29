/* Owner re-derivation from retail assembly (clean input set). */
/* Counts the pool entries (20 x 0x68 at D_8011F9D0) whose id equals `id`; if the count is below `limit`,
 * forwards all arguments to func_801466F0, else returns 0. */
#include "psx_types.h"

typedef struct {
    u16 id;
    u8 pad02[0x66];
} Ent80146608; /* 0x68 bytes */

extern u8 D_8011F9D0[];
/* local prototype: retail returns the callee's v0 here; the owner source for 801466F0 declares void */
extern s32 func_801466F0(u16 id, s32 b, s16 c, s16 d, s16 e, s32 f, s32 g, s32 h);

s32 func_80146608(u16 id, s32 b, u16 c, u16 d, u16 e, s32 f, s32 g, u32 limit, s32 h)
{
    u32 n;
    Ent80146608 *p;

    n = 0;
    for (p = ((Ent80146608 *)&D_8011F9D0); p < &((Ent80146608 *)&D_8011F9D0)[20]; p++) {
        if (p->id == id) {
            n++;
        }
    }
    if (n < limit) {
        return func_801466F0(id, b, c, d, e, f, g, h);
    }
    return 0;
}
