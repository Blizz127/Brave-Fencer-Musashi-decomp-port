#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "mine_terrain_vector.h"

typedef struct {
    s32 result;
    MineActionPoint destination;
} Reply;

static Reply replies[2];
static MineActionPoint before, after;
static MineActionPoint *expected_from, *expected_to;
static s16 first_mode;
static unsigned call_count;
static int update_source;

static MineActionPoint point(u16 x, u16 y, u16 z, u16 flags)
{
    MineActionPoint p = {0};
    p.x = x;
    p.y = y;
    p.z = z;
    p.surface.flags = flags;
    return p;
}

static int same_point(const MineActionPoint *a, const MineActionPoint *b)
{
    return a->x == b->x && a->y == b->y && a->z == b->z && a->surface.flags == b->surface.flags;
}

s32 func_80133784(s16 mode, MineActionPoint *from, MineActionPoint *to)
{
    assert(call_count < 2);
    if (call_count == 0) {
        assert(mode == first_mode);
        assert(from == expected_from && to == expected_to);
        assert(same_point(to, &before));
        *to = replies[0].destination;
    } else {
        assert(mode == 2);
        assert(from == expected_to);
        assert(to != expected_to && to != expected_from);
        assert(same_point(from, &replies[0].destination));
        if (update_source)
            *from = after;
        /* The probe supplies every halfword of this output-only local point. */
        *to = replies[1].destination;
    }
    return replies[call_count++].result;
}

static void odd_case(s16 mode, s32 result, u16 material, u32 expected)
{
    MineActionPoint source = point(9, 14, 20, 0xace1);
    MineActionPoint destination = point(35, 42, 57, 0xbed4);

    expected_from = &source;
    expected_to = &destination;
    first_mode = mode;
    call_count = 0;
    before = destination;
    replies[0].destination = point(91, 102, 113, material);
    replies[0].result = result;

    assert((u32)func_8012CEB0(&source, &destination, mode) == expected);
    assert(call_count == 1 && same_point(&destination, &replies[0].destination));
    assert(source.x == 9 && source.surface.flags == 0xace1);
}

static void even_case(s16 mode, u32 first, u32 second, u16 action, int mutate,
                      int alias, unsigned calls, int copy_y, u32 expected)
{
    MineActionPoint source = point(6, 11, 13, 0xabcd);
    MineActionPoint destination = point(21, 28, 32, 0xdbee);
    MineActionPoint *from = alias ? &destination : &source;
    MineActionPoint wanted;

    expected_from = from;
    expected_to = &destination;
    first_mode = 0;
    call_count = 0;
    before = destination;
    update_source = mutate;
    after = point(115, 217, 319, 0xb11b);
    replies[0].result = (s32)first;
    replies[0].destination = point(92, 93, 94, action);
    replies[1].result = (s32)second;
    replies[1].destination = point(501, 0x8201, 503, 0xdd02);

    assert((u32)func_8012CEB0(from, &destination, mode) == expected);
    assert(call_count == calls);
    wanted = mutate && calls == 2 ? after : replies[0].destination;
    if (copy_y)
        wanted.y = replies[1].destination.y;
    assert(same_point(&wanted, &destination));
    assert(source.x == 6 && source.surface.flags == 0xabcd);
}

int main(void)
{
    odd_case(1, 0, 0x3c00, 0);
    odd_case(-5, (s32)0x80004000, 0x5402, 0x80005002);
    odd_case(7, 0x2100, 0xab1b, 0x311b);
    odd_case(3, 0, 0x021c, 0x1c);
    odd_case(1, 2, 0x1b01, 3);
    odd_case(1, -1, 0x101b, UINT32_MAX);

    even_case(-4, 0, 0x2000, 0x0a02, 0, 0, 1, 0, 0);
    even_case(2, 1, 0, 0x8c02, 0, 1, 2, 0, 0);
    even_case(0, 0x8000, 0x80008000, 0xbe1b, 1, 0, 2, 0, 0);
    even_case(-2, 0x40000000, 0x4000, 0x9901, 0, 1, 2, 1, 0x4001);
    even_case(4, 1, 0x8000a000, 0x991b, 0, 0, 2, 1, 0x8000301b);
    even_case(0, 0xffff8000, 0x2000, 0x8602, 0, 0, 2, 1, 0xb002);
    even_case(6, 0x82740001, 0x70004040, 0x3301, 1, 1, 2, 1, 0x7000505b);
    even_case(2, 0x8000, 0xfffffff0, 0x8803, 0, 0, 2, 1, 0xfffffff3);
    return 0;
}
