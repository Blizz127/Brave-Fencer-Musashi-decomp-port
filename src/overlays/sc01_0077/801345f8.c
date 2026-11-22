/* Walk the cell face IDs from last to first. A high ID bit selects the five-plane face table. */
#include "forest_floor.h"

#if defined(__mips__) && !defined(__LP64__)
typedef u32 FloorAddress;
#else
#include <stdint.h>
typedef uintptr_t FloorAddress;
#endif

s32 func_801345F8(FloorGrid *grid)
{
    FloorPoint *probe = D_801870AC;
    s16 cell_x, cell_z;
    s32 *distances;
    FloorPoint *vertices;
    u8 *short_faces, *long_faces;
    u16 remaining;
    u16 cell;
    u8 *face_ids;
    FloorAddress cursor;
    u16 id;
    FloorFace *face;
    s16 five_planes;
    u16 width = grid->w;
    s32 bias = 0x8000;

    /* The caller passes both ends; the query reads the shared probe. */
    cell_x = ((probe->x + bias) >> 7 & 0x1FF) - grid->x0;
    if ((u32)(cell_x & 0xFFFF) >= width) {
        return 0;
    }
    cell_z = ((probe->z + bias) >> 7 & 0x1FF) - grid->z0;
    if ((u32)(cell_z & 0xFFFF) >= grid->h) {
        return 0;
    }
    distances = grid->dist;
    vertices = grid->verts;
    short_faces = grid->faces18;
    long_faces = grid->faces22;
    face_ids = grid->ids;
    cell = ((u32)(u16)cell_z * width + (u16)cell_x) * 2;
    remaining = grid->cells[cell + 1];
    /* A zero or exhausted list can put this address before its first ID. */
    cursor = (FloorAddress)(face_ids + grid->cells[cell] + remaining * 2) - 2;
    while (remaining--) {
        id = *(u16 *)cursor;
        five_planes = id & 0x8000;
        if (five_planes == 0) {
            face = (FloorFace *)(short_faces + id * 18);
        } else {
            face = (FloorFace *)(long_faces + (id & 0x7FFF) * 22);
        }
        if (func_801347A0(five_planes, face, vertices, distances) != 0) {
            D_801D9500 = face->flags;
            return 1;
        }
        cursor -= 2;
    }
    return 0;
}
