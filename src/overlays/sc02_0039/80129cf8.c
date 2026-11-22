/* Updates the chosen camera and publishes its view after the matrix service returns. */
#include "mine_camera_frame.h"

#if BFM_MINE_DIRECT_GUEST_ADDRESS
typedef struct {
    s16 m[3][3], padding;
    s32 t[3];
} CameraFrameMatrix;

typedef struct {
    u8 before_matrix[0x18];
    CameraFrameMatrix mat;
} CameraFrameScreen;

extern CameraFrameScreen D_800AF630;
extern CameraFrameMatrix D_800AE688;
extern void func_8012A328(void);
extern void func_80053308(s32 parameter);
extern void func_80054DCC(s32 *view);
extern s16 func_80012F74(s16 current, s16 target, s16 divisor, s16 minimum);
#define PREPARE_FRAME func_8012A328
#define SET_FRAME_PARAMETER func_80053308
#define SUBMIT_FRAME_VIEW func_80054DCC
#define EASE_COMPONENT(current, target) func_80012F74((s16)(current), (s16)(target), 2, 1)
#define FRAME_ADD(first, second) ((s32)((u32)(first) + (u32)(second)))
#else
#include <string.h>
#define PREPARE_FRAME bfm_camera_frame_prepare
#define SET_FRAME_PARAMETER bfm_camera_frame_set_parameter
#define SUBMIT_FRAME_VIEW bfm_camera_frame_submit_view

static s16 frame_half(s32 value)
{
    s32 bits = (s32)((u32)value & 0xffffu);
    return (s16)(bits >= 32768 ? bits - 65536 : bits);
}

static s32 frame_add(s32 first, s32 second)
{
    u32 bits = (u32)first + (u32)second;
    return bits <= 0x7fffffffu ? (s32)bits : -1 - (s32)~bits;
}
static s32 ease_component(s32 current, s32 target)
{
    return frame_half(bfm_camera_frame_ease(frame_half(current), frame_half(target), 2, 1));
}
#define EASE_COMPONENT ease_component
#define FRAME_ADD frame_add

static u32 callback_key(const FacingActor *cam)
{
    const u8 *b = cam->callback_address;
    return (u32)b[0] | ((u32)b[1] << 8) | ((u32)b[2] << 16) | ((u32)b[3] << 24);
}

static u32 published_key(const FacingActor *cam)
{
    if (cam == &D_80126E60) return 0x80126E60u;
    if (cam == &D_80126F04) return 0x80126F04u;
    if (cam == &D_80126FA8) return 0x80126FA8u;
    return 0x80126948u;
}
#endif

void func_80129CF8(void)
{
    FacingActor *cam, *cur;
    SceneCameraHandler hook;
#if BFM_MINE_DIRECT_GUEST_ADDRESS
    CameraFrameScreen *screen;
    CameraFrameMatrix scratch;
#else
    u32 callback;
#endif

    PREPARE_FRAME();
#if BFM_MINE_DIRECT_GUEST_ADDRESS
    screen = &D_800AF630;
#endif
    cur = &D_80126DB8;
    if (D_801150D6 == 1) {
        cam = &D_80126E60;
    } else if (D_801150D6 == 2) {
        cam = &D_80126F04;
    } else {
        cam = &D_80126948;
        if (D_801150D6 == 3) {
            cam = &D_80126FA8;
        }
    }
#if BFM_MINE_DIRECT_GUEST_ADDRESS
    hook = (SceneCameraHandler)*(u32 *)cam->callback_address;
#else
    callback = callback_key(cam);
    hook = callback ? bfm_camera_frame_hook(callback) : 0;
    if (callback && !hook) {
        bfm_camera_frame_refused(callback);
        return;
    }
#endif
    if (hook != 0) {
        hook(cam);
    }
    SET_FRAME_PARAMETER(cam->distance);
    if (D_80127504 != 0) {
        cur->camera.x = EASE_COMPONENT(cur->camera.x, cam->camera.x);
        cur->camera.y = EASE_COMPONENT(cur->camera.y, cam->camera.y);
        cur->camera.z = EASE_COMPONENT(cur->camera.z, cam->camera.z);
        cur->focus.x = EASE_COMPONENT(cur->focus.x, cam->focus.x);
        cur->focus.y = EASE_COMPONENT(cur->focus.y, cam->focus.y);
        cur->focus.z = EASE_COMPONENT(cur->focus.z, cam->focus.z);
        cur->frame_component = EASE_COMPONENT(cur->frame_component, cam->frame_component);
        D_80127504--;
    } else {
        cur->camera.x = cam->camera.x;
        cur->camera.y = cam->camera.y;
        cur->camera.z = cam->camera.z;
        cur->focus.x = cam->focus.x;
        cur->focus.y = cam->focus.y;
        cur->focus.z = cam->focus.z;
        cur->frame_component = cam->frame_component;
    }
    cam->view[0] = FRAME_ADD(cur->camera.x, cam->view_offset[0]);
    cam->view[1] = FRAME_ADD(cur->camera.y, cam->view_offset[1]);
    cam->view[2] = FRAME_ADD(cur->camera.z, cam->view_offset[2]);
    cam->view[3] = FRAME_ADD(cur->focus.x, cam->view_offset[3]);
    cam->view[4] = FRAME_ADD(cur->focus.y, cam->view_offset[4]);
    cam->view[5] = FRAME_ADD(cur->focus.z, cam->view_offset[5]);
    cam->view[6] = FRAME_ADD(cur->frame_component, cam->view_offset[6]);
    cam->published_parameter = cam->view_parameter;
    SUBMIT_FRAME_VIEW(cam->view);
#if BFM_MINE_DIRECT_GUEST_ADDRESS
    screen->mat = D_800AE688;
    D_801151D4 = (u32)cam;
#else
    memcpy(D_800AF630 + 0x18, D_800AE688, 0x20);
    D_801151D4 = published_key(cam);
#endif
}
