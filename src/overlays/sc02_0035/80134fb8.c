/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

typedef struct { s16 x, y, z; } V3;

s32 func_80134FB8(V3 *a, V3 *b, s32 c) {
    return a->x * b->x + a->y * b->y + a->z * b->z + c;
}
