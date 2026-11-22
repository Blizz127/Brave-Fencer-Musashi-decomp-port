#include "forest_secondary_event.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

u8 D_800AF648, D_800A4F17, D_800A4F18;
s16 D_800A4EFC;

static u16 position[4];
static u32 screen;
static s32 projection_flag;
static unsigned calls;

void func_8004914C(void *matrix)
{
    assert(matrix == &D_800AF648 && calls == 0);
    ++calls;
}

void func_800491AC(void *matrix)
{
    assert(matrix == &D_800AF648 && calls == 1);
    ++calls;
}

void func_8004955C(void *input, void *projected, void *depth, void *flag)
{
    s32 distance = 12;
    assert(input == position && calls == 2);
    ++calls;
    memcpy(projected, &screen, sizeof(screen));
    memcpy(depth, &distance, sizeof(distance));
    memcpy(flag, &projection_flag, sizeof(projection_flag));
}

void func_8002DC68(int command, int argument)
{
    assert(command == 0xfffe && argument == 0x8073);
    assert(D_800A4F17 == 1 && calls == 3);
    ++calls;
}

/* A high command and an empty pending queue use none of these services. */
void func_8002E138(int command, int argument, int zero)
{
    (void)command;
    (void)argument;
    (void)zero;
    abort();
}
void func_8002D904(int command) { (void)command; abort(); }
void func_80030F80(void) { abort(); }
void func_8002D320(void) { abort(); }
void func_80031BE0(void) { abort(); }

static void check(s16 x, s16 y, s32 flag, unsigned expected_calls)
{
    calls = 0;
    D_800A4F17 = 0;
    screen = ((u32)(u16)y << 16) | (u16)x;
    projection_flag = flag;
    func_8012E5CC(position, 0xfffe, 0x8073);
    assert(calls == expected_calls && D_800A4F17 == 0);
}

int main(void)
{
    check(-199, -169, 0, 4);
    check(199, 169, 0, 4);
    check(-200, 0, 0, 3);
    check(200, 0, 0, 3);
    check(0, -170, 0, 3);
    check(0, 170, 0, 3);
    check(0, 0, -1, 3);
    return 0;
}
