#ifndef BFM_TOWER_POSTFIRE_CAMERA_H
#define BFM_TOWER_POSTFIRE_CAMERA_H

#include "../sc02_0039/camera.h"

typedef struct {
    s16 x, y, z;
    u16 pad;
} PostfireCameraRawPosition;

extern FacingActor D_80126948, D_80126FA8;
extern const PostfireCameraRawPosition D_801E2D90, D_801E2D98;
extern u16 D_80126940;
extern s16 D_80126942, D_80126944;
extern u8 D_801150D6;

#if defined(__mips__) && !defined(__LP64__)
/* The original camera fields each carry their own symbol relocation. */
extern s32 D_80126FB4;
extern s32 D_80126FE4, D_80126FE8, D_80126FEC;
extern s32 D_80126FF0, D_80126FF4, D_80126FF8;
/* Preserve A094's result across A100, whose original body leaves v0 intact. */
#define TOWER_CAMERA_RESULT(name) register u32 name __asm__("$2")
#else
#define D_80126FB4 (D_80126FA8.target_distance)
#define D_80126FE4 (D_80126FA8.camera.x)
#define D_80126FE8 (D_80126FA8.camera.y)
#define D_80126FEC (D_80126FA8.camera.z)
#define D_80126FF0 (D_80126FA8.focus.x)
#define D_80126FF4 (D_80126FA8.focus.y)
#define D_80126FF8 (D_80126FA8.focus.z)
#define TOWER_CAMERA_RESULT(name) u32 name
#endif

u32 func_8017E26C(void);
u32 func_80129FF4(void);
s32 func_8012A018(u32 callback_address, s32 mode);
s32 func_8012A048(u8 *camera, u32 callback_address, u8 mode);
s32 func_8012A094(u8 *camera);
void func_8012A100(u8 mode);
#endif
