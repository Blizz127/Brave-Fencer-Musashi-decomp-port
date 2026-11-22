#include <assert.h>
#include <string.h>

#include "mine_award_controller.h"

u8 D_80126B58[0x218];
MineAwardVector D_80126B30;
u16 D_80126B38, D_80126B3C;
s16 D_80126B3A, D_8011F9C8, D_80126938;
u32 D_8012750C;
void *D_80127508;
MineAwardPhase D_8018271C[7];

static u8 model[0x14];
static u8 actor[0x218];
static u8 requested_actor;
static int lookups, installations, phase_calls;
static s32 requested_type;
static s16 requested_id;
static void *lookup_result;
static MineAwardPhase installed;
static MineAwardVector vector = {{1, 6, 0, 5, 3, 5, 8, 9}};

void *func_8014C118(u8 *controller, s32 type, s16 id)
{
    assert(controller == D_80126B58 && type == requested_type && id == requested_id);
    assert(D_80126B38 == 0xa5a5 && D_80126B3A == 107 && D_8012750C == 0x12345678);
    ++lookups;
    return lookup_result;
}

void func_80174684(MineAwardPhase phase)
{
    assert(phase == func_80173B4C);
    assert(D_80127508 == (requested_type ? lookup_result : 0));
    assert(!memcmp(D_80126B30.bytes, vector.bytes, sizeof vector.bytes));
    assert(D_80126B38 == 0xf031 && D_80126B3A == -23 && D_80126B3C == 0xe381);
    assert(D_8011F9C8 == 110 && D_80126938 == -116 && D_8012750C == 0x12345678);
    ++installations;
    installed = phase;
}

static void phase_one(u8 *selected)
{
    assert(selected == actor && selected[0x214] == 1);
    ++phase_calls;
    selected[0x214] = 6;
}

static void phase_six(u8 *selected)
{
    assert(selected == actor && selected[0x214] == 6);
    phase_calls += 2;
}

static void reset_state(void)
{
    D_80126B38 = 0xa5a5;
    D_80126B3A = 107;
    D_80127508 = &requested_actor;
    D_8011F9C8 = 110;
    D_80126938 = -116;
    D_8012750C = 0x12345678;
}

int main(void)
{
    u8 *model_address = model;
    memcpy(D_80126B58 + 0x20, &model_address, sizeof model_address);
    model[0x12] = 0x81;
    model[0x13] = 0xe3;
    D_8018271C[1] = phase_one;
    D_8018271C[6] = phase_six;

    for (int test = 0; test < 4; ++test) {
        reset_state();
        requested_type = test == 0 ? 0 : test == 1 ? 0x10082 : 0x82;
        requested_id = -7;
        lookup_result = test == 2 ? 0 : &requested_actor;
        if (test & 1) {
            func_80173A28(0x1f031, 0x1ffe9, requested_type, 0x1fff9, &vector, 0x4700a5fe);
            assert(D_8012750C == 0x4700a5fe);
        } else {
            func_80173A60(0x1f031, 0x1ffe9, requested_type, 0x1fff9, &vector);
            assert(D_8012750C == 0);
        }
        assert(D_8011F9C8 == 0 && D_80126938 == 0);
    }
    assert(lookups == 3 && installations == 4);
    actor[0x214] = 1;
    installed(actor);
    installed(actor);
    assert(phase_calls == 3);
    return 0;
}
