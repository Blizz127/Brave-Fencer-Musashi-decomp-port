/* func_8012A1BC [8012A1BC,8012A2F4) - owner re-derivation from retail assembly (clean input set).
 * Snapshots the 0x2C-byte camera tuning block at D_80120200 (see func_8012A110) into the working copy
 * at D_80126950, member by member in layout order (the two V4 members are 8-byte struct copies), then
 * the three words at D_80120294.. into D_801269E4... */
#include "psx_types.h"

typedef struct { u16 a, b, c, d; } V4;
typedef struct {
    s32 a, b, c, d;
    V4 e;
    V4 f;
    u16 g[4];
    u16 h, i;
} G; /* 0x2C */

/* callers read v0: none-direct (jal; nop); void.
 * Owner sources declare D_80120200 and D_80126950 as s32 scalars, so the block is viewed through a
 * G cast at each use; both struct copies and member stores share one base symbol per side. */
extern s32 D_80120200;
extern s32 D_80126950;
extern s32 D_80120294;
extern u16 D_80120298;
extern u16 D_8012029A;
extern s32 D_801269E4;
extern s16 D_801269E8;
extern s16 D_801269EA;

void func_8012A1BC(void) {
    ((G *)&D_80126950)->a = ((G *)&D_80120200)->a;
    ((G *)&D_80126950)->b = ((G *)&D_80120200)->b;
    ((G *)&D_80126950)->c = ((G *)&D_80120200)->c;
    ((G *)&D_80126950)->d = ((G *)&D_80120200)->d;
    ((G *)&D_80126950)->e = ((G *)&D_80120200)->e;
    ((G *)&D_80126950)->f = ((G *)&D_80120200)->f;
    ((G *)&D_80126950)->g[0] = ((G *)&D_80120200)->g[0];
    ((G *)&D_80126950)->g[1] = ((G *)&D_80120200)->g[1];
    ((G *)&D_80126950)->g[2] = ((G *)&D_80120200)->g[2];
    ((G *)&D_80126950)->g[3] = ((G *)&D_80120200)->g[3];
    ((G *)&D_80126950)->h = ((G *)&D_80120200)->h;
    ((G *)&D_80126950)->i = ((G *)&D_80120200)->i;
    D_801269E4 = D_80120294;
    D_801269E8 = (*(s16 *)&D_80120298);
    D_801269EA = (*(s16 *)&D_8012029A);
}
