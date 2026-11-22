/* Owner clean-room reconstruction of the opening movie's output callback. */
#include "psx_types.h"

typedef struct {
    s16 x;
    s16 y;
    s16 width;
    s16 height;
} MovieRect;

typedef struct {
    u32 buffers[2];
    u8 pad08[0x14];
    s32 frame_index;
    MovieRect strip;
    s32 frame_complete;
    s32 stream_mode;
    u8 pad30[0x10];
    s32 first_strip;
} MovieTransfer;

extern MovieTransfer D_800EC9B0;
extern s32 D_800EC9B8;
extern MovieRect D_800EC9BC[2];
extern s16 D_800EC9D0;
extern s16 D_800EC9D2;
extern s16 D_800EC9D4;
extern s16 D_800EC9D6;
extern s32 D_800A5F1C;

extern void func_80046D1C(void);
extern void func_800D2C88(u32 buffer, u32 words);
extern void func_800599B8(MovieRect *rect, u32 buffer);

void func_800D24D0(void)
{
    MovieRect upload[2];
    s32 *buffer_index;
    s32 completed_buffer;
    s32 remainder;

    if (D_800EC9B0.stream_mode && D_800A5F1C) {
        func_80046D1C();
        D_800A5F1C = 0;
    }

    buffer_index = &D_800EC9B8;
    completed_buffer = *buffer_index;
    upload[0] = D_800EC9B0.strip;
    *buffer_index = (*buffer_index == 0);

    if (D_800EC9B0.first_strip) {
        remainder = D_800EC9BC[D_800EC9B0.frame_index].width % D_800EC9D4;
        if (remainder != 0) {
            D_800EC9D0 += remainder;
            D_800EC9B0.first_strip = 0;
            goto check_frame_end;
        }
    }
    D_800EC9B0.strip.x += D_800EC9D4;

check_frame_end:
    if (D_800EC9B0.strip.x < D_800EC9BC[D_800EC9B0.frame_index].x +
                              D_800EC9BC[D_800EC9B0.frame_index].width) {
        func_800D2C88(D_800EC9B0.buffers[D_800EC9B8],
                      ((D_800EC9D4 << 4) * ((D_800EC9D6 - 1) / 16 + 1)) >> 1);
    } else {
        D_800EC9B0.frame_complete = 1;
        D_800EC9B0.frame_index = (D_800EC9B0.frame_index == 0);
        D_800EC9B0.strip.x = D_800EC9BC[D_800EC9B0.frame_index].x;
        D_800EC9D2 = D_800EC9BC[D_800EC9B0.frame_index].y;
        D_800EC9B0.first_strip = 1;
    }

    func_800599B8(&upload[0], D_800EC9B0.buffers[completed_buffer]);
}
