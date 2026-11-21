#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "model_draw.h"

_Static_assert(sizeof(DrawArg) == 24, "guest draw request width");
_Static_assert(offsetof(DrawArg, unk8) == 8, "guest model offset");
_Static_assert(sizeof(OtRec) == 20, "ordering-table context stride");
_Static_assert(offsetof(OtRec, unk4) == 4, "ordering-table field offset");

s32 D_80078D88;
s32 D_800A2B78;

static DrawArg *expected_arg;
static OtRec *expected_ot;
static s32 expected_mode;
static unsigned calls[2];

static void record(unsigned branch, DrawArg *arg, OtRec *ot, s32 mode)
{
    assert(arg == expected_arg);
    assert(ot == expected_ot);
    assert(mode == expected_mode);
    assert(branch == ((arg->attr >> 6) & 1u));
    assert((u32)D_80078D88 == arg->attr);
    assert((u32)D_800A2B78 == ((arg->attr >> 28) & 7u));
    ++calls[branch];
    ot->unk4 ^= arg->attr;
}

void func_80024448(DrawArg *arg, OtRec *ot, s32 mode)
{
    record(0, arg, ot, mode);
}

void func_80026D64(DrawArg *arg, OtRec *ot, s32 mode)
{
    record(1, arg, ot, mode);
}

int main(void)
{
    DrawArg args[2] = {0};
    OtRec contexts[3] = {0};
    u32 state = 41;

    for (unsigned i = 0; i < 4096; ++i) {
        DrawArg *arg = &args[i % 2];
        OtRec *ot = &contexts[i % 3];
        state = state * 1664525u + 1013904223u;
        arg->attr = (state & ~64u) | ((i & 1u) << 6);
        u32 initial = ot->unk4;
        s32 mode;
        memcpy(&mode, &state, sizeof(mode));
        expected_arg = arg;
        expected_ot = ot;
        expected_mode = mode;
        func_800243EC(arg, ot, mode);
        assert(ot->unk4 == (initial ^ arg->attr));
    }
    assert(calls[0] == 2048);
    assert(calls[1] == 2048);
    return 0;
}
