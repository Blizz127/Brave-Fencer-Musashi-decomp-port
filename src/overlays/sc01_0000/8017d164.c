/* Owner re-derivation from retail assembly (clean input set). */
/* Copies two triples of u16 values (from the arrays in a0 and a1) into the six u16 globals D_80182698..D_801826A4; the source reads use raw offsets so that each load stays next to its store. */
#include "psx_types.h"

extern u16 D_80182698;
extern u16 D_8018269A;
extern u16 D_8018269C;
extern u16 D_801826A0;
extern u16 D_801826A2;
extern u16 D_801826A4;

void func_8017D164(u8 *a, u8 *b)
{
    D_80182698 = *(u16 *)(a + 0);
    D_8018269A = *(u16 *)(a + 2);
    D_8018269C = *(u16 *)(a + 4);
    D_801826A0 = *(u16 *)(b + 0);
    D_801826A2 = *(u16 *)(b + 2);
    D_801826A4 = *(u16 *)(b + 4);
}
