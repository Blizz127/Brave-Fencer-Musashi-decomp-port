/* Translate a forest contact and its auxiliary point after the optional matrix transform. */
#include "forest_floor.h"

/* Resident LIBGTE ApplyMatrixSV and ApplyRotMatrix services. */
extern void func_80048FBC(void *matrix, FloorPoint *in, FloorPoint *out);
extern void func_8004864C(FloorPoint *in, s32 *out);

void func_80136A94(s32 rotate, FloorContactRecord *record, FloorPoint *output, void *matrix)
{
    if (rotate != 0) {
        s32 transformed[3];
        FloorPoint *point = D_801870B4;
        FloorPoint *normal;

        func_80048FBC(matrix, point, point);
        func_80048FBC(matrix, &D_80126720, &D_80126720);
        normal = &D_801152A8;
        func_8004864C(normal, transformed);
        normal->x = (u16)transformed[0];
        D_801152A8.y = (u16)transformed[1];
        D_801152A8.z = (u16)transformed[2];
    }
    {
        FloorPoint *point = D_801870B4;
        FloorPoint *auxiliary;

        output->x = (u16)(point->x + record->x);
        output->y = (u16)(point->y + record->y);
        output->z = (u16)(point->z + record->z);
        auxiliary = &D_80126720;
        auxiliary->x = (u16)(auxiliary->x + record->x);
        D_80126720.y = (u16)(D_80126720.y + record->y);
        D_80126720.z = (u16)(D_80126720.z + record->z);
    }
}
