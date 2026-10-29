/* Owner re-derivation from retail assembly (clean input set). */
/* Draws one menu item of the window record w: when w->f14 is negative it is a text pointer, converted to glyph words and drawn once; otherwise it indexes the sprite-record tables D_801826EC/D_80182774 and each 5-word record (the first word re-packed from its own x/y plus the window origin) is drawn. Then draws each element of the position list at w->f18 (the item chooser func_8017CEB8 supplies the next element, 0 ends the list) and finishes with the frame boxes from w->f4 and w->f8. The highlight value (0xFF when the globally selected D_8011511A differs from sel, else 0) picks the colours. Returns the next free primitive. */
#include "psx_types.h"

typedef struct {
    s16 f0;
    u8 pad2[2];
    u32 f4;
    u32 f8;
    s16 fC;
    s16 fE;
    s16 x;
    s16 y;
    s32 f14;
    u32 f18;
    u32 f1C;
} Win;

extern u8 D_80182774[];
extern u32 D_801826EC[];
extern u16 D_8011511A;

extern s16 func_80024054(s32 str, u32 *out);
extern u32 func_800D2650(u32 p, u32 *glyphs, s32 x, s32 y, s32 pal, u32 color);
extern u32 func_800D27DC(u32 flags, u32 p, u32 *src, s16 n, u16 xoff);
extern u32 func_800D29F8(s16 sel, u32 p, u32 src, s16 n, u16 xoff);
extern s32 func_8017CEB8(u32 tbl, s16 a, s16 b, s16 idx, u32 *out);

u32 func_8017CBC8(u32 p, Win *w, s16 sel)
{
    u32 buf[28];
    u32 *bp;
    u32 pad[4];
    u32 rec;
    s16 *xp;
    s16 *yp;
    s32 hl;
    s32 hl2;
    s16 i;
    s16 k;
    s32 r;

    if (sel == 0) {
        hl = 0;
    } else {
        hl = -((D_8011511A ^ sel) != 0) & 0xFF;
    }
    if (w->f14 < 0) {
        func_80024054(w->f14, buf);
        p = func_800D2650(p, buf, w->x, w->y, 1, hl ? 0x585858 : 0x808080);
    } else {
        rec = D_801826EC[w->f14];
        if (D_80182774[w->f14] != 0) {
            k = 0;
            bp = buf;
            hl2 = hl;
            do {
                i = 0;
                do {
                    if (i == 0) {
                        buf[18] = ((((s16 *)rec)[0] + w->x) & 0xFFFF) | ((((s16 *)rec)[1] + w->y) << 16);
                    } else {
                        bp[18 + i] = *(u32 *)(rec + i * 4);
                    }
                    i++;
                } while (i < 5);
                p = func_800D27DC(hl2 ? 3 : 2, p, &buf[18], 1, 0);
                k++;
                rec += 0x14;
            } while (k < D_80182774[w->f14]);
        }
    }
    i = 0;
    xp = (s16 *)w->f18;
    yp = xp + 1;
    hl2 = hl;
    while (1) {
        r = func_8017CEB8(w->f1C, w->f0, sel, i++, &buf[24]);
        if (r == 0) {
            break;
        }
        if (r < 0) {
            func_80024054(r, buf);
            if (hl2) {
                buf[24] = 0x585858;
            }
            p = func_800D2650(p, buf, *xp, *yp, 1, buf[24]);
        } else {
            rec = D_801826EC[r];
            *(u32 *)rec = (u16)*xp | (*yp << 16);
            p = func_800D27DC(hl2 ? 3 : 2, p, (u32 *)rec, 1, 0);
        }
        yp += 2;
        xp += 2;
    }
    rec = hl;
    p = func_800D27DC(rec != 0, p, (u32 *)w->f4, w->fC, 0);
    return func_800D29F8(rec, p, w->f8, w->fE, 0);
}
