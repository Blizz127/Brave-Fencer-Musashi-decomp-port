/* Scan this cell's face records in list order and keep its selected material. */
#include "forest_floor.h"
#ifdef __mips__
typedef u32 ContactAddress;
typedef FloorPoint ContactPoint;
#else
#include <stdint.h>
typedef uintptr_t ContactAddress;
/* The three work points may partly overlap at halfword boundaries. */
typedef volatile FloorPoint ContactPoint;
#endif

s32 func_80133AB0(s16 mode, s16 x, s16 z, FloorGrid *grid)
{
    u16 col, row, remaining;
    u16 *cell, *token;
    FloorFace *face;
    u16 id, large;
    ContactAddress cells;
    u8 *list, *short_faces, *long_faces;
    FloorPoint *vertices;
    s32 *distances;
    s16 result;
    ContactPoint *far_point = D_801870B0;
    ContactPoint *near_point = D_801870AC;
    ContactPoint *movement = D_801870B8;

    movement->x = far_point->x - near_point->x;
    movement->y = far_point->y - near_point->y;
    movement->z = far_point->z - near_point->z;
    if ((col = ((((u16)x + 0x8000) >> 7) & 0x1ff) - grid->x0) < grid->w) {
        if ((row = ((((u16)z + 0x8000) >> 7) & 0x1ff) - grid->z0) >= grid->h)
            return 0;
        cells = (ContactAddress)grid->cells;
        list = grid->ids;
        distances = grid->dist;
        vertices = grid->verts;
        short_faces = grid->faces18;
        long_faces = grid->faces22;
        cell = (u16 *)((u16)((row * (u32)grid->w + col) * 2) * 2 + cells);
        token = (u16 *)(list + cell[0]);
        remaining = cell[1];
        while (remaining-- != 0) {
            id = *token;
            if ((large = id & 0x8000) == 0)
                face = (FloorFace *)(short_faces + id * 18);
            else
                face = (FloorFace *)(long_faces + (id & 0x7fff) * 22);
            result = func_80133CD4((s16)(large | mode), face, vertices, distances);
            if (result != 0) {
                if (result > 0)
                    D_801D9500 = face->flags;
                return 1;
            }
            if (D_801D94FC != 0) {
                D_801D9500 = face->flags;
                return 0;
            }
            ++token;
        }
    }
    return 0;
}
