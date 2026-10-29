/* Owner re-derivation from retail assembly (clean input set). */
/* Dispatches D_80186F7C[o->f2](o) (discarding its result), decrements the global D_801B6438
 * counter, and once it reaches -1 runs func_801599A4(o); func_80159B3C(o). void abi: sampled
 * caller's next instr is a plain epilogue restore. */
#include "psx_types.h"

typedef struct {
    u8 pad0[2];
    u16 f2; /* 0x2 */
} Obj;

typedef void (*Handler)(Obj *);

extern u32 D_80186F7C[];
extern s32 D_801B6438;
extern void func_801599A4(void *o);
extern void func_80159B3C(void *o);

void func_80157808(Obj *o)
{
    ((Handler)D_80186F7C[o->f2])(o);

    D_801B6438 = D_801B6438 - 1;
    if (D_801B6438 == -1) {
        func_801599A4(o);
        func_80159B3C(o);
    }
}
