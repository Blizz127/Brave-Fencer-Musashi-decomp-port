#ifndef BFM_MINE_FADE_LIFECYCLE_H
#define BFM_MINE_FADE_LIFECYCLE_H

#include "mine_actor_fade.h"

void func_8013A448(ScreenWork *work);
void func_8013A4C4(ScreenWork *work);

#if BFM_SCREEN_DIRECT
#define fade_address(w) ((w)->effect_address)
#define fade_resolve(address) ((ScreenEffect *)(address))
#define fade_substate(w) (*(u16 *)((w)->reserved00 + 2))
#define set_substate(w, value) (fade_substate(w) = (value))
#define set_state(w, value) (*(u16 *)((w)->reserved00) = (value))
#define fade_half(value) ((u16)(value))
#define fade_signed(value) ((s16)(value))
#define set_fade_half(field, value) ((field) = (value))
#define forget_effect(w) ((w)->effect_address = 0)
#else
static inline u16 fade_read_half(const u8 *p)
{
    return (u16)(p[0] | (u16)p[1] << 8);
}

static inline void fade_put_half(u8 *p, u16 value)
{
    p[0] = (u8)value;
    p[1] = (u8)(value >> 8);
}

static inline u32 fade_address(const ScreenWork *work)
{
    const u8 *p = work->effect_address.le;
    return (u32)p[0] | (u32)p[1] << 8 | (u32)p[2] << 16 | (u32)p[3] << 24;
}

static inline ScreenEffect *fade_resolve(u32 address)
{
    if (!address || (address & 1u) || address > 0xffffffc8u)
        return 0;
    return bfm_screen_effect_resolve(address);
}

static inline void forget_effect(ScreenWork *work)
{
    u8 *p = work->effect_address.le;
    p[0] = p[1] = p[2] = p[3] = 0;
}

#define fade_substate(w) fade_read_half((w)->reserved00 + 2)
#define set_substate(w, value) fade_put_half((w)->reserved00 + 2, value)
#define set_state(w, value) fade_put_half((w)->reserved00, value)
#define fade_half(value) fade_read_half((value).le)
#define fade_signed(value) ((s32)(fade_half(value) ^ 0x8000u) - 0x8000)
#define set_fade_half(field, value) fade_put_half((field).le, value)
#endif

#endif
