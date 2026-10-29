/* Owner re-derivation from retail assembly (clean input set). */
/* Flush queued VRAM uploads: for each active upload list, send images/CLUTs/TIMs/texture pages to VRAM. */
#include "psx_types.h"

typedef struct {
    s16 x;
    s16 y;
    s16 w;
    s16 h;
} VramRect;

/* Image description filled by func_80053D44; the pixel/CLUT pointers are guest addresses. */
typedef struct {
    u32 pmode;
    s16 px;
    s16 py;
    u16 pw;
    u16 ph;
    u32 pixel;
    s16 cx;
    s16 cy;
    u16 cw;
    u16 ch;
    u32 clut;
} TimInfo;

typedef struct {
    u16 type;
    u16 pad2;
    s16 x;
    s16 y;
    s16 w;
    s16 h;
    u32 data;
} UploadEntry;

typedef struct {
    s16 active;
    u16 pad2;
    u32 list;
    u16 x;
    u16 y;
    u8 fixedY;
    u8 useSlot;
    u16 padE;
} UploadQueue;

extern UploadQueue D_800B97D8[];

void func_800599B8(VramRect *rect, u32 *pixels);
void func_80053D44(u32 *tim, TimInfo *info);
u16 func_80058890(u32 *pixels, s32 mode, s32 abr, s32 x, s32 y, s32 w, s32 h);

void func_800184F0(void)
{
    s32 i;
    s32 done;
    UploadQueue *q;
    UploadEntry *e;
    VramRect *r;
    TimInfo info;

    r = (VramRect *)0x1F800000;
    for (i = 0, q = D_800B97D8; i < 32; i++, q++) {
        if (q->active == 0) {
            continue;
        }
        e = (UploadEntry *)q->list;
        done = 0;
        do {
            switch (e->type) {
            case 9:
                if (q->useSlot == 0) {
                    r->x = e->x;
                    r->y = e->y;
                } else {
                    r->x = (q->fixedY > 0x20) << 8;
                    r->y = q->fixedY | 0x1E0;
                }
                r->w = e->w;
                r->h = e->h;
                func_800599B8(r, (u32 *)e->data);
                break;
            case 0:
                if (q->useSlot == 0) {
                    r->x = e->x;
                    r->y = e->y;
                } else {
                    r->x = q->x;
                    r->y = q->y;
                }
                r->w = e->w;
                r->h = e->h;
                func_800599B8(r, (u32 *)e->data);
                break;
            case 5:
                func_80053D44((u32 *)e->data, &info);
                r->x = info.px;
                r->y = info.py;
                r->w = info.pw;
                r->h = info.ph;
                func_800599B8(r, (u32 *)info.pixel);
                break;
            case 11:
                func_80058890((u32 *)e->data, 1, 0, e->x, e->y, e->w * 2, e->h);
                break;
            case 10:
                break;
            case 0xFF:
                done = 1;
                break;
            default:
                done = 1;
                break;
            }
            e++;
        } while (!done);
        q->active = 0;
    }
}
