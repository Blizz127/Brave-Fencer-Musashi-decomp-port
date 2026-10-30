/* Owner re-derivation from retail assembly (clean input set). */
/* Picks the active camera block by D_801150D6, eases D_80126DB8 toward it, builds its view and publishes it in D_801151D4. */
#include "psx_types.h"

typedef struct Mat80129CF8 {
    /* 0x00 */ s16 m[3][3];
    /* 0x12 */ s16 pad12;
    /* 0x14 */ s32 t[3];
} Mat80129CF8; /* size 0x20 */

typedef struct Cam80129CF8 {
    /* 0x00 */ u32 hook; /* guest pointer: void (*)(Cam80129CF8 *) */
    /* 0x04 */ u8 pad04[4];
    /* 0x08 */ s32 h;
    /* 0x0C */ u8 pad0C[0x30];
    /* 0x3C */ s32 pos[7];
    /* 0x58 */ s32 f58;
    /* 0x5C */ s32 view[7];
    /* 0x78 */ s32 f78;
    /* 0x7C */ s32 ofs[7];
    /* 0x98 */ u8 pad98[0xC];
} Cam80129CF8; /* size 0xA4 */

typedef struct Scr80129CF8 {
    /* 0x00 */ u8 pad00[0x18];
    /* 0x18 */ Mat80129CF8 mat;
} Scr80129CF8;

typedef void (*Hook80129CF8)(Cam80129CF8 *cam);

extern u8 D_801150D6;
extern u8 D_80127504;
extern Cam80129CF8 D_80126948;
extern Cam80129CF8 D_80126E60;
extern Cam80129CF8 D_80126F04;
extern Cam80129CF8 D_80126FA8;
extern Cam80129CF8 D_80126DB8;
extern Scr80129CF8 D_800AF630;
extern Mat80129CF8 D_800AE688;
extern u32 D_801151D4; /* guest pointer: current camera */

extern void func_8012A328(void);
extern void func_80053308(s32 h);
extern s16 func_80012F74(s16 cur, s16 target, s16 div, s16 min);
extern void func_80054DCC(s32 *view);

void func_80129CF8(void)
{
    Cam80129CF8 *cam;
    Cam80129CF8 *cur;
    Hook80129CF8 hook;
    Scr80129CF8 *scr;
    Mat80129CF8 scratch; /* unreferenced: reproduces the retail 0x20-byte frame reserve */

    func_8012A328();
    scr = &D_800AF630;
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
    hook = (Hook80129CF8)cam->hook;
    if (hook != 0) {
        hook(cam);
    }
    func_80053308(cam->h);
    if (D_80127504 != 0) {
        cur->pos[0] = func_80012F74(cur->pos[0], cam->pos[0], 2, 1);
        cur->pos[1] = func_80012F74(cur->pos[1], cam->pos[1], 2, 1);
        cur->pos[2] = func_80012F74(cur->pos[2], cam->pos[2], 2, 1);
        cur->pos[3] = func_80012F74(cur->pos[3], cam->pos[3], 2, 1);
        cur->pos[4] = func_80012F74(cur->pos[4], cam->pos[4], 2, 1);
        cur->pos[5] = func_80012F74(cur->pos[5], cam->pos[5], 2, 1);
        cur->pos[6] = func_80012F74(cur->pos[6], cam->pos[6], 2, 1);
        D_80127504--;
    } else {
        cur->pos[0] = cam->pos[0];
        cur->pos[1] = cam->pos[1];
        cur->pos[2] = cam->pos[2];
        cur->pos[3] = cam->pos[3];
        cur->pos[4] = cam->pos[4];
        cur->pos[5] = cam->pos[5];
        cur->pos[6] = cam->pos[6];
    }
    cam->view[0] = cur->pos[0] + cam->ofs[0];
    cam->view[1] = cur->pos[1] + cam->ofs[1];
    cam->view[2] = cur->pos[2] + cam->ofs[2];
    cam->view[3] = cur->pos[3] + cam->ofs[3];
    cam->view[4] = cur->pos[4] + cam->ofs[4];
    cam->view[5] = cur->pos[5] + cam->ofs[5];
    cam->view[6] = cur->pos[6] + cam->ofs[6];
    cam->f78 = cam->f58;
    func_80054DCC(cam->view);
    scr->mat = D_800AE688;
    D_801151D4 = (u32)cam;
}
