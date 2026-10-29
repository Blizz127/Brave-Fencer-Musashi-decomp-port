/* func_80163534 [80163534,80163664) - owner re-derivation from retail assembly (clean input set).
 * Clears three result words, builds the two offset vectors D_801270B0 / D_80126B18 from o and the
 * u16 triple p, then calls func_80163664 with a long list of table pointers and stores its result. */
#include "psx_types.h"

typedef struct {
    u8 p0[6];
    u16 f6;
    u8 p8[2];
    u16 fA;
    u8 pC[2];
    u16 fE;
    u8 p10[0x34];
    u16 f44;
    u16 f46;
    u16 f48;
} O;

/* callers read v0: yes (callers tail-return it); returns func_80163664's result (13 arguments).
 * a1/a2 are u16 parameters (retail masks them at entry); the fifth parameter is a u16 on the stack
 * and the sixth the u16 triple pointer. */
extern s32 func_80163664(O *o, u32 a1, u32 a2, void *a3, void *a4, void *a5, void *a6, void *a7, void *a8,
                         s32 a9, u32 a10, void *a11, void *a12);
extern u32 D_80115200[3];
extern u16 D_801270B0[3];
extern u8 D_80126B18[];
extern u8 D_80114EB0[];
extern u8 D_80114EC8[];
extern u8 D_80115100[];
extern u8 D_8011DAF0[];
extern u8 D_80115298[];
extern u8 D_80126734[];

s32 func_80163534(O *o, u16 a1, u16 a2, s32 a3, u16 e, u16 *p) {
    s32 r;

    D_80115200[0] = 0;
    D_80115200[1] = 0;
    D_80115200[2] = 0;
    D_801270B0[0] = o->f44 + p[0];
    D_801270B0[1] = o->f46 + p[1];
    D_801270B0[2] = o->f48 + p[2];
    ((u16 *)&D_80126B18)[0] = o->f6 + p[0];
    ((u16 *)&D_80126B18)[1] = o->fA + p[1];
    ((u16 *)&D_80126B18)[2] = o->fE + p[2];
    r = func_80163664(o, a1, a2, D_801270B0, ((u16 *)&D_80126B18), D_80114EB0, D_80114EC8, D_80115100, D_8011DAF0, a3, e,
                      D_80115298, D_80126734);
    D_80115200[0] = r;
    return r;
}
