#include <assert.h>
#include <string.h>
#include "mine_award_controller.h"

extern void func_801741EC(u8 *actor);
extern void func_80174274(u8 *actor);

u16 D_80126B3C;
s16 D_8011F9C8;
static u8 actor[0x218], current[24], replacement[24];
static s32 step;
static unsigned progress;

s32 func_80012B04(s32 from, s32 to, s32 shift)
{
    assert(from == -2 && to == -32765 && shift == 4 && progress == 0);
    progress = 1;
    return step;
}

void func_80171A1C(u8 *p)
{
    assert(p == actor && D_8011F9C8 == 1 && progress == 1);
    progress = 2;
}

void func_80174650(u8 *p)
{
    u8 *model = replacement;
    assert(p == actor && progress == 2);
    progress = 3;
    memcpy(p + 0x20, &model, sizeof model);
}

int main(void)
{
    D_80126B3C = 0x8003;
    for (int finish = 0; finish < 2; ++finish) {
        u8 *model = current;
        memcpy(actor + 0x20, &model, sizeof model);
        current[0x12] = 0xfe;
        current[0x13] = 0xff;
        replacement[0x12] = 0x21;
        replacement[0x13] = 0x43;
        progress = 0;
        D_8011F9C8 = -1;
        step = finish ? 0x10000 : -7;
        func_801741EC(actor);
        if (finish) {
            assert(D_8011F9C8 == 1 && progress == 3);
            assert(current[0x12] == 0xfe && current[0x13] == 0xff);
            assert(replacement[0x12] == 0x21 && replacement[0x13] == 3);
        } else {
            assert(D_8011F9C8 == -1 && progress == 1);
            assert(current[0x12] == 0xf7 && current[0x13] == 0xf);
        }
        unsigned saved = progress;
        func_80174274(actor);
        assert(progress == saved);
    }
    return 0;
}
