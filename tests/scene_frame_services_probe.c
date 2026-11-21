#include <assert.h>
#include "psx_types.h"

extern void func_801287B8(void);
u8 D_80126B58[12];
u32 D_801A0D90[3];
u16 D_800B99DA;
u8 D_800B9A64;

enum Step {
    BEGIN, SCENE, OBJECT_A, OBJECT_B, OBJECT_C,
    SERVICE_A, SERVICE_B, SERVICE_C, SERVICE_D, SERVICE_E, SERVICE_F,
    RESIDENT, SERVICE_G, SERVICE_H, BEFORE_OPTION, OPTIONAL_OBJECT,
    AFTER_OPTION, AFTER_UPDATE, OPTIONAL_DISPLAY, END
};
static enum Step next;
static u32 busy_after;
static u16 counter_before;
static u8 display_after;

static void visit(enum Step step)
{
    assert(next == step && D_800B99DA == counter_before);
    ++next;
}
#define STEP(name, step) void name(void) { visit(step); }
STEP(func_80129CF8, BEGIN)
STEP(func_8017849C, SCENE)
STEP(func_80165E90, SERVICE_A)
STEP(func_801627E8, SERVICE_B)
STEP(func_80162B1C, SERVICE_C)
STEP(func_80165CA0, SERVICE_D)
STEP(func_80129010, SERVICE_E)
STEP(func_8013CA14, SERVICE_F)
STEP(func_800190AC, RESIDENT)
STEP(func_8012956C, SERVICE_G)
STEP(func_8016E95C, SERVICE_H)

void func_8014FDF4(u8 *obj)
{
    assert(obj == D_80126B58);
    visit(OBJECT_A);
}
s32 func_801505FC(void *obj)
{
    assert(obj == D_80126B58);
    visit(OBJECT_B);
    return -37;
}
s32 func_801508B4(void *obj)
{
    assert(obj == D_80126B58);
    visit(OBJECT_C);
    return 53;
}
void func_801754A8(void)
{
    visit(BEFORE_OPTION);
    D_801A0D90[0] = busy_after;
    if (busy_after)
        ++next;
}
void func_8013BC7C(u8 *obj)
{
    assert(obj == (u8 *)D_801A0D90 && !busy_after);
    visit(OPTIONAL_OBJECT);
}
STEP(func_8013BCDC, AFTER_OPTION)
void func_801379FC(void)
{
    visit(AFTER_UPDATE);
    D_800B9A64 = display_after;
}
void func_8001212C(void)
{
    assert(next == OPTIONAL_DISPLAY && display_after != 0);
    assert(D_800B99DA == (u16)(counter_before + 1));
    ++next;
}

int main(void)
{
    u32 seed = 157;
    for (unsigned i = 0; i < 4096; ++i) {
        seed = seed * 1664525u + 1013904223u;
        next = BEGIN;
        counter_before = (i % 31 == 0) ? (u16)-1 : (u16)(seed >> 7);
        D_800B99DA = counter_before;
        busy_after = (i & 1) ? seed | 1u : 0;
        display_after = (i & 2) ? (u8)(seed | 1u) : 0;
        D_801A0D90[0] = !busy_after;
        D_800B9A64 = !display_after;
        func_801287B8();
        assert(next == (display_after ? END : OPTIONAL_DISPLAY));
        assert(D_800B99DA == (u16)(counter_before + 1));
    }
    return 0;
}
