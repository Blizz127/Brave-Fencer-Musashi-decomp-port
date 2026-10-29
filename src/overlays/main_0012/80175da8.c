/* Owner re-derivation from retail assembly (clean input set). */
/* Builds and links the draw primitives of HUD slot `idx`: five 0x14-byte primitives copied from the
 * template D_80181A40 and offset by the slot's three position pairs, the gauge pair from
 * func_8017742C and a closing link, then refreshes the slot object's flag/colour bytes and queues the
 * panel through func_800183E0. The slot's primitive count in D_800AF634 grows by the words written. */
#include "psx_types.h"

/* Displayed stats copy (HUD block + 0x48); only the fields used here. */
typedef struct {
    u8 pad00[0x1E];
    s16 f1E;
    u8 pad20[0x2E - 0x20];
    u16 f2E;
    u8 pad30[0x40 - 0x30];
    u16 f40;
    u8 pad42[0x48 - 0x42];
    u8 f48;
} Stats;

/* HUD block: position pairs at 0xC/0x10/0x14, per-slot guest addresses at 0x28 (chain start) and
 * 0x30 (chain end). */
typedef struct {
    u8 pad00[7];
    u8 f7;
    u8 pad08[4];
    u16 x0;
    u16 y0;
    u16 x1;
    u16 y1;
    u16 x2;
    u16 y2;
    u8 pad18[0x28 - 0x18];
    u32 c[2];
    u32 d[2];
} Hud;

/* Primitive of 0x14 bytes. */
typedef struct {
    u32 tag;
    u32 w1;
    s16 x;
    s16 y;
    u32 w3;
    u32 w4;
} Poly;

/* Per-slot primitive buffer descriptor (12 bytes). */
typedef struct {
    s32 n;
    u32 base;
    s32 pad;
} Buf;

extern Stats D_8011F7F0;
extern Buf D_800AF634[];
extern u8 D_80181A40[];
extern u16 D_80181A9C;
extern u16 D_80181A90[];
extern u32 D_80181B48[];
extern u8 D_80181B30[];
extern u8 D_80181B1C[];
extern u8 D_800D45D4[];
extern u8 D_800D43D4[];
extern u8 D_800D4414[];
extern u8 D_800B9A13;

extern Poly *func_8017742C(Poly *p, s16 x, s16 y);
extern void func_8005A600(Poly *p, s32 a, s32 b, s32 c, s32 d);
extern s32 func_800183E0(u8 *cmd);

void func_80175DA8(s16 idx)
{
    Stats *c = &D_8011F7F0;
    Hud *H = (Hud *)((u8 *)c - 0x48);
    Poly *p;
    u8 *t = D_80181A40;
    s16 j = 0;
    u32 slot;
    u32 slot2;
    u32 slot3;
    u8 *cmd;
    u16 *e;

    p = (Poly *)(D_800AF634[idx].base + D_800AF634[idx].n * 4);
    H->c[idx] = (u32)p;
    do {
        p->tag = (((u32)p - 0x14) & 0xFFFFFF) | 0x04000000;
        p->w1 = *(u32 *)t;
        t += 4;
        if (j < 2) {
            p->x = *(u16 *)t + H->x0;
            t += 2;
            p->y = *(u16 *)t + H->y0;
            t += 2;
        } else if (j == 2) {
            p->x = *(u16 *)t + H->x1;
            t += 2;
            p->y = *(u16 *)t + H->y1;
            t += 2;
        } else {
            p->x = *(u16 *)t + H->x2;
            t += 2;
            p->y = *(u16 *)t + H->y2;
            t += 2;
        }
        p->w3 = *(u32 *)t;
        t += 4;
        p->w4 = *(u32 *)t;
        t += 4;
        p++;
    } while (++j < 5);
    H->f7 = D_800B9A13;
    p = func_8017742C(p, H->x1 - 0x98, H->y1 + 9);
    func_8005A600(p, 0, 0, 0x16, 0);
    p->tag = (((u32)p - 0x14) & 0xFFFFFF) | 0x02000000;
    H->d[idx] = (u32)p;
    p++;
    D_800AF634[idx].n += ((s32)((u32)p - H->c[idx])) >> 2;
    p = (Poly *)H->c[idx];
    if (c->f48 != 0) {
        *((u8 *)p + 0xD) = 0;
    } else {
        *((u8 *)p + 0xD) = 0xA0;
    }
    slot = H->c[idx];
    *(u16 *)(slot + 0x22) = 0x6CD6;
    if (c->f48 & 0x80) {
        *(u16 *)(slot + 0x20) = D_80181A9C;
        cmd = D_800D45D4;
    } else {
        e = D_80181A90;
        if (c->f48 != 0) {
            e = (u16 *)((u8 *)D_80181A90 + (c->f48 - 1) * 2);
        }
        *(u16 *)(slot + 0x20) = *e;
        cmd = (u8 *)D_80181B48[c->f48];
    }
    func_800183E0(cmd);
    slot2 = H->c[idx];
    if ((s16)c->f2E != 0) {
        *(u8 *)(slot2 + 0x49) = D_80181B30[(s16)c->f2E >> 4];
    } else {
        *(u8 *)(slot2 + 0x49) = 0xA0;
    }
    slot3 = H->c[idx];
    if (c->f40 < 100) {
        *(u8 *)(slot3 + 0x5D) = D_80181B1C[(u16)(c->f40 / 5)];
    } else {
        *(u8 *)(slot3 + 0x5D) = 0;
    }
    if (c->f1E & 0x8000) {
        func_800183E0(D_800D4414);
    } else {
        func_800183E0(D_800D43D4);
    }
}
