#include "mine_hud_sprite.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

MineFrameSlot D_80115218[4];
u8 D_801824A8[] = {6, 9, 12, 15};
static u8 templates[4][16];
u8 *D_80182498[] = {templates[0], templates[1], templates[2], templates[3]};
u8 D_801824B0[] = {3, 2, 4, 1};
u16 D_801824B4[] = {10, 20, 30, 40};

static unsigned category, value, clears, stage, count, mutate;
static MineFrameSlot *current;
static MineHudSprite seen[8];

static void put(u8 *dst, u16 v)
{
    dst[0] = (u8)v;
    dst[1] = (u8)(v >> 8);
}

void func_80016714(s8 *p, s32 bytes)
{
    assert((void *)p == (void *)D_80115218 && bytes == 128);
    ++clears;
    memset(p, 0, (size_t)bytes);
}

s32 func_800D2CA8(s32 v, s32 shift)
{
    assert(stage++ == 0 && v == (s32)value && shift == 28);
    return v + 40;
}

void func_800D2D10(s32 packed, s32 digits, u8 *out, s32 flags)
{
    assert(stage++ == 1 && packed == (s32)value + 40 && digits == 1 && !flags);
    assert(out == templates[category] + D_801824B0[category]);
    *out = 0x42;
}

s16 func_80024054(u8 *p, s16 *result)
{
    u8 *out = (u8 *)result;
    assert(stage++ == 2 && p == templates[category] && p[D_801824B0[category]] == 0x42);
    assert(out == current->reserved + 6);
    put(out, 0x1835);
    put(out + 2, 0x1858);
    put(out + 4, 0x3872);
    put(out + 6, 0x1850);
    put(out + 8, 0x2346);
    put(out + 10, 0xffff);
    return 5;
}

void func_80017B98(MineHudSprite *s)
{
    assert(count < 8 && s->depth == 16);
    seen[count++] = *s;
    if (mutate && count == 1) {
        put(current->reserved + 2, 300);
        put(current->reserved + 4, 400);
        put(current->reserved + 8, 0x2345);
        s->palette = 63;
    }
}

int main(void)
{
    memset(D_80115218, 0x6d, sizeof D_80115218);
    func_8016E8F0();
    assert(clears == 1);
    for (unsigned i = 0; i < sizeof D_80115218; ++i) assert(!((u8 *)D_80115218)[i]);
    for (category = 0; category < 4; ++category) {
        value = 251 - category;
        current = &D_80115218[3 - category];
        current->active = (u8)(category + 1);
        current->reserved[0] = 31;
        current->reserved[1] = (u8)value;
        current->handler = 7;
        count = stage = mutate = 0;
        func_8016E9EC(current);
        assert(stage == 3 && current->handler == 8 && !current->reserved[0]);
        assert(current->reserved[1] == category && count == 5);
        assert(seen[0].x == (u16)-199 && seen[0].y == D_801824B4[category]);
        assert(seen[0].width == 40 && seen[0].u == 1600 && seen[0].v == 456);
        for (unsigned i = 0; i < count; ++i) {
            assert(seen[i].palette == D_801824A8[category] && seen[i].height == 8);
            assert(seen[i].r == 128 && seen[i].g == 128 && seen[i].b == 128 && !seen[i].flags);
        }
        assert(seen[1].x == (u16)-215 && seen[1].y == seen[0].y + 10);
        assert(seen[2].x == (u16)-203 && seen[2].y == seen[0].y + 3);
        assert(seen[3].x == (u16)-201 && seen[4].x == (u16)-205);
        assert(seen[4].u == 1280 + 0x46 && seen[4].v == 256 + 0x23);
        count = 0;
        mutate = 1;
        func_8016EC0C(current, 0x123);
        assert(count == 5 && seen[0].palette == D_801824A8[category]);
        assert(seen[1].palette == 63 && seen[0].r == 0x23);
        assert(seen[2].x == (u16)-207 && seen[2].y == seen[0].y + 10);
        assert(seen[2].u == 1280 + 0x45 && seen[2].v == 256 + 0x23);
        assert(seen[4].x == (u16)-197);
    }
    return 0;
}
