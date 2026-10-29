/* Owner re-derivation from retail assembly (clean input set). */
/* Copies D_80127508's three u16 fields (+0x6/+0xA/+0xE) into a local buffer, calls
 * func_80171AB0(o, buf); unless it returns nonzero, runs func_80171A1C(o); func_80174650(o)
 * [callee ignores o]. void abi: sampled caller's next instr is a plain epilogue restore. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x6];
    u16 f6;  /* 0x6 */
    u8 pad8[0xA - 0x8];
    u16 fA;  /* 0xA */
    u8 padC[0xE - 0xC];
    u16 fE;  /* 0xE */
} Rec;

extern Rec *D_80127508;
extern s16 func_80171AB0(void *o, void *a1);
extern void func_80171A1C(void *o);
extern void func_80174650();

void func_80172E20(void *o)
{
    Rec *p = D_80127508;
    u16 buf[3];

    buf[0] = p->f6;
    buf[1] = p->fA;
    buf[2] = p->fE;

    if (func_80171AB0(o, buf) == 0) {
        func_80171A1C(o);
        func_80174650(o);
    }
}
