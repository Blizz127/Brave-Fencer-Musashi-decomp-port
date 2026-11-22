#include "mine_fade_lifecycle.h"

#include <assert.h>
#include <string.h>

ScreenWork D_8011DA80[2];
static ScreenEffect effect;
static ScreenEffect *alternate;
static unsigned effect_calls, nested_calls, expect_cleared_target;
static const u32 handle = 0x18624;

static void put_half(u8 p[2], u16 v)
{
    p[0] = (u8)v;
    p[1] = (u8)(v >> 8);
}

static u16 half(const u8 p[2])
{
    return (u16)(p[0] | (unsigned)p[1] << 8);
}

static void address(ScreenWork *slot, u32 v)
{
    unsigned i;
    for (i = 0; i < 4; ++i)
        slot->effect_address.le[i] = (u8)(v >> (8*i));
}

ScreenEffect *bfm_screen_effect_resolve(u32 addr)
{
    if (expect_cleared_target)
        assert(half(D_8011DA80[0].target.le) == 0);
    ++effect_calls;
    return addr == handle ? (alternate ? alternate : &effect) : 0;
}

void bfm_screen_observe_3aa24(ScreenEffect *a, ScreenWork *b, ScreenWork *c, u32 d)
{
    (void)d;
    assert(a == &effect && b == c);
    assert(b == D_8011DA80 || b == D_8011DA80 + 1);
    ++nested_calls;
}

void bfm_screen_observe_15f04(s32 a,s32 b,s32 c,s32 d,u32 e,u32 f,u32 g,u32 h,u32 i)
{
    (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;
    assert(0);
}

void bfm_screen_observe_15d4c(s32 a,s32 b,s32 c,s32 d,u32 e,u32 f,u32 g,u32 h,u32 i)
{
    (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;
    assert(0);
}

static void reset(u16 state, u16 phase, u16 x, u16 y)
{
    memset(D_8011DA80,0,sizeof(D_8011DA80));
    memset(&effect,0,sizeof(effect));
    put_half(D_8011DA80[0].reserved00,state);
    put_half(D_8011DA80[0].reserved00+2,phase);
    address(D_8011DA80,handle);
    put_half(effect.factors.x.le,x);
    put_half(effect.factors.y.le,y);
    effect_calls = nested_calls = expect_cleared_target = 0;
    alternate = 0;
}

int main(void)
{
    unsigned x;
    u16 y, next_x, next_y;
    for (x = 0; x < 65536; ++x) {
        y = (u16)(x ^ 0xb69du);
        reset(1,19,(u16)x,y);
        func_8013A448(D_8011DA80);
        next_x = (u16)(x+512);
        assert(half(effect.factors.x.le) == next_x && half(effect.factors.y.le) == (u16)(y+512));
        assert(half(D_8011DA80[0].reserved00) == (next_x == 4096 ? 2 : 1));
        assert(half(D_8011DA80[0].reserved00+2) == (next_x == 4096 ? 0 : 19));
        assert(effect_calls == 2 && !nested_calls);

        reset(3,27,(u16)x,y);
        put_half(D_8011DA80[0].target.le,42);
        expect_cleared_target = 1;
        func_8013A4C4(D_8011DA80);
        next_x = (u16)(x-1024);
        next_y = (u16)(y-1024);
        if ((next_x & 0x8000u) || next_x <= 1024) {
            assert(!half(effect.factors.x.le) && !half(effect.factors.y.le));
            assert(half(D_8011DA80[0].reserved00) == 4);
            assert(!half(D_8011DA80[0].reserved00+2));
            assert(!memcmp(D_8011DA80[0].effect_address.le,(u8[4]){0},4));
            assert(effect_calls == 1);
        } else {
            assert(half(effect.factors.x.le) == next_x && half(effect.factors.y.le) == next_y);
            assert(half(D_8011DA80[0].reserved00) == 3 && half(D_8011DA80[0].reserved00+2) == 27);
            assert(effect_calls == 2);
        }
    }
    reset(1,0,13,515);
    effect.mode.le[0] = 2;
    put_half(D_8011DA80[0].target.le,3000);
    for (x = 0; x < 7; ++x)
        func_8013A380();
    assert(nested_calls == 7 && half(D_8011DA80[0].reserved00) == 2);
    assert(half(effect.factors.x.le) == 4096 && half(effect.factors.y.le) == 4096);
    assert(half(D_8011DA80[0].level.le) == 3000);
    put_half(D_8011DA80[0].reserved00,3);
    for (x=0;x<3;++x)
        func_8013A380();
    assert(nested_calls == 9 && half(D_8011DA80[0].reserved00) == 4);
    assert(!half(effect.factors.x.le) && !half(effect.factors.y.le));

    /* The first effect's Y factor aliases the slot's phase. Retail reloads the phase. */
    {
        u8 shared[64] = {0};
        ScreenWork *overlap = (ScreenWork *)(void *)(shared+40);
        reset(0,0,0,0);
        alternate = (ScreenEffect *)(void *)shared;
        put_half(overlap->reserved00,1);
        address(overlap,handle);
        func_8013A448(overlap);
        assert(half(overlap->reserved00)==1024 && half(overlap->reserved00+2)==1025);
    }
    {
        u32 bad[] = {0, handle+1u, 0xffffffcau, 0x8888u};
        for (x=0;x<sizeof(bad)/sizeof(*bad);++x) {
            reset(1,17,23,42);
            address(D_8011DA80,bad[x]);
            func_8013A448(D_8011DA80);
            assert(half(D_8011DA80[0].reserved00+2)==17);
            put_half(D_8011DA80[0].target.le,123);
            func_8013A4C4(D_8011DA80);
            assert(!half(D_8011DA80[0].target.le) && !nested_calls);
        }
        func_8013A448(0);
        func_8013A4C4(0);
    }
    return 0;
}
