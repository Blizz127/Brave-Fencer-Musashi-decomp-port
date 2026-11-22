#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "mine_model_sequence.h"

u8 D_80126738[16], D_80126838[16], D_80126748[16], D_80126848[16];
static MineSequenceWord seen;
static unsigned flags_calls;
static u8 *seen_dest;
s32 func_801549F8(MineSequenceActor *a) { return a->pc + 5; }
void func_80154B4C(MineSequenceActor *a, MineSequenceWord v) { (void)a; seen = v; }
void func_80154A74(MineSequenceActor *a, s32 i) { (void)a; seen = (u32)i; }
void func_801553C0(MineSequenceActor *a) { (void)a; ++flags_calls; }
void func_80155440(MineSequenceActor *a) { (void)a; ++flags_calls; }
void func_80155394(MineSequenceActor *a) { (void)a; ++flags_calls; }
void func_801553A8(MineSequenceActor *a) { (void)a; ++flags_calls; }
void func_80059A1C(MineSequenceWord v, u8 *d) { seen = v; seen_dest = d; }

int main(void)
{
    _Static_assert(offsetof(MineSequenceActor, model) == 0x20, "model slot");
    _Static_assert(offsetof(MineSequenceActor, timer) == 0xb8, "sequence status");
    _Static_assert(offsetof(MineSequenceActor, dc) == 0xdc, "pose counter");
    _Static_assert(offsetof(MineSequenceModel, pose) == 0x20, "pose pointer");
    MineSequenceActor a = {0};
    MineSequenceModel m = {{0}, 0};
    u8 pose[32];
    MineSequenceWord dest[] = {14, (MineSequenceWord)pose, 7, 21,
        (MineSequenceWord)pose, 17, (MineSequenceWord)pose, 0};
    MineSequenceWord src[] = {5, (MineSequenceWord)dest, 0};
    s32 count = 0, skip = 0;
    u32 mask = 0;
    a.model = &m;
    a.script = src;
    a.timer = 0x8301;
    a.saved = 1;
    func_8015444C(&a, &count, &mask, &skip);
    assert(a.script == dest && a.pc == 7 && a.depth == 3 && a.dc == 1);
    assert(a.timer == 0x4001 && mask == 0x4000 && count == 1);
    assert(seen == (MineSequenceWord)pose && seen_dest == D_80126738 && flags_calls == 1);
    assert(m.pose == (MineSequenceWord)(pose + 16));
    count = 0;
    mask = 0;
    func_8015444C(&a, &count, &mask, &skip);
    assert(a.timer == 0x8000 && a.saved == 0 && a.pc == 0 && a.depth == 0);

    MineSequenceWord held[] = {20, 22, (MineSequenceWord)pose, 24};
    a.script = held;
    a.timer = 1;
    a.saved = 7;
    count = 0;
    mask = 0;
    skip = 1;
    seen = 0;
    seen_dest = NULL;
    func_8015444C(&a, &count, &mask, &skip);
    assert(a.b1A8 == 0 && seen == 0 && seen_dest == NULL);
    assert(a.pc == 4 && a.timer == 7 && m.pose == (MineSequenceWord)(pose + 16));

    MineSequenceWord wrapped[] = {UINT32_MAX - 3};
    a.script = wrapped;
    a.pc = 0;
    a.timer = 1;
    count = 0;
    func_8015444C(&a, &count, &mask, &skip);
    assert(m.pose == 12 && a.pc == 1);

    a.model = NULL;
    count = 0;
    a.timer = 1;
    func_8015444C(&a, &count, &mask, &skip);
    assert(count == 0 && a.timer == 1 && a.pc == 1);
    return 0;
}
