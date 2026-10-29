/* Owner re-derivation from retail assembly (clean input set). */
/* Fetches the next decoded frame: polls func_80046C38(&handle, &frame) up to 0x8000 times and returns 0 if it never
 * succeeds. Otherwise flags the frame's counter word (+8) against the header's limit (+0xC) in D_800EC9F4/D_800EC9F8
 * and, when the frame size (+0x10/+0x12) differs from the remembered D_800D9484/D_800D9488, clears a rectangle of the
 * display (black when the header flag +4 is set, grey otherwise) and remembers the new size. Stores the (possibly
 * 3/2-scaled) width and the height into the output record (+0x1C/+0x24 and +0x1E/+0x26/+0x32) and returns the handle.
 * The clear is ONE call site (retail has a single jal for it): both arms of the black/grey choice set the rectangle
 * pointer and the three colour values, and the call follows the join. The pointer is deliberately set in both arms:
 * with &rect evaluated at the call instead, gcc puts the address setup in the jal delay slot (one word short). */
#include "psx_types.h"

typedef struct {
    s16 x, y, w, h;
} Rect;

typedef struct {
    u8 pad0[8];
    u32 w8;
    u8 padC[4];
    u16 h10;
    u16 h12;
} Frame;

typedef struct {
    u8 pad0[4];
    s32 w04;
    u8 pad08[4];
    u32 w0C;
    u8 pad10[8];
    s32 w18;
    s32 w1C;
} Hdr;

typedef struct {
    u8 pad0[0x1C];
    u16 h1C;
    u16 h1E;
    u8 pad20[4];
    u16 h24;
    u16 h26;
    u8 pad28[10];
    u16 h32;
} Out;

extern s32 D_800EC9F4;
extern s32 D_800EC9F8;
extern s32 D_800D9484;
extern s32 D_800D9488;
extern s32 func_80046C38(u32 *handle, u32 *frame);
extern void func_80059888(Rect *r, s32 a, s32 b, s32 c);

u32 func_800D27A0(Out *o, Hdr *h)
{
    u32 p0;
    u32 p1;
    s32 n;
    u16 x;
    u16 y;

    for (n = 0x8000; n != 0; n--) {
        if (func_80046C38(&p0, &p1) == 0) {
            goto got;
        }
    }
    return 0;
got:
    if (((Frame *)p1)->w8 >= h->w0C) {
        D_800EC9F4 = 1;
    }
    if (((Frame *)p1)->w8 >= h->w0C - 15) {
        D_800EC9F8 = 1;
    } else {
        D_800EC9F8 = 0;
    }
    if ((u32)D_800D9484 != ((Frame *)p1)->h10 || (u32)D_800D9488 != ((Frame *)p1)->h12) {
        Rect rect;
        Rect *area;
        s32 red;
        s32 green;
        s32 blue;
        u16 w;

        rect.x = 0;
        rect.y = 0;
        if (h->w04 != 0) {
            w = (h->w18 * 3) / 2;
        } else {
            w = h->w18;
        }
        rect.w = w;
        rect.h = h->w1C << 1;
        if (h->w04 != 0) {
            area = &rect;
            red = 0;
            green = 0;
            blue = 0;
        } else {
            area = &rect;
            red = 0x40;
            green = 0x40;
            blue = 0x40;
        }
        func_80059888(area, red, green, blue);
        D_800D9484 = ((Frame *)p1)->h10;
        D_800D9488 = ((Frame *)p1)->h12;
    }
    if (h->w04 != 0) {
        x = (D_800D9484 * 3) / 2;
    } else {
        x = D_800D9484;
    }
    y = D_800D9488;
    o->h24 = x;
    o->h1C = x;
    o->h26 = y;
    o->h1E = y;
    o->h32 = y;
    return p0;
}
