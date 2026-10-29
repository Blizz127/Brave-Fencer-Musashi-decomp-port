/* Owner re-derivation from retail assembly (clean input set). */
/* Main wait loop: clears the two event words D_80114E98/D_80114E9C, runs func_8001B85C once, then repeats: call the handler
 * D_800D3488[D_800B99F6] while the first word is zero, fetch the second word from func_8001B86C(1) while it is zero (argument 0 here; the same loop as func_800CF104 passes 1),
 * leave through func_80011B7C(7) once both are set; otherwise redraw (func_800596F4, func_8004239C with the word at
 * +0xA3E8 of the state block D_800AF630, func_80059CF4 with the first word of the 0x14-byte record selected by the
 * half-word at +0xA3D2, plus 4, and func_8001AF34). */
#include "psx_types.h"

typedef struct {
    u8 pad0[0xA3D2];
    u16 sel;
    u8 padA3D4[0x14];
    u32 head;
} State;

typedef struct {
    u32 w0;
    u8 pad04[0x10];
} Rec;

extern State D_800AF630;
extern Rec D_800A651C[];
extern s32 D_80114E98;
extern s32 D_80114E9C;
extern u16 D_800B99F6;
extern u32 D_800D3488[];
extern void func_8001B85C(void);
extern s32 func_8001B86C(s32 a);
extern s32 func_80011B7C(s32 k);
extern void func_800596F4(s32 a);
extern void func_8004239C(u32 a);
extern void func_80059CF4(u32 a);
extern void func_8001AF34(void);

/* Returns retail's leftover v0: func_80011B7C(7)'s result (the only exit). */
s32 func_800CEFD0(void)
{
    State *s = &D_800AF630;

    D_80114E98 = 0;
    D_80114E9C = 0;
    func_8001B85C();
again:
    if (D_80114E98 == 0) {
        ((void (*)(void))D_800D3488[D_800B99F6])();
    }
    if (D_80114E9C == 0) {
        D_80114E9C = func_8001B86C(0);
    }
    if (D_80114E98 != 0 && D_80114E9C != 0) {
        return func_80011B7C(7);
    }
    func_800596F4(0);
    func_8004239C(s->head);
    func_80059CF4(D_800A651C[s->sel].w0 + 4);
    func_8001AF34();
    goto again;
}
