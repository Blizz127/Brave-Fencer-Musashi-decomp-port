#ifndef BFM_MINE_TERRAIN_CELL_H
#define BFM_MINE_TERRAIN_CELL_H

#include "../main_0012/floor_cell_map.h"

extern u32 D_8017F68C, D_8017F690, D_8017F698;
extern s16 D_8019F9C4;
extern u16 D_8019F9C8;

s32 func_80133AB0(s16 mode, s16 previous_x, s16 previous_z, FloorCellMap *map);
s16 func_80133CD4(s16 mode, s16 *face, FloorPlane *planes, s32 *distances);

#endif
