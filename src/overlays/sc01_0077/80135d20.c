/* Test the forest object's surface and transform the first contact into the probe. */
#include "forest_floor.h"

/* Direct addresses belong to the historical compiler. A host resolves 32-bit game handles. */
#if defined(__mips__) && !defined(__LP64__)
#define DIRECT_FOREST_PROBE 1
#define PROBE_RECORD(address) ((FloorContactRecord *)(address))
#define NEXT_BOX(box) ((FloorBox *)(box)->next)
#define FIRST_BOX(tag) ((FloorBox *)(((tag) & 0x0fffffffu) | 0x80000000u))
#else
#define DIRECT_FOREST_PROBE 0
extern void *bfm_forest_contact_pointer(u32 address, u32 width);
#define PROBE_RECORD(address) ((FloorContactRecord *)bfm_forest_contact_pointer((address), sizeof(FloorContactRecord)))
#define NEXT_BOX(box) forest_next_box(box)
#define FIRST_BOX(tag) ((s32)(tag) < 0 ? (FloorBox *)forest_require_model(((tag) & 0x0fffffffu) | 0x80000000u, 1) : (FloorBox *)0)
#endif

s32 func_80135D20(u32 record, u32 geometry, FloorPoint *probe)
{
#if DIRECT_FOREST_PROBE
    u8 workspace[96];
#define model PROBE_RECORD(record)
#else
    FloorContactRecord *model;
#endif
    FloorPoint far_point;
    FloorBox *box;
    s32 rotate;
    void *matrix;
#if DIRECT_FOREST_PROBE
    /* Keep the original unused local stack space below the far point. */
    (void)workspace;
#else
    if (record == 0)
        bfm_forest_model_invalid_handle(record);
    model = PROBE_RECORD(record);
    if (model == 0)
        bfm_forest_model_invalid_handle(record);
#endif
    far_point.x = probe->x;
    far_point.y = probe->y + 768;
    far_point.z = probe->z;
    switch (func_80135480(model, geometry, probe, &far_point)) {
    case 0:
        return 0;
    case 1:
        matrix = &model->rotation;
        box = FIRST_BOX(geometry);
        rotate = 0;
        break;
    case 2:
        matrix = &model->rotation;
        box = FIRST_BOX(geometry);
        rotate = 1;
        break;
    case 3:
        matrix = &model->rotation;
        box = &D_801D9524;
        rotate = 0;
        break;
    case 4:
        matrix = &D_801D9504;
        box = &D_801D9524;
        rotate = 1;
        break;
#if !DIRECT_FOREST_PROBE
    default:
        return 0;
#endif
    }
    if ((s32)geometry < 0) {
        if (func_80135EB0(box, 0))
            goto hit;
        box = NEXT_BOX(box);
        while (box != 0) {
            if (func_80135EB0(box, 0)) {
hit:
                func_80136A94(rotate, model, probe, matrix);
                return 1;
            }
            box = NEXT_BOX(box);
        }
    } else {
#if DIRECT_FOREST_PROBE
        FloorGrid *grid = (FloorGrid *)geometry;
#else
        FloorGrid *grid = forest_require_model(geometry, 0);
#endif
        if (func_801345F8(grid))
            goto hit;
    }
    return 0;
}
