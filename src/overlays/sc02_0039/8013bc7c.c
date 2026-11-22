#include "mine_frame_callback.h"

#if BFM_MINE_DIRECT_GUEST_ADDRESS
extern u32 D_8017F0B4[];

typedef struct {
    u8 reserved[0x68];
    u32 index;
} MineCallbackRecord;

void func_8013BC7C(u8 *record)
{
    ((void (*)(u8 *))D_8017F0B4[((MineCallbackRecord *)record)->index])(record);
}
#else
int bfm_mine_frame_record_step(u8 *record, unsigned bytes)
{
    const u8 *p;
    u32 index, target;
    if (!record || bytes < 0x6c)
        return 0;
    p = record + 0x68;
    index = (u32)p[0] | (u32)p[1] << 8 | (u32)p[2] << 16 | (u32)p[3] << 24;
    if (!bfm_mine_frame_callback_word(BFM_MINE_RECORD_CALLBACK_TABLE + 4u * index, &target) ||
            !target || (target & 3u))
        return 0;
    return bfm_mine_record_callback(target, record);
}

void func_8013BC7C(u8 *record)
{
    (void)bfm_mine_frame_record_step(record, 0x6c);
}
#endif
