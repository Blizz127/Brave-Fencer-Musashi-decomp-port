/* func_8012CFA8 - owner re-derivation from retail assembly (clean input set).
 * Allocates a 0x14-byte primitive, projects o and links it into the current
 * OT at depth n. The link call sits in its own do/while(0) block (as a macro
 * would expand), which keeps the arg copy after the stores. void. */
#include "psx_types.h"
typedef struct {
    u32 ot; /* guest pointer */
    u8 pad[0x14 - 4];
} OtBuffer;
typedef struct { s32 w0; s32 w4; u16 x, y, x1, y1, x2, y2, x3, y3; } Q8012CFA8;
extern u8 D_800AF648;
extern u8 D_800A651C[];
extern u16 D_800B9A02;
extern void *func_80010A08(s32 size);
extern void func_80058D70(void *p);
extern void func_8004914C(void *p);
extern void func_800491AC(void *p);
extern s32 func_8004955C(s32 o, s32 *a, s32 *b, s32 *c);
extern void func_80058C6C(u32 p, void *q);

void func_8012CFA8(s32 o)
{
    Q8012CFA8 *q;
    s32 x;
    s32 y;
    s32 z;
    s32 n;

    q = func_80010A08(0x14);
    q->w4 = 0xFF;
    func_80058D70(q);
    func_8004914C(&D_800AF648);
    func_800491AC(&D_800AF648);
    n = (func_8004955C(o, &x, &y, &z) >> 2) - 0x20;
    if (n > 0) {
        *(s32 *)&q->x = x;
        q->x1 = q->x + 8;
        q->y1 = q->y;
        q->x2 = q->x;
        q->y2 = q->y + 8;
        do {
            func_80058C6C(((OtBuffer *)&D_800A651C)[D_800B9A02].ot + n * 4, q);
        } while (0);
    }
}

