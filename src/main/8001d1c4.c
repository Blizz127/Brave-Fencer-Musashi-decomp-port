/* Owner re-derivation from retail assembly (clean input set). */
/* Runs the per-object handler pass over the six object pools, skipping pools whose bit is set in the pause mask; the handlers share one counter. */
#include "psx_types.h"

typedef struct {
    u8 data[0x38];
} Obj;

/* Object pools at 0x800B5BD8 up to the mode/pause mask byte at 0x800B9A11. */
typedef struct {
    Obj grp0[4];     /* +0x0000 */
    Obj grp1[2];     /* +0x00E0 */
    Obj grp2[16];    /* +0x0150 */
    Obj grp3[40];    /* +0x04D0 */
    Obj grp4[64];    /* +0x0D90 */
    Obj grp5[130];   /* +0x1B90 */
    u8 pad[0x3E39 - 0x3800];
    u8 skip;         /* +0x3E39 = 0x800B9A11 */
} ObjPools;

extern ObjPools D_800B5BD8;

/* The second argument is passed through to the subtype handler. */
extern void func_8001D388(Obj *obj, s32 *count);

void func_8001D1C4(void)
{
    s32 count;
    u8 skip;
    Obj *o;

    count = 0;
    skip = D_800B5BD8.skip;
    for (o = D_800B5BD8.grp0; o < &D_800B5BD8.grp0[4]; o++) {
        func_8001D388(o, &count);
    }
    if (!(skip & 0x04)) {
        for (o = D_800B5BD8.grp1; o < &D_800B5BD8.grp1[2]; o++) {
            func_8001D388(o, &count);
        }
    }
    if (!(skip & 0x10)) {
        for (o = D_800B5BD8.grp2; o < &D_800B5BD8.grp2[16]; o++) {
            func_8001D388(o, &count);
        }
    }
    if (!(skip & 0x20)) {
        for (o = D_800B5BD8.grp3; o < &D_800B5BD8.grp3[40]; o++) {
            func_8001D388(o, &count);
        }
    }
    if (!(skip & 0x40)) {
        for (o = D_800B5BD8.grp4; o < &D_800B5BD8.grp4[64]; o++) {
            func_8001D388(o, &count);
        }
    }
    if (!(skip & 0x80)) {
        for (o = D_800B5BD8.grp5; o < &D_800B5BD8.grp5[130]; o++) {
            func_8001D388(o, &count);
        }
    }
}
