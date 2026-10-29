/* Owner re-derivation from retail assembly (clean input set). */
/* Grid probe: stores the move delta (D_8017E914 - D_8017E910) into D_8017E91C, maps
 * (x, z) to a grid cell, then tests every shape record listed for that cell with
 * func_80133CD4; returns 1 on a hit and records the hit shape's id in D_80184130. */
#include "psx_types.h"

/* 3-short vector; D_8017E910/914/91C hold guest addresses of these. */
typedef struct {
    s16 vx;
    s16 vy;
    s16 vz;
} Vec3;

/* Cell grid. Pointer-valued fields are guest addresses (u32). */
typedef struct {
    u16 x0;      /* 0x00: first column */
    u16 z0;      /* 0x02: first row */
    u16 w;       /* 0x04: columns */
    u16 h;       /* 0x06: rows */
    u32 cells;   /* 0x08: u16 pairs {list offset, count} per cell */
    u32 arg3;    /* 0x0C: passed as func_80133CD4 arg 4 */
    u32 arg2;    /* 0x10: passed as func_80133CD4 arg 3 */
    u32 list;    /* 0x14: u16 shape ids, bit 15 selects the record table */
    u32 recA;    /* 0x18: 18-byte records */
    u32 recB;    /* 0x1C: 22-byte records */
} Grid;

extern u32 D_8017E910;
extern u32 D_8017E914;
extern u32 D_8017E91C;
extern s16 D_8018412C;
extern u16 D_80184130;
extern s16 func_80133CD4(s16 flags, u32 rec, u32 a2, u32 a3);

int func_80133AB0(s16 mode, u16 x, u16 z, Grid *g)
{
    u16 ix;
    u16 iz;
    u16 n;
    u16 *cell;
    u16 *p;
    u32 rec;
    u16 v;
    u16 flag;
    u32 cells;
    u32 list;
    u32 arg3;
    u32 arg2;
    u32 recA;
    u32 recB;
    s16 r;

    ((Vec3 *)D_8017E91C)->vx = ((Vec3 *)D_8017E914)->vx - ((Vec3 *)D_8017E910)->vx;
    ((Vec3 *)D_8017E91C)->vy = ((Vec3 *)D_8017E914)->vy - ((Vec3 *)D_8017E910)->vy;
    ((Vec3 *)D_8017E91C)->vz = ((Vec3 *)D_8017E914)->vz - ((Vec3 *)D_8017E910)->vz;

    if ((ix = (((x + 0x8000) >> 7) & 0x1FF) - g->x0) < g->w) {
        if ((iz = (((z + 0x8000) >> 7) & 0x1FF) - g->z0) >= g->h) {
            return 0;
        }
        cells = g->cells;
        list = g->list;
        arg3 = g->arg3;
        arg2 = g->arg2;
        recA = g->recA;
        recB = g->recB;
        cell = (u16 *)((u16)((iz * g->w + ix) * 2) * 2 + cells);
        p = (u16 *)(list + cell[0]);
        n = cell[1];
        while (n-- != 0) {
            v = *p;
            if ((flag = v & 0x8000) == 0) {
                rec = recA + v * 18;
            } else {
                rec = recB + (v & 0x7FFF) * 22;
            }
            r = func_80133CD4(flag | mode, rec, arg2, arg3);
            if (r != 0) {
                if (r > 0) {
                    D_80184130 = *(u16 *)rec;
                }
                return 1;
            }
            if (D_8018412C != 0) {
                D_80184130 = *(u16 *)rec;
                return 0;
            }
            p++;
        }
    }
    return 0;
}
