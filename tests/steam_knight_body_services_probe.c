#include "steam_knight_hit.h"
#include <assert.h>
#include <string.h>

extern void func_801831A4(SteamKnightPart *body);
extern void func_80183200(SteamKnightPart *body);
extern void func_801858C4(void);
u32 D_801E43C0, D_801E43C4;

static SteamKnightPart body, objects[4];
static u32 actions[16];
static int preparation, update, branch, rewrite, calls[8], n;
static u16 flags;
enum { TABLE = 0x801962c0u, RESET = 0x4321u, OTHER = 0x2875u };
static u32 key(unsigned i) { return 0x70000000u + 268u * i; }

static void record(int kind) { assert(n < 8); calls[n++] = kind; }
void *bfm_tower_spawn_pointer(u32 address)
{
    unsigned offset = address - TABLE;
    assert(!(offset % 4) && offset / 4 < 16);
    return &actions[offset / 4];
}

SteamKnightPart *bfm_steam_knight_break_actor(u32 address)
{
    unsigned offset = address - key(0);
    assert(!(offset % sizeof body) && offset / sizeof body < 4);
    return &objects[offset / sizeof body];
}

void func_80186600(SteamKnightPart *p)
{
    assert(p == &body);
    record(1);
    p->state = (u16)preparation;
}

void func_80183218(SteamKnightPart *p)
{
    assert(p == &body && p->state == (u16)preparation);
    record(2);
    if (update >= 0) p->state = (u16)update;
    actions[p->state] = branch ? RESET : OTHER;
}

void bfm_tower_spawn_setup(u32 target, void *actor)
{
    assert(actor == &body && target == (branch ? RESET : OTHER));
    record(3);
    if (target == RESET) func_80183200(actor);
}

void func_80186DE8(SteamKnightPart *p)
{
    assert(p == &body);
    record(4);
    assert(p->flags == (branch ? (u16)(flags & ~1u) : flags));
    assert(p->hit_kind == (branch ? 0 : 277u));
    assert(p->reaction == (branch ? 0 : 91));
}

s32 func_8012C218(u8 *actor)
{
    int selected = (int)((SteamKnightPart *)actor - objects);
    assert(selected >= 0 && selected < 4);
    record(10 + selected);
    if (rewrite == 1) D_801E43C4 = key(3);
    if (rewrite == 2) D_801E43C4 = 0;
    if (rewrite == 3) D_801E43C0 = key(2);
    return -1;
}

int main(void)
{
    static const int ordinary[] = {1, 2, 3, 4};
    for (preparation = 0; preparation < 16; ++preparation)
        for (update = -1; update < 16; ++update)
            for (branch = 0; branch < 2; ++branch) {
                n = 0;
                memset(actions, 0x56, sizeof actions);
                memset(&body, 0x97, sizeof body);
                flags = (u16)(0x3901u ^ ((unsigned)preparation << 2));
                body.reaction = 91;
                body.hit_kind = 277;
                body.flags = flags;
                func_801831A4(&body);
                assert(n == 4 && !memcmp(calls, ordinary, sizeof ordinary));
            }
    for (int first = -1; first < 4; ++first)
        for (int second = -1; second < 4; ++second)
            for (rewrite = 0; rewrite < 4; ++rewrite) {
                n = 0;
                D_801E43C0 = first < 0 ? 0 : key((unsigned)first);
                D_801E43C4 = second < 0 ? 0 : key((unsigned)second);
                int later = first < 0 ? second : rewrite == 1 ? 3 : rewrite == 2 ? -1 : second;
                func_801858C4();
                assert(n == (first >= 0) + (later >= 0));
                if (first >= 0) assert(calls[0] == 10 + first);
                if (later >= 0) assert(calls[n - 1] == 10 + later);
                assert(!D_801E43C4);
                assert(D_801E43C0 == (rewrite == 3 && later >= 0 ? key(2) : 0));
            }
    return 0;
}
