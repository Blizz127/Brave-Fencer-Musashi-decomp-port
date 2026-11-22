#ifndef BFM_MINE_INITIAL_PLACEMENT_H
#define BFM_MINE_INITIAL_PLACEMENT_H
#include "psx_types.h"

/* The zero type terminates each mutable list of twenty-byte placement records. */
typedef struct {
    s16 position[3];
    u16 type;
    s16 identifier;
    u16 flags;
    s16 yaw;
    u16 parameter;
    u32 argument;
} MineInitialPlacement;

extern MineInitialPlacement *D_801274C8;
extern MineInitialPlacement *D_801274CC;
s32 func_8012C750(MineInitialPlacement *row);
void func_8013E4B4(void);
#endif
