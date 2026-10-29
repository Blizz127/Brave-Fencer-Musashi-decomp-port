/* Owner re-derivation from retail assembly (clean input set). */
/* Pad-1 debug display shift: save/restore the three display rects, nudge the origin with R2+D-pad, rebuild the display env. */
#include "psx_types.h"

typedef struct {
    s16 x;
    s16 y;
    s16 w;
    s16 h;
} Rect;

/* 0x5C bytes, libgpu DRAWENV layout. */
typedef struct {
    Rect clip;
    s16 ofs[2];
    Rect tw;
    u16 tpage;
    u8 dtd;
    u8 dfe;
    u8 isbg;
    u8 r0;
    u8 g0;
    u8 b0;
    u32 dr_env[16];
} DrawEnv;

/* 0x14 bytes, libgpu DISPENV layout. */
typedef struct {
    Rect disp;
    Rect screen;
    u8 isinter;
    u8 isrgb24;
    u8 pad0;
    u8 pad1;
} DispEnv;

/* Only the fields this function touches are laid out. */
typedef struct {
    u8 pad0[0x38];
    DrawEnv draw[3];  /* 0x38 */
    DispEnv disp[3];  /* 0x14C */
    u16 cur;          /* 0x188: display env index */
    u8 pad18A[0xA3E5 - 0x18A];
    u8 lock;          /* 0xA3E5 (D_800B9A15) */
    u8 padA3E6[2];
    s32 step;         /* 0xA3E8 */
} State;

extern State D_800AF630;
extern s32 D_80075818;    /* display origin x offset */
extern s32 D_8007581C;    /* display origin y offset */
extern Rect D_80075820[3]; /* saved disp rects */
extern s32 func_80014C54(u32, u32, u32);
extern u16 func_80014BFC(u8);
extern u16 func_80014B10(s32);
extern u32 func_80058B04(void *, s32, s32, s32, s32); /* returns its first argument */

void func_80028694(void)
{
    State *p = &D_800AF630;

    if (p->lock != 0) return;
    if ((s16)func_80014C54(1, 0, 2)) {
        p->draw[0].dfe = p->draw[1].dfe = p->draw[2].dfe = 1;
        D_80075818 = D_8007581C = 0;
        D_80075820[0].x = p->disp[0].disp.x;
        D_80075820[0].y = p->disp[0].disp.y;
        D_80075820[0].w = p->disp[0].disp.w;
        D_80075820[0].h = p->disp[0].disp.h;
        D_80075820[1].x = p->disp[1].disp.x;
        D_80075820[1].y = p->disp[1].disp.y;
        D_80075820[1].w = p->disp[1].disp.w;
        D_80075820[1].h = p->disp[1].disp.h;
        D_80075820[2].x = p->disp[2].disp.x;
        D_80075820[2].y = p->disp[2].disp.y;
        D_80075820[2].w = p->disp[2].disp.w;
        D_80075820[2].h = p->disp[2].disp.h;
    } else if ((func_80014BFC(1) & 2) && !(func_80014B10(1) & 2)) {
        p->draw[0].dfe = p->draw[1].dfe = p->draw[2].dfe = 0;
        p->disp[0].disp.x = D_80075820[0].x;
        p->disp[0].disp.y = D_80075820[0].y;
        p->disp[0].disp.w = D_80075820[0].w;
        p->disp[0].disp.h = D_80075820[0].h;
        p->disp[1].disp.x = D_80075820[1].x;
        p->disp[1].disp.y = D_80075820[1].y;
        p->disp[1].disp.w = D_80075820[1].w;
        p->disp[1].disp.h = D_80075820[1].h;
        p->disp[2].disp.x = D_80075820[2].x;
        p->disp[2].disp.y = D_80075820[2].y;
        p->disp[2].disp.w = D_80075820[2].w;
        p->disp[2].disp.h = D_80075820[2].h;
    }
    if ((s16)func_80014C54(1, 1, 2)) {
        switch (func_80014B10(1) & 0x5000) {
        case 0x1000:
            D_8007581C -= p->step * 2;
            break;
        case 0x4000:
            D_8007581C += p->step * 2;
            break;
        }
        switch (func_80014B10(1) & 0xA000) {
        case 0x8000:
            D_80075818 -= p->step * 2;
            break;
        case 0x2000:
            D_80075818 += p->step * 2;
            break;
        }
        if ((s16)func_80014C54(1, 1, 8)) {
            func_80058B04(&p->disp[p->cur], D_80075818, D_8007581C, 320, 240);
        } else {
            func_80058B04(&p->disp[p->cur], D_80075818, D_8007581C, 640, 480);
        }
    }
}
