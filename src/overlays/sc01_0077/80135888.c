/* Select the collision model, test its faces and transform the first contact. */
#include "forest_floor.h"
#ifdef __mips__
typedef u32 CollisionAddress;
typedef FloorPoint SegmentPoint;
#else
#include <stdint.h>
typedef uintptr_t CollisionAddress;
/* Halfword stores can overlap the next coordinate read through another point. */
typedef volatile FloorPoint SegmentPoint;
#endif
extern s32 func_80135EB0(FloorBox *node, s32 flags);

s32 func_80135888(FloorContactRecord *record, s32 packed, FloorPoint *start, FloorPoint *end)
{
    /* The original frame reserves these 32 bytes above the outgoing arguments. */
    u8 workspace[32];
    void *model;
    void *matrix;
    s32 rotate;
    s32 kind;
    SegmentPoint *near_point;
    SegmentPoint *far_point;
    SegmentPoint *movement;
    (void)workspace;
    kind = func_80135480(record, packed, start, end);
    switch (kind) {
#if defined(__GNUC__) && __GNUC__ >= 4
    /* The paired classifier can only return zero through four. */
    default: __builtin_unreachable();
#endif
    case 0:
        return 0;
    case 1:
        matrix = &record->rotation;
        model = (void *)(CollisionAddress)(((u32)packed & 0x0fffffffu) | 0x80000000u);
        rotate = 0;
        break;
    case 2:
        matrix = &record->rotation;
        model = (void *)(CollisionAddress)(((u32)packed & 0x0fffffffu) | 0x80000000u);
        rotate = 1;
        break;
    case 3:
        matrix = &record->rotation;
        model = &D_801D9524;
        rotate = 0;
        break;
    case 4:
        matrix = &D_801D9504;
        model = &D_801D9524;
        rotate = 1;
        break;
    }
    if (packed < 0) {
        if (func_80135EB0(model, 0)) goto hit;
        model = (void *)(CollisionAddress)((FloorBox *)model)->next;
        while (model) {
            if (func_80135EB0(model, 0)) {
hit:
                func_80136A94(rotate, record, end, matrix);
                return 1;
            }
            model = (void *)(CollisionAddress)((FloorBox *)model)->next;
        }
    } else {
        far_point = D_801870B0;
        near_point = D_801870AC;
        movement = D_801870B8;
        movement->x = far_point->x - near_point->x;
        movement->y = far_point->y - near_point->y;
        movement->z = far_point->z - near_point->z;
        if (func_80134A74(0, (s16)near_point->x, (s16)near_point->z, model)) goto hit;
    }
    return 0;
}
