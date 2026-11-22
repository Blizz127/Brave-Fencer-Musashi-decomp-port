#include "psx_types.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum { STATE = 0x110, TABLE = 0x160, OTHER = 0x300, FRAME = 0x400 };
static _Alignas(16) unsigned char image[0xd000];
static unsigned calls;
static unsigned mutate;
static u32 seen[5];

void func_8013240C(void *context);
void *bfm_tower_animation_pointer(u32 address)
{
    unsigned offset = address & 0x1fffffffu;
    assert(offset < sizeof image);
    return image + offset;
}
static void half(unsigned offset, u16 value) { memcpy(image + offset, &value, 2); }
static void word(unsigned offset, u32 value) { memcpy(image + offset, &value, 4); }
static u16 read_half(unsigned offset)
{
    u16 value;
    memcpy(&value, image + offset, sizeof value);
    return value;
}
static u32 read_word(unsigned offset)
{
    u32 value;
    memcpy(&value, image + offset, sizeof value);
    return value;
}

void func_801325B8(u32 model, u32 first, u32 second, u32 third, s32 weight)
{
    ++calls;
    seen[0] = model;
    seen[1] = first;
    seen[2] = second;
    seen[3] = third;
    seen[4] = (u32)weight;
    assert(!(read_word(STATE + 12) & 0xc000u));
    if (mutate) {
        /* The caller keeps its current row, then consults the new table and cursor. */
        word(STATE, OTHER);
        half(STATE + 4, 1);
        half(STATE + 6, 22);
        half(TABLE + 4, 25);
        half(TABLE + 6, (u16)-3);
        word(STATE + 12, 0xaa010005u);
    }
}
static void prepare(int time, int duration, int step, u32 payload)
{
    memset(image, 0, sizeof image);
    word(STATE, TABLE);
    half(STATE + 6, (u16)time);
    word(STATE + 8, 0xa12378c0u);
    word(STATE + 12, 0x11aec00au);
    word(TABLE, payload);
    half(TABLE + 4, (u16)duration);
    half(TABLE + 6, (u16)step);
    word(FRAME, 0x1234abcdu);
    word(FRAME + 4, 0x99887766u);
    word(FRAME + 8, 1);
    calls = 0;
    mutate = 0;
}

int main(int argc, char **argv)
{
    assert((uintptr_t)image > UINT32_MAX);
    if (argc > 1) {
        prepare(-7, 0, argv[1][0] == '-' ? -1 : 1, FRAME);
        func_8013240C(image + STATE);
        return 1;
    }
    prepare(3, 10, -2, FRAME);
    func_8013240C(image + STATE);
    assert(calls == 1 && seen[0] == 0xa12378c0u && seen[1] == 0x1234abcdu);
    assert(seen[2] == 0x99887766u && seen[3] == 1 && seen[4] == 2868);
    assert(read_half(STATE + 6) == 5 && read_word(STATE + 12) == 0x11ae000au);

    prepare(3, 10, 2, FRAME);
    mutate = 1;
    word(OTHER + 16, 0xffffffffu);
    func_8013240C(image + STATE);
    assert(seen[4] == 1228 && read_half(STATE + 4) == 2 && read_half(STATE + 6) == 0);
    assert(read_word(STATE + 12) == 0xaa014005u);

    prepare(32767, 123, 1, FRAME);
    func_8013240C(image + STATE);
    assert(read_half(STATE + 6) == 0x8000 && read_half(STATE + 4) == 0);

    prepare(-8, 0, 0, 0xffffffffu);
    func_8013240C(image + STATE);
    assert(calls == 0 && read_half(STATE + 6) == (u16)-8);
    assert(read_word(STATE + 12) == 0x11ae400au);

    prepare(1, 1, 1, 0);
    word(0, 11);
    word(4, 22);
    word(8, 33);
    func_8013240C(image + STATE);
    assert(calls == 1 && seen[1] == 11 && seen[2] == 22 && seen[3] == 33);
    assert(read_half(STATE + 4) == 0 && read_word(STATE + 12) == 0x11ae800au);

    /* The descriptor is fetched before state clears the marker at the same address. */
    prepare(3, 7, 1, FRAME);
    word(STATE, STATE + 12);
    word(STATE + 12, 0xa000c400u);
    word(0xc400, 0xcafed00du);
    half(STATE + 16, 7);
    half(STATE + 18, 1);
    func_8013240C(image + STATE);
    assert(calls == 1 && seen[1] == 0xcafed00du && read_half(STATE + 6) == 4);
    return 0;
}
