#include "psx_types.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

extern void func_8017DE84(u8 *);
extern void func_8017DEC0(u8 *);
extern s32 func_8012A048(u8 *, u32, u8);
extern void func_800118AC(void);

static void unexpected(u8 *unused)
{
    (void)unused;
    assert(0 && "unexpected event state");
}

void (*D_8018A740[])(u8 *) = {func_8017DEC0, unexpected, unexpected, unexpected, unexpected};
u8 D_800AE648[0x40];
u8 D_800AF630[0xA3C0];
_Alignas(4) u8 camera_storage[0x260];

static u32 word(const u8 *p)
{
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

static u16 half(const u8 *p)
{
    return (u16)((u16)p[0] | ((u16)p[1] << 8));
}

static void set_half(u8 *p, u16 value)
{
    p[0] = (u8)value;
    p[1] = (u8)(value >> 8);
}

static void check_camera_setup(void)
{
    u8 player[0x218], expected_flags[sizeof D_800AE648];
    u8 unaligned[0xA8];
    memset(player, 0x5A, sizeof player);
    player[0x214] = 0;
    memset(camera_storage, 0x5A, sizeof camera_storage);
    set_half(camera_storage + 0x21E, 0x1234);
    set_half(camera_storage + 0x222, 0xBEEF);
    set_half(camera_storage + 0x226, 0xCAFE);
    memset(D_800AE648, 0xFF, sizeof D_800AE648);
    memcpy(expected_flags, D_800AE648, sizeof expected_flags);
    for (unsigned bit = 0x125; bit <= 0x12C; ++bit)
        expected_flags[bit >> 3] &= (u8)~(1u << (bit & 7));
    func_8017DE84(player);
    assert(player[0x214] == 1 && player[0x213] == 0x5A && player[0x215] == 0x5A);
    assert(memcmp(expected_flags, D_800AE648, sizeof expected_flags) == 0);
    assert(word(camera_storage + 8) == 0x80174F28u && camera_storage[12] == 0);
    assert(word(camera_storage + 0x10) == 500 && word(camera_storage + 0x14) == 500);
    assert(word(camera_storage + 0x1C) == 800 && half(camera_storage + 0x28) == 96);
    assert(half(camera_storage + 0x2A) == 2048 && half(camera_storage + 0x2C) == 0);
    assert(half(camera_storage + 0x36) == 24 && half(camera_storage + 0x38) == (u16)-128);
    assert(half(camera_storage + 0x3A) == (u16)-125);
    assert(word(camera_storage + 0x3C) == 0x801B1DACu && half(camera_storage + 0x40) == 0);
    assert(word(camera_storage + 0xA4) == 0 && word(camera_storage + 0xA8) == 0);
    assert(half(camera_storage) == 0x1234 && half(camera_storage + 2) == 0xBEEF);
    assert(half(camera_storage + 4) == 0xCAFE);

    memset(unaligned, 0xA5, sizeof unaligned);
    assert(func_8012A048(unaligned + 1, 0x12345678, 3) == 0x1234);
    assert(word(unaligned + 1) == 0x12345678 && unaligned[5] == 3);
    assert(unaligned[0] == 0xA5 && unaligned[6] == 0xA5 && unaligned[0xA5] == 0xA5);
    assert(word(unaligned + 0x9D) == 0 && word(unaligned + 0xA1) == 0);
}

static void check_scene_counter(void)
{
    u8 *fields = D_800AF630 + 0xA3B4;
    for (unsigned i = 0; i <= UINT16_MAX; ++i) {
        memset(fields, 0xA5, 12);
        set_half(fields, (u16)i);
        func_800118AC();
        assert(half(fields) == (u16)(i + 1));
        assert(half(fields + 2) == 0 && half(fields + 4) == 0xA5A5);
        assert(half(fields + 6) == 0 && half(fields + 8) == 0 && half(fields + 10) == 0);
    }
}

int main(void)
{
    check_camera_setup();
    check_scene_counter();
    return 0;
}
