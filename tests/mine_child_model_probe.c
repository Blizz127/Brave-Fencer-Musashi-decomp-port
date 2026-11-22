#include "mine_child_model.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

_Static_assert(sizeof(MineChildModelActor) == 0x10c, "field actor size");
_Static_assert(offsetof(MineChildModelActor, model_index) == 0x102, "model record selector");
_Static_assert(offsetof(MineChildModelRecord, feedback_a) == 4, "first feedback field");
_Static_assert(offsetof(MineChildModelRecord, feedback_b) == 6, "second feedback field");

MineChildModelRecord *D_8019B618[3];
u16 D_80126B96, D_80126B98;
static MineChildModelActor child;
static MineChildModelRecord records[2];
static MineChildModelRecord *selected;
static s32 query_result;
static unsigned calls;

void func_8012B2CC(char *actor)
{
    assert(actor == (char *)&child && calls == 0);
    ++calls;
    D_8019B618[child.model_index] = NULL;
    child.model_index = 2;
    selected->feedback_a = 0xa155;
    selected->feedback_b = 0xb266;
}

s32 func_8017EF5C(MineChildModelActor *actor, MineChildModelRecord *record)
{
    assert(actor == &child && actor->model_index == 2);
    assert(record == selected && calls == 1);
    assert(record->feedback_a == 0xa155 && record->feedback_b == 0xb266);
    assert(D_80126B96 == 123 && D_80126B98 == 456);
    ++calls;
    record->feedback_a ^= 0x8080;
    record->feedback_b ^= 0x4001;
    return query_result;
}

static void check(s16 index, s32 result)
{
    MineChildModelActor expected;
    unsigned i;
    memset(&child, 0xa3, sizeof(child));
    child.model_index = index;
    expected = child;
    D_80126B96 = 123;
    D_80126B98 = 456;
    for (i = 0; i < 2; ++i) {
        records[i].feedback_a = (u16)(54 + i);
        records[i].feedback_b = (u16)(221 + i);
        D_8019B618[i] = &records[i];
    }
    D_8019B618[2] = NULL;
    selected = index >= 0 && index < 2 ? &records[index] : NULL;
    calls = 0;
    query_result = result;
    func_8017EED8(&child);
    if (selected) {
        expected.model_index = 2;
        assert(calls == 2 && D_8019B618[index] == NULL);
    } else {
        assert(calls == 0);
    }
    assert(memcmp(&child, &expected, sizeof(child)) == 0);
    if (selected && result) {
        assert(D_80126B96 == (0xa155 ^ 0x8080));
        assert(D_80126B98 == (0xb266 ^ 0x4001));
    } else {
        assert(D_80126B96 == 123 && D_80126B98 == 456);
    }
}

int main(void)
{
    check(0, 0);
    check(0, 1);
    check(1, -1);
    check(2, 1);
    check(-1, 1);
    check(3, 1);
    check(-32768, 1);
    check(32767, 1);
    func_8017EED8(NULL);
    assert(calls == 0 && D_80126B96 == 123 && D_80126B98 == 456);
    return 0;
}
