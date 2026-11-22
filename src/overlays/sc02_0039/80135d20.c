/* Probe model geometry beneath the room object and transform the first contact. */
#include "model_surface_probe.h"

#ifdef __mips__
#define resolve_record(address) ((SceneSurfaceRecord *)(address))
#define resolve_box(address) ((SceneSurfaceBox *)(address))
#define initial_box(tag) resolve_box(((tag) & 0x0fffffffu) | 0x80000000u)
#else
#define resolve_record(address) bfm_model_probe_record(address)
#define resolve_box(address) ((address) ? bfm_model_probe_box(address) : (SceneSurfaceBox *)0)
#define initial_box(tag) (((s32)(tag) < 0) ? resolve_box(((tag) & 0x0fffffffu) | 0x80000000u) : (SceneSurfaceBox *)0)
#endif
extern SceneSurfaceBox D_8019F9EC;
extern u8 D_8019F9CC[];

extern s32 func_80135480(SceneSurfaceRecord *model, u32 tag, SceneProbeVector *from, SceneProbeVector *to);
extern s32 func_80135EB0(SceneSurfaceBox *box, s32 flag);
extern void func_80136A94(s32 rotate, SceneSurfaceRecord *model, SceneProbeVector *out, void *matrix);

s32 func_80135D20(u32 record_address, u32 tag, SceneProbeVector *probe)
{
#ifndef __mips__
    SceneSurfaceRecord *model = resolve_record(record_address);
#else
#define model resolve_record(record_address)
    /* The original caller reserves this local space before the far point. */
    u8 workspace[96];
#endif
    SceneProbeVector far;
    SceneSurfaceBox *box;
    s32 rotate;
    void *matrix;
#ifdef __mips__
    (void)workspace;
#else
    if (model == 0)
        return 0;
#endif
    far.x = probe->x;
    far.y = probe->y + 768;
    far.z = probe->z;
    switch (func_80135480(model, tag, probe, &far)) {
    case 0:
        return 0;
    case 1:
        matrix = model->matrix;
        box = initial_box(tag);
        rotate = 0;
        break;
    case 2:
        matrix = model->matrix;
        box = initial_box(tag);
        rotate = 1;
        break;
    case 3:
        matrix = model->matrix;
        box = &D_8019F9EC;
        rotate = 0;
        break;
    case 4:
        matrix = D_8019F9CC;
        box = &D_8019F9EC;
        rotate = 1;
        break;
#ifndef __mips__
    default:
        return 0;
#endif
    }
    if ((s32)tag < 0) {
#ifndef __mips__
        if (box == 0)
            return 0;
#endif
        if (func_80135EB0(box, 0))
            goto hit;
        box = resolve_box(box->next);
        while (box != 0) {
            if (func_80135EB0(box, 0)) {
hit:
                func_80136A94(rotate, model, probe, matrix);
                return 1;
            }
            box = resolve_box(box->next);
        }
    } else {
#ifdef __mips__
        FloorCellMap *grid = (FloorCellMap *)tag;
#else
        FloorCellMap *grid = bfm_model_probe_grid(tag);
        if (grid == 0)
            return 0;
#endif
        if (func_801345F8(grid))
            goto hit;
    }
    return 0;
}
