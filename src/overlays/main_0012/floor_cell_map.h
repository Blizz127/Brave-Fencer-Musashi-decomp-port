#ifndef BFM_FLOOR_CELL_MAP_H
#define BFM_FLOOR_CELL_MAP_H

#include "floor_providers.h"

typedef struct { s16 v[3]; s16 w; } FloorPlane;
typedef struct {
    u16 x0, z0, w, h;
    u32 cells, table, planes, lists, plain, flagged;
} FloorCellMap;

s32 func_801345F8(FloorCellMap *map);
s32 func_801347A0(s16 flag, s16 *record, FloorPlane *planes, s32 *table);

#if defined(__mips__) && !defined(__LP64__)
#define FLOOR_ADDRESS(type, address) ((type *)(address))
#else
/* Host callers provide the backing store for each original 32-bit address. */
void *bfm_floor_address(u32 address);
#define FLOOR_ADDRESS(type, address) ((type *)bfm_floor_address(address))
#endif
#endif
