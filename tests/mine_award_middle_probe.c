#include <assert.h>
#include <string.h>
#include "mine_award_middle.h"

u8 *D_801151D4;
const u8 D_800D5DA8[1] = {0}, D_800D5DB8[1] = {0}, D_800D5DC0[1] = {0};
void *D_80127508;
u16 D_80126B38;
u32 D_8012750C;
static u8 actor[0x218], camera[0x48], pool[16], other[16], handles[2];
static char calls[48];
static unsigned used;
static s32 facing, ready;

static void record(char id)
{
    assert(used + 1 < sizeof calls);
    calls[used++] = id;
}

static void begin(void)
{
    for (unsigned i = 0; i < sizeof calls; ++i) calls[i] = 0;
    used = 0;
}

s32 func_80171AB0(u8 *a, const s16 *p)
{
    assert(a == actor && p[0] == -1 && p[1] == 2 && p[2] == -32768);
    record('F');
    return facing;
}
void func_8014706C(u8 *a) { assert(a == actor); record('U'); }
void func_80154274(u8 *a, const u8 *s)
{
    assert(a == actor);
    record(s == D_800D5DC0 ? 'C' : s == D_800D5DB8 ? 'B' : 'A');
}
void func_801553A8(u8 *a) { assert(a == actor); record('V'); }
void func_8016BADC(u8 *a, s32 low, s32 high)
{
    assert(a == actor && low == -64 && high == 64);
    record('R');
}
void func_80147324(s32 request)
{
    assert((request == 2115 && READ_COUNT(actor) == 32) ||
           (request == 1702 && READ_COUNT(actor) == 128));
    record('S');
}
void *func_8013767C(u32 word)
{
    assert(word == 0xa0014040u);
    record('Q');
    return &handles[0];
}
void func_80171A1C(u8 *a) { assert(a == actor); record('N'); }
void func_801484B0(u8 *first, u8 *second)
{
    assert(second == first + 8);
    first[0] = 31;
    record('T');
}
void func_80149350(u8 *a, u8 *first, u8 *second)
{
    assert(a == actor && first == second && first[0] == 31);
    first[0] += 2;
    D_80127508 = other;
    record('G');
}
void func_80015954(const u8 *source, u8 *dest)
{
    assert(dest == other + 4 && source[0] == 33);
    *dest = source[0];
    record('W');
}
s32 func_801399F0(void *h)
{
    assert(h == &handles[0]);
    WRITE_HANDLE(actor, &handles[1]);
    record('P');
    return ready;
}
void func_80139914(void *h)
{
    assert(h == &handles[1]);
    record('X');
}

int main(void)
{
    D_801151D4 = camera;
    camera[0x3c] = camera[0x3d] = 0xff;
    camera[0x40] = 2;
    camera[0x45] = 0x80;
    D_8012750C = 0xa0014040u;
    const char *expected[] = {"FUASQN","FUARSQN","FUARSQN","FUBVRSQN","FUQN"};
    for (unsigned mode = 0; mode <= 4; ++mode) {
        begin();
        D_80126B38 = (u16)(0x8000 | mode);
        facing = 0x10000;
        func_80173CB4(actor);
        assert(!strcmp(calls, expected[mode]) && READ_HANDLE(actor) == &handles[0]);
    }
    begin();
    facing = -1;
    func_80173CB4(actor);
    assert(!strcmp(calls, "F"));

    D_80127508 = pool;
    ready = 0;
    begin();
    func_80173E1C(actor);
    assert(!strcmp(calls, "TGWP") && READ_HANDLE(actor) == &handles[1]);
    assert(other[4] == 33 && pool[4] == 0);
    begin();
    WRITE_HANDLE(actor, &handles[0]);
    D_80127508 = NULL;
    ready = 1;
    func_80173E1C(actor);
    assert(!strcmp(calls, "PXCN") && !READ_HANDLE(actor));
    WRITE_COUNT(actor, 1);
    begin();
    func_80173E1C(actor);
    assert(!*calls && !READ_COUNT(actor));
    func_80173E1C(actor);
    assert(!strcmp(calls, "CN") && READ_COUNT(actor) == (u32)-1);
    return 0;
}
