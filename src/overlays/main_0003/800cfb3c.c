/* Owner re-derivation from retail assembly (clean input set). */
/* Walks the 16-byte records until the type 0xFF terminator. Each type-9 record fills w half-words at (data + 2*w)
 * with 0x7FFF and submits the rectangle (x, y, w, h) with that buffer through func_800599B8. D_800EC69C is cleared first. */
#include "psx_types.h"

typedef struct {
    u16 type;
    u8 pad2[2];
    u16 x;
    u16 y;
    s16 w;
    u16 h;
    u32 data;
} Rec;

typedef struct {
    s16 x, y, w, h;
} Rect;

extern s32 D_800EC69C;
extern void func_800599B8(Rect *r, u32 p);

void func_800CFB3C(Rec *rec)
{
    Rect r;
    s32 i;
    u16 *q;
    u32 buf;

    D_800EC69C = 0;
    for (; rec->type != 0xFF; rec++) {
        if (rec->type == 9) {
            q = (u16 *)(rec->data + rec->w * 2);
            for (i = 0; i < rec->w; i++) {
                *q++ = 0x7FFF;
            }
            buf = rec->data + rec->w * 2;
            r.x = rec->x;
            r.y = rec->y;
            r.w = rec->w;
            r.h = rec->h;
            func_800599B8(&r, buf);
        }
    }
}
