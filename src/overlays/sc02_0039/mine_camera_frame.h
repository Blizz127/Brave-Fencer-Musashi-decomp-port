#ifndef BFM_MINE_CAMERA_FRAME_H
#define BFM_MINE_CAMERA_FRAME_H
#include "camera.h"
#include "mine_guest_abi.h"

extern u8 D_801150D6, D_80127504;
extern FacingActor D_80126948, D_80126E60, D_80126F04, D_80126FA8, D_80126DB8;
extern u32 D_801151D4;
void func_80129CF8(void);

#if !BFM_MINE_DIRECT_GUEST_ADDRESS
extern u8 D_800AF630[], D_800AE688[];
void bfm_camera_frame_prepare(void);
void bfm_camera_frame_set_parameter(s32 parameter);
s32 bfm_camera_frame_ease(s16 current, s16 target, s16 divisor, s16 minimum);
void bfm_camera_frame_submit_view(s32 *view);
SceneCameraHandler bfm_camera_frame_hook(u32 key);
void bfm_camera_frame_refused(u32 key);
#endif
#endif
