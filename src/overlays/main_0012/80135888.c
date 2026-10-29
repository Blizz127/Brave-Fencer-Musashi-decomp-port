/* Owner re-derivation from retail assembly (clean input set). */
/* Classifies a move with func_80135480, picks the shape list and node for that class, probes the shapes (list walk or terrain probe) and on a hit resolves the move with func_80136A94; returns 1 on a hit. */
#include "psx_types.h"

/* 3-short vector; D_8017E910/914/91C hold guest addresses of these. */
typedef struct {
    s16 vx;
    s16 vy;
    s16 vz;
} Vec3;

extern u32 D_8017E910;
extern u32 D_8017E914;
extern u32 D_8017E91C;
extern u8 D_80184134[];
extern u8 D_80184154[];
extern int func_80135480(u32 obj, s32 flags, u32 from, u32 to);
extern int func_80135EB0(u32 shape, int arg);
extern int func_80134A74(int arg, s16 x, s16 z, u32 shape);
extern void func_80136A94(int mode, u32 obj, u32 to, u32 node);

int func_80135888(u32 obj, s32 flags, u32 from, u32 to)
{
    u32 shape;
    int mode;
    u32 node;
    s32 pad[8]; /* retail frame reserves 32 bytes of locals that no path touches */

    switch (func_80135480(obj, flags, from, to)) {
    case 0:
        return 0;
    case 1:
        node = obj + 0x34;
        shape = (flags & 0x0FFFFFFF) | 0x80000000;
        mode = 0;
        break;
    case 2:
        node = obj + 0x34;
        shape = (flags & 0x0FFFFFFF) | 0x80000000;
        mode = 1;
        break;
    case 3:
        node = obj + 0x34;
        shape = (u32)D_80184154;
        mode = 0;
        break;
    case 4:
        node = (u32)D_80184134;
        shape = (u32)D_80184154;
        mode = 1;
        break;
    }
    if (flags < 0) {
        /* Shape list: first word of each record links to the next one. */
        if (func_80135EB0(shape, 0) != 0) goto hit;
        shape = *(u32 *)shape;
        if (shape == 0) return 0;
        do {
            if (func_80135EB0(shape, 0) != 0) {
            hit:
                func_80136A94(mode, obj, to, node);
                return 1;
            }
            shape = *(u32 *)shape;
        } while (shape != 0);
        return 0;
    }
    ((Vec3 *)D_8017E91C)->vx = ((Vec3 *)D_8017E914)->vx - ((Vec3 *)D_8017E910)->vx;
    ((Vec3 *)D_8017E91C)->vy = ((Vec3 *)D_8017E914)->vy - ((Vec3 *)D_8017E910)->vy;
    ((Vec3 *)D_8017E91C)->vz = ((Vec3 *)D_8017E914)->vz - ((Vec3 *)D_8017E910)->vz;
    if (func_80134A74(0, ((Vec3 *)D_8017E910)->vx, ((Vec3 *)D_8017E910)->vz, shape) != 0) goto hit;
    return 0;
}
