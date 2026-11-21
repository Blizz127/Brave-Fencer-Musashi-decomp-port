/* Select the local contact representation and translate both segment endpoints. */
#include "forest_floor.h"

#ifdef __mips__
typedef u32 FloorAddress;
#else
#include <stdint.h>
typedef uintptr_t FloorAddress;
#endif

typedef struct { s32 x, y, z, pad; } LongVector;

/* External LIBGTE matrix services. The public ApplyTransposeMatrixLV entry is a stub. */
extern void func_80048EAC(void *matrix, void *result);
extern void func_8004D504(void *matrix, LongVector *in, LongVector *out);
extern void func_80048B6C(LongVector *in, LongVector *out);
extern void func_8004914C(void *matrix);
extern void func_8004864C(FloorPoint *in, s32 *out);

s32 func_80135480(FloorContactRecord *record, u32 flags, FloorPoint *near_point, FloorPoint *far_point)
{
    LongVector start, end;
    s32 transformed[4];
    FloorMatrix scale;
    FloorPoint v;
    FloorBox *box;
    s16 result;
    s32 mode;

    start.x = (s16)near_point->x - record->x;
    start.z = (s16)near_point->z - record->z;
    if (!(flags & 0x10000000u)
        && (s32)((u32)start.x * (u32)start.x + (u32)start.z * (u32)start.z) > 0x40000)
        return 0;
    mode = flags & 0x60000000u;
    if (mode) {
        result = 1;
        if ((s32)flags >= 0)
            mode &= 0x40000000u;
        start.y = (s16)near_point->y - record->y;
        end.x = (s16)far_point->x - record->x;
        end.y = (s16)far_point->y - record->y;
        end.z = (s16)far_point->z - record->z;
        switch (mode) {
        case 0x60000000: {
            s32 inverse_y, inverse_z;
            D_801D9504.rotation.pairs[0] = 0x1000000 / record->scale_x;
            inverse_y = 0x1000000 / record->scale_y;
            inverse_z = 0x1000000 / record->scale_z;
            D_801D9504.rotation.pairs[1] = 0;
            D_801D9504.rotation.pairs[3] = 0;
            D_801D9504.rotation.pairs[2] = inverse_y;
            D_801D9504.rotation.coefficients[8] = (s16)inverse_z;
            func_80048EAC(&record->rotation, &D_801D9504);
            func_8004D504(&D_801D9504, &start, &start);
            func_80048B6C(&end, &end);
            result = 2;
        }
        /* Fall through. */
        case 0x20000000: {
            s32 sx = record->scale_x;
            s32 sy = record->scale_y;
            u16 sz = (u16)record->scale_z;
            box = (FloorBox *)(FloorAddress)((flags & 0x0fffffff) | 0x80000000);
            scale.rotation.pairs[0] = sx;
            scale.rotation.pairs[1] = 0;
            scale.rotation.pairs[2] = sy;
            scale.rotation.pairs[3] = 0;
            scale.rotation.coefficients[8] = sz;
            func_8004914C(&scale);
            v.x = box->xlo;
            v.y = box->ylo;
            v.z = box->zlo;
            func_8004864C(&v, transformed);
            D_801D9524.xlo = transformed[0];
            D_801D9524.ylo = transformed[1];
            D_801D9524.zlo = transformed[2];
            v.x = box->xhi;
            v.y = box->yhi;
            v.z = box->zhi;
            func_8004864C(&v, transformed);
            D_801D9524.next = 0;
            D_801D9524.xhi = transformed[0];
            D_801D9524.yhi = transformed[1];
            D_801D9524.zhi = transformed[2];
            result += 2;
            break;
        }
        case 0x40000000:
            func_8004D504(&record->rotation, &start, &start);
            func_80048B6C(&end, &end);
            result = 2;
            break;
        }
        D_801870AC->x = start.x;
        D_801870AC->y = start.y;
        D_801870AC->z = start.z;
        D_801870B0->x = end.x;
        D_801870B0->y = end.y;
        D_801870B0->z = end.z;
        return result;
    }
    D_801870AC->x = start.x;
    D_801870AC->y = (s16)near_point->y - record->y;
    D_801870AC->z = start.z;
    D_801870B0->x = (s16)far_point->x - record->x;
    D_801870B0->y = (s16)far_point->y - record->y;
    D_801870B0->z = (s16)far_point->z - record->z;
    return 1;
}
