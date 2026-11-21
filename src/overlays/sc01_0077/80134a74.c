/* Look up moving-segment contacts in the cell, from the last face ID to the first. */
#include "forest_floor.h"

#ifdef __mips__
typedef u32 ContactAddress;
#else
#include <stdint.h>
typedef uintptr_t ContactAddress;
#endif

s32 func_80134A74(s32 flags, u16 x, u16 z, FloorGrid *grid)
{
    s32 width;
    s16 column;
    s16 row;
    u16 slot;
    u16 *cell;
    s32 *distances;
    FloorPoint *vertices;
    u8 *small;
    u8 *large;
    FloorFace *record;
    u16 token;
    s16 kind;
#if defined(__GNUC__) && __GNUC__ == 2 && defined(__mips__)
    register ContactAddress cursor __asm__("$17");
    u16 count;
    register u8 *ids __asm__("$3");
#else
    ContactAddress cursor;
    u16 count;
    u8 *ids;
#endif

    column = (((x + 0x8000) >> 7) & 0x1FF) - grid->x0;
    width = grid->w;
    if ((u32)(column & 0xFFFF) >= (u32)width)
        return 0;
    row = (((z + 0x8000) >> 7) & 0x1FF) - grid->z0;
    if ((u32)(row & 0xFFFF) >= grid->h)
        return 0;
    slot = ((u16)row * (u32)width + (u16)column) * 2u;
    ids = (u8 *)grid->cells;
    cell = (u16 *)((u32)slot * 2u + (ContactAddress)ids);
    distances = grid->dist;
    vertices = grid->verts;
    small = grid->faces18;
    large = grid->faces22;
    ids = grid->ids;
    count = cell[1];
    ids += cell[0];
    ids += (u16)count * 2u;
    cursor = (ContactAddress)ids - 2u;
    while ((u16)count--) {
        token = *(u16 *)cursor;
        kind = token & 0x8000;
        if (kind == 0)
            record = (FloorFace *)(small + token * 18);
        else
            record = (FloorFace *)(large + (token & 0x7FFF) * 22);
        cursor -= 2u;
        if (func_80134C20((s16)(kind | flags), record, vertices, distances)) {
            D_801D9500 = record->flags;
            return 1;
        }
    }
    return 0;
}
