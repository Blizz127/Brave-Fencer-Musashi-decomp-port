/* Test the starting grid cell, then the destination cell when the segment crosses a boundary. */
#include "forest_floor.h"

#ifdef __mips__
typedef FloorPoint CellPoint;
#else
/* Work points and incoming endpoints may overlap at halfword offsets. */
typedef volatile FloorPoint CellPoint;
#endif

#define NEAR_POINT ((CellPoint *)D_801870AC)
#define FAR_POINT ((CellPoint *)D_801870B0)

s32 func_801343C4(s16 mode, FloorPoint *from, FloorPoint *to)
{
    s32 flags = mode;
    u16 z;

#ifdef __mips__
    NEAR_POINT->x = ((CellPoint *)from)->x;
    NEAR_POINT->y = ((CellPoint *)from)->y;
    NEAR_POINT->z = ((CellPoint *)from)->z;
    FAR_POINT->x = ((CellPoint *)to)->x;
    FAR_POINT->y = ((CellPoint *)to)->y;
    FAR_POINT->z = ((CellPoint *)to)->z;
    D_801D9500 = 0;
    D_801D94FC = 0;
    if (func_80133AB0(flags, (s16)NEAR_POINT->x, (s16)NEAR_POINT->z, D_801D94F0) != 0) {
#else
    /* Preserve the first call's observations if a point overlaps a contact
     * field. The callbacks may rebind any of these slots before the retry. */
    CellPoint *first_near = *(FloorPoint *volatile *)&D_801870AC;
    u16 source_x = ((CellPoint *)from)->x;
    FloorGrid *first_grid = *(FloorGrid *volatile *)&D_801D94F0;
    CellPoint *first_far = *(FloorPoint *volatile *)&D_801870B0;
    first_near->x = source_x;
    first_near->y = ((CellPoint *)from)->y;
    first_near->z = ((CellPoint *)from)->z;
    first_far->x = ((CellPoint *)to)->x;
    first_far->y = ((CellPoint *)to)->y;
    first_far->z = ((CellPoint *)to)->z;
    s16 first_x = (s16)first_near->x;
    s16 first_z = (s16)first_near->z;
    *(volatile u16 *)&D_801D9500 = 0;
    *(volatile s16 *)&D_801D94FC = 0;
    if (func_80133AB0(flags, first_x, first_z, first_grid) != 0) {
#endif
hit:
        ((CellPoint *)to)->x = FAR_POINT->x;
        ((CellPoint *)to)->y = FAR_POINT->y;
        z = FAR_POINT->z;
        ((CellPoint *)to)->w = D_801D9500;
        ((CellPoint *)to)->z = z;
        return 1;
    }
    if ((((s16)NEAR_POINT->x & 0xff80) != ((s16)FAR_POINT->x & 0xff80)
         || ((s16)NEAR_POINT->z & 0xff80) != ((s16)FAR_POINT->z & 0xff80))
        && func_80133AB0(flags, (s16)FAR_POINT->x, (s16)FAR_POINT->z, D_801D94F0) != 0) {
        goto hit;
    }
    return 0;
}
