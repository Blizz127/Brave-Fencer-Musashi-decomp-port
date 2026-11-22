#include "screen_effect.h"

#if BFM_SCREEN_DIRECT
#define unsigned_half(v) ((u16)(v))
#define signed_half(v) ((s16)(v))
#define put_half(v, value) ((v) = (value))
#define copy_pair(dst, src) ((dst) = (src))
#define geometry_half(value) ((s16)(value))
#define scale12(value) ((value) >> 12)
#else
#include <string.h>

static u16 read_half(const ScreenHalf *p)
{
    return (u16)(p->le[0] | (u16)p->le[1] << 8);
}

static s32 sign_half(u16 value)
{
    return (s32)(value ^ 0x8000u) - 0x8000;
}

static s32 scale12(s32 value)
{
    return value < 0 ? -1 - (-(value + 1) / 4096) : value / 4096;
}

#define geometry_half(value) sign_half((u16)(value))

static void write_half(ScreenHalf *p, u16 value)
{
    p->le[0] = (u8)value;
    p->le[1] = (u8)(value >> 8);
}

static u32 guest_address(const ScreenAddress *p)
{
    return (u32)p->le[0] | (u32)p->le[1] << 8 |
           (u32)p->le[2] << 16 | (u32)p->le[3] << 24;
}
#define unsigned_half(v) read_half(&(v))
#define signed_half(v) sign_half(read_half(&(v)))
#define put_half(v, value) write_half(&(v), (u16)(value))
#define copy_pair(dst, src) memcpy(&(dst), &(src), sizeof(ScreenPair))
#endif

void func_8013A530(ScreenWork *work)
{
#if BFM_SCREEN_DIRECT
    ScreenEffect *effect = (ScreenEffect *)work->effect_address;
    u16 mode = unsigned_half(effect->mode);
    u8 flags = effect->flags;
#else
    ScreenEffect *effect;
    u16 mode;
    u8 flags;
#endif
    u16 width;

#if !BFM_SCREEN_DIRECT
    if (!work)
        return;
    effect = bfm_screen_effect_resolve(guest_address(&work->effect_address));
    if (!effect)
        return;
    mode = unsigned_half(effect->mode);
    flags = effect->flags;
#endif
    if (mode == 0 || mode >= 7)
        return;
    switch (mode) {
    case 2:
    case 3:
    case 4:
    case 5:
    case 6: {
        u32 height = unsigned_half(effect->extent_y) + 16;

        width = unsigned_half(effect->extent_x) + 16;
        if (effect->wide)
            width = unsigned_half(effect->extent_x) + 56;
        put_half(work->scaled.x, (signed_half(effect->factors.x) * width) / 154);
        if (!(flags & 8))
            put_half(work->scaled.x, -signed_half(work->scaled.x));
        if (!(flags & 0x60) || (flags & 0x80))
            put_half(work->scaled.x, -signed_half(work->scaled.x));
        put_half(work->scaled.y, (signed_half(effect->factors.y) * (u16)height) / 42);
        if (flags & 16)
            put_half(work->scaled.y, -signed_half(work->scaled.y));
        if (signed_half(work->level) < signed_half(work->target)) {
            put_half(work->level, signed_half(work->level) + 1024);
            if (signed_half(work->level) > signed_half(work->target))
                put_half(work->level, signed_half(work->target));
        } else if (signed_half(work->level) > signed_half(work->target)) {
            put_half(work->level, signed_half(work->level) - 1024);
            if (signed_half(work->level) < signed_half(work->target))
                put_half(work->level, signed_half(work->target));
        }
        bfm_screen_observe_3aa24(effect, work, work, height);
        break;
    }
    case 1: {
        u16 left, top, height;

        copy_pair(work->scaled, effect->factors);
        width = scale12((signed_half(effect->extent_x) + 16) * signed_half(work->scaled.x));
        height = scale12((signed_half(effect->extent_y) + 16) * signed_half(work->scaled.y));
        left = unsigned_half(effect->center_x) - (width >> 1);
        top = unsigned_half(effect->center_y) - (height >> 1);
        bfm_screen_observe_15f04(geometry_half(left), geometry_half(top), geometry_half(width), geometry_half(height),
                                179, 153, 76, unsigned_half(effect->parameter), 4);
        bfm_screen_observe_15f04(geometry_half(left + 1), geometry_half(top + 1),
                                geometry_half(width - 2), geometry_half(height - 2),
                                230, 204, 115, unsigned_half(effect->parameter), 4);
        bfm_screen_observe_15d4c(geometry_half(left), geometry_half(top), geometry_half(width), geometry_half(height),
                                255, 230, 153, unsigned_half(effect->parameter), 4);
        break;
    }
    }
}
