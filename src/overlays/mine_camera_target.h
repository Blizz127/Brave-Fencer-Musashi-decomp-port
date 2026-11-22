#ifndef BFM_MINE_CAMERA_TARGET_H
#define BFM_MINE_CAMERA_TARGET_H

#include "sc02_0039/camera.h"

#if defined(__mips__) && defined(__LITTLE_ENDIAN__) && defined(__GNUC__) && __GNUC__ == 2
/* The room reads the contiguous eight-byte anchor from this address. */
extern SceneVector D_80126940;
#define CAMERA_ANCHOR() D_80126940
#else
extern u16 D_80126940;
extern s16 D_80126942, D_80126944, D_80126946;

/* The registration helper publishes each axis through a separate symbol. */
static inline SceneVector mine_camera_anchor(void)
{
    SceneVector anchor;
    anchor.position.x = (s16)D_80126940;
    anchor.position.y = D_80126942;
    anchor.position.z = D_80126944;
    anchor.padding = D_80126946;
    return anchor;
}
#define CAMERA_ANCHOR() mine_camera_anchor()
#endif

void func_8017D3C0(FacingActor *camera);
void func_8017F15C(FacingActor *camera);

#endif
