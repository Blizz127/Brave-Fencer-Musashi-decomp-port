/* Owner re-derivation from retail assembly (clean input set). */
/* Copies D_801151D4's three s32 fields (+0x3C/+0x40/+0x44) truncated into a local s16[3], calls
 * func_80171AB0(o, buf); unless it returns nonzero, runs func_80171928(o). */
#include "psx_types.h"

typedef struct {
    u8 pad[0x3C];
    s32 x; /* 0x3C */
    s32 y; /* 0x40 */
    s32 z; /* 0x44 */
} PtrObj;

extern PtrObj *D_801151D4;
extern s16 func_80171AB0(void *o, void *a1);
extern void func_80171928(void *o);

void func_8017044C(void *o)
{
    PtrObj *p = D_801151D4;
    s16 buf[3];

    buf[0] = p->x;
    buf[1] = p->y;
    buf[2] = p->z;

    if (func_80171AB0(o, buf) == 0) {
        func_80171928(o);
    }
}
