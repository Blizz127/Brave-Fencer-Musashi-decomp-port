#ifndef BFM_MODEL_SURFACE_PROBE_H
#define BFM_MODEL_SURFACE_PROBE_H

#include "../main_0012/floor_cell_map.h"

typedef struct {
    s32 x, y, z, pad;
} SceneTransformVector;

typedef struct {
    u32 next;
    s16 bounds[6];
} SceneSurfaceBox;

typedef struct {
    u8 prefix[52];
    u8 matrix[32];
} SceneSurfaceRecord;

#ifndef __mips__
SceneSurfaceRecord *bfm_model_probe_record(u32 address);
SceneSurfaceBox *bfm_model_probe_box(u32 address);
FloorCellMap *bfm_model_probe_grid(u32 address);
SceneProbeVector *bfm_model_probe_point(u32 address);
#endif

s32 func_80135D20(u32 record_address, u32 tagged_geometry, SceneProbeVector *probe);

#endif
