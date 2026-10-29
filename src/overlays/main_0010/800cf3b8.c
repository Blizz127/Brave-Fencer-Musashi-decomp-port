/* Owner re-derivation from retail assembly (clean input set). */
/* Wait loop: clears the event words D_80114E98/D_80114E9C, runs func_8001B85C once, then repeats until D_80114E98 is
 * set: call the handler D_800D3488[D_800B99F6], leave if the word is now set, otherwise run func_8004239C with the
 * word at +0xA3E8 of the state block D_800AF630 and func_8001AF34. On exit runs func_80015310 and func_80011B7C(7). */
#include "psx_types.h"

typedef struct {
    u8 pad0[0xA3E8];
    u32 head;
} State;

extern State D_800AF630;
extern s32 D_80114E98;
extern s32 D_80114E9C;
extern u16 D_800B99F6;
extern u32 D_800D3488[];
extern void func_8001B85C(void);
extern void func_8004239C(u32 a);
extern void func_8001AF34(void);
extern void func_80015310(void);
extern s32 func_80011B7C(s32 k);

/* Returns retail's leftover v0: func_80011B7C(7)'s result. */
s32 func_800CF3B8(void)
{
    State *s = &D_800AF630;

    D_80114E98 = 0;
    D_80114E9C = 0;
    func_8001B85C();
again:
    if (D_80114E98 != 0) {
        goto out;
    }
    ((void (*)(void))D_800D3488[D_800B99F6])();
    if (D_80114E98 != 0) {
        goto out;
    }
    func_8004239C(s->head);
    func_8001AF34();
    goto again;
out:
    func_80015310();
    return func_80011B7C(7);
}
