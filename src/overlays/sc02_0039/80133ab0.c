#include "mine_terrain_cell.h"

#if defined(__mips__) && !defined(__LP64__)
typedef SceneProbeVector MovementPoint;
#else
/* The three work vectors can overlap by a halfword. Preserve their access order. */
typedef volatile SceneProbeVector MovementPoint;
#endif

s32 func_80133AB0(s16 mode, s16 previous_x, s16 previous_z, FloorCellMap *map)
{
    u16 column, row, remaining;
    u16 *cell;
    u32 list, cursor, distance_table, plane_table, triangles, quads, selected;
    u16 id, shape;
    s16 result;
    MovementPoint *current = FLOOR_ADDRESS(MovementPoint, D_8017F690);
    MovementPoint *previous = FLOOR_ADDRESS(MovementPoint, D_8017F68C);
    MovementPoint *movement = FLOOR_ADDRESS(MovementPoint, D_8017F698);

    movement->x = current->x - previous->x;
    movement->y = current->y - previous->y;
    movement->z = current->z - previous->z;
    if ((column = ((((u16)previous_x + 0x8000) >> 7) & 0x1ff) - map->x0) < map->w) {
        if ((row = ((((u16)previous_z + 0x8000) >> 7) & 0x1ff) - map->z0) >= map->h)
            return 0;
        list = map->lists;
        distance_table = map->table;
        plane_table = map->planes;
        triangles = map->plain;
        quads = map->flagged;
        cell = FLOOR_ADDRESS(u16, (u16)((row * (u32)map->w + column) * 2) * 2 + map->cells);
        cursor = list + cell[0];
        remaining = cell[1];

        while (remaining-- != 0) {
            id = *FLOOR_ADDRESS(u16, cursor);
            if ((shape = id & 0x8000) == 0)
                selected = triangles + id * 18;
            else
                selected = quads + (id & 0x7fff) * 22;
            result = func_80133CD4((s16)(shape | mode), FLOOR_ADDRESS(s16, selected),
                                   FLOOR_ADDRESS(FloorPlane, plane_table), FLOOR_ADDRESS(s32, distance_table));
            if (result != 0) {
                if (result > 0)
                    D_8019F9C8 = *FLOOR_ADDRESS(u16, selected);
                return 1;
            }
            if (D_8019F9C4 != 0) {
                D_8019F9C8 = *FLOOR_ADDRESS(u16, selected);
                return 0;
            }
            cursor += 2;
        }
    }
    return 0;
}
