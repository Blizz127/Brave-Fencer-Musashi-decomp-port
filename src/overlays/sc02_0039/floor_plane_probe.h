#ifndef BFM_FLOOR_PLANE_PROBE_H
#define BFM_FLOOR_PLANE_PROBE_H

#include "../main_0012/floor_cell_map.h"

s32 func_80134A28(const FloorPlane *plane, const SceneProbeVector *point, s32 offset);

#if !defined(__mips__) || defined(__LP64__)
/* Report the original divide-by-zero boundary before any hit output is written. */
void bfm_floor_plane_divide_by_zero(void);
#endif

#endif
