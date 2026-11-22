/* Select the collision model, test its faces and transform the first contact. */
#include "forest_floor.h"
#define PACKED_ADDRESS(tag) (((u32)(tag) & 0x0fffffffu) | 0x80000000u)
#if defined(__mips__) && !defined(__LP64__)
#define DIRECT_MODEL 1
#define FIRST_MODEL(tag) ((void *)PACKED_ADDRESS(tag))
#define NEXT_BOX(box) ((void *)((FloorBox *)(box))->next)
typedef FloorPoint SegmentPoint;
#else
#define DIRECT_MODEL 0
/* Leave an external model unresolved until the query actually needs it. */
#define FIRST_MODEL(tag) ((void *)0)
#define NEXT_BOX(box) forest_next_box((FloorBox *)(box))
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
        model = FIRST_MODEL(packed);
        rotate = 0;
        break;
    case 2:
        matrix = &record->rotation;
        model = FIRST_MODEL(packed);
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
#if !DIRECT_MODEL
        if (!model) model = forest_require_model(PACKED_ADDRESS(packed), 1);
#endif
        if (func_80135EB0(model, 0)) goto hit;
        model = NEXT_BOX(model);
        while (model) {
            if (func_80135EB0(model, 0)) {
hit:
                func_80136A94(rotate, record, end, matrix);
                return 1;
            }
            model = NEXT_BOX(model);
        }
    } else {
        far_point = D_801870B0;
        near_point = D_801870AC;
        movement = D_801870B8;
        movement->x = far_point->x - near_point->x;
        movement->y = far_point->y - near_point->y;
        movement->z = far_point->z - near_point->z;
        {
            s16 x = (s16)near_point->x;
            s16 z = (s16)near_point->z;
#if !DIRECT_MODEL
            if (!model) model = forest_require_model(PACKED_ADDRESS(packed), 0);
#endif
            if (func_80134A74(0, x, z, model)) goto hit;
        }
    }
    return 0;
}
