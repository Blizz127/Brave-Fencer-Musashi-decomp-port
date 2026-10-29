/* func_8012F568 - owner re-derivation from retail assembly (clean input set).
 * Stores two short vectors and three halfwords into the block at
 * D_80126B94 and ORs a0 into its flags.
 *
 * One struct object (not separate globals): retail keeps the flags load
 * after both 8-byte copies, which needs the copies and the load to share a
 * base symbol so the scheduler sees a dependence. */
#include "psx_types.h"

typedef struct { s16 v[4]; } V4;
typedef struct {
    u16 flags;      /* 80126B94 */
    s16 a;          /* 80126B96 */
    s16 c;          /* 80126B98 */
    s16 b;          /* 80126B9A */
    u8 pad[0x9C];
    V4 v0;          /* 80126C38 */
    V4 v1;          /* 80126C40 */
} Blk;

extern Blk D_80126B94;

void func_8012F568(s32 a0, s16 a1, s16 a2, s16 a3, V4 *p1, V4 *p2) {
    D_80126B94.v0 = *p1;
    D_80126B94.v1 = *p2;
    D_80126B94.a = a1;
    D_80126B94.b = a2;
    D_80126B94.c = a3;
    D_80126B94.flags |= a0;
}
