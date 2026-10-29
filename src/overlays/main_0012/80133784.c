/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"
/* Sweeps a movement segment from point a to point b: seeds the two working points, picks a
 * start height from the segment length (mode 0) or a fixed rise (mode 2), then probes the
 * grid until the sweep stops hitting. Returns flag bits describing which ends are clipped. */

/* Working point record: three coordinates and a height. */
typedef struct {
    u16 x;
    u16 y;
    u16 z;
    s16 h;
} Pt;

/* Pointer-valued globals are guest addresses (u32). */
extern u32 D_8017E910;
extern u32 D_8017E914;
extern u32 D_80184120;
extern s16 D_8018412C;
extern u16 D_80184130;

extern int func_80047D3C(int sq);
extern int func_80133AB0(s16 mode, s16 x, s16 z, u32 grid);

u16 func_80133784(s16 mode, u16 *a, u16 *b)
{
    Pt *q;
    s32 flags;
    s16 done;
    s16 cnt;
    s16 acc;
    s32 r;
    s16 dx;
    s16 dy;
    s16 dz;

    flags = 0;
    cnt = 0;
    acc = 0;
    ((Pt *)D_8017E910)->h = -0x7FFF;
    ((Pt *)D_8017E914)->h = 0x7FFF;
    ((Pt *)D_8017E910)->x = a[0];
    ((Pt *)D_8017E910)->z = a[2];
    done = 0;
    ((Pt *)D_8017E914)->x = b[0];
    D_80184130 = 0;
    D_8018412C = 0;
    ((Pt *)D_8017E914)->z = b[2];
    if (mode == 0) {
        dx = b[0] - a[0];
        dy = b[1] - a[1];
        dz = b[2] - a[2];
        if (dx == 0 && dy == 0) {
            done = dz == 0;
        }
        ((Pt *)D_8017E910)->y = a[1] - 4;
        r = func_80047D3C(dx * dx + dz * dz);
        if (r < 3) {
            r = 4;
        } else if (r < 5) {
            r++;
        }
        ((Pt *)D_8017E914)->y = b[1] + r + 1;
    } else {
        ((Pt *)D_8017E910)->y = a[1];
        if (mode == 2) {
            ((Pt *)D_8017E914)->x = ((Pt *)D_8017E910)->x;
            ((Pt *)D_8017E914)->y = ((Pt *)D_8017E910)->y + 6;
            ((Pt *)D_8017E914)->z = ((Pt *)D_8017E910)->z;
            done = 1;
        } else {
            ((Pt *)D_8017E914)->y = b[1];
        }
    }
    for (;;) {
        r = func_80133AB0(mode, ((Pt *)D_8017E910)->x, ((Pt *)D_8017E910)->z, D_80184120);
        if (r == 0) {
            break;
        }
        acc |= r;
        if (done != 0) {
            break;
        }
        if (cnt++ < 5) {
            continue;
        }
        ((Pt *)D_8017E914)->x = ((Pt *)D_8017E910)->x;
        ((Pt *)D_8017E914)->y = ((Pt *)D_8017E910)->y;
        ((Pt *)D_8017E914)->z = ((Pt *)D_8017E910)->z;
        flags = 0x2000;
        goto finish;
    }
    if (acc != 0 || D_8018412C != 0) {
        if (((Pt *)D_8017E910)->h >= -0xBCB) {
            if (((Pt *)D_8017E910)->h < -0x578) {
                flags |= 0x4000;
            } else {
                flags |= 0x8000;
            }
        }
        if (((Pt *)D_8017E914)->h < -0xBCB) {
            flags |= 0x2000;
        }
finish:
        q = (Pt *)D_8017E914;
        b[0] = q->x;
        b[1] = q->y;
        b[2] = q->z;
        b[3] = D_80184130;
        return flags;
    }
    b[3] = D_80184130;
    return 0;
}
