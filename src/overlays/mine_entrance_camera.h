#ifndef BFM_MINE_ENTRANCE_CAMERA_H
#define BFM_MINE_ENTRANCE_CAMERA_H

#include "sc02_0039/camera.h"

extern FacingActor D_80126948;
s32 func_8012A018(u32 callback_address, s32 mode);
s32 func_8012A094(u8 *record);
void func_8017D2E0(void);
void func_8017D384(FacingActor *camera);
void func_8017F07C(void);
void func_8017F120(FacingActor *camera);

/* These field symbols share storage with the camera record on the console. */
#if defined(__mips__) && defined(__LITTLE_ENDIAN__) && defined(__GNUC__) && __GNUC__ == 2
extern s32 D_80126950, D_80126954, D_8012695C;
extern s16 D_80126968, D_8012696A, D_8012696C, D_80126976, D_80126978, D_8012697A;
#define CAMERA_FIELD(original, field) original
#define CAMERA_TOKEN(function, address) ((u32)(function))
#else
#define CAMERA_FIELD(original, field) D_80126948.field
#define CAMERA_TOKEN(function, address) address
#endif

#endif
