#include <assert.h>
#include <stddef.h>
#include <string.h>
#include "mine_backdrop.h"

_Static_assert(sizeof(MineBackdropRecord) == 84, "record stride");
_Static_assert(offsetof(MineBackdropRecord, state) == 0x34, "state offset");
_Static_assert(offsetof(MineBackdropGlobals, template_index) == 0xa3d2, "template index offset");

s16 D_80114EE0;
u8 D_801A0D70;
MineBackdropRecord D_800B9A78[2];
MineBackdropGlobals D_800AF630;
u8 D_8019F94C[32];
u8 D_800A6518[80];
MineBackdropPrimitive mine_backdrop_primitive;
MineBackdropTransform mine_backdrop_transform;

static unsigned calls, selection_indices[2], gradients, rendered, alternate, change;
static s32 geometry[6][4], colors[6][6];

void func_801299C8(s16 selection, s16 index, void *record)
{
    MineBackdropRecord *r = record;
    assert(selection == D_801A0D70 && calls < 2);
    assert(r == &D_800B9A78[0] || r == &D_800B9A78[1]);
    selection_indices[calls++] = (unsigned)index;
    if (change) {
        r->transform[0] = 75;
        r->flag = 1;
        r->state = 0x246;
        D_800AF630.template_index = 3;
        D_800B9A78[1].state = 0x7fff;
    }
}

void func_80015B6C(s32 x, s16 y, s32 w, s16 h,
                    s32 r0, s32 g0, s32 b0, s32 r1, s32 g1, s32 b1)
{
    s32 args[4] = {x, y, w, h};
    s32 rgb[6] = {r0, g0, b0, r1, g1, b1};
    assert(gradients < 6);
    memcpy(geometry[gradients], args, sizeof(args));
    memcpy(colors[gradients], rgb, sizeof(rgb));
    ++gradients;
}

void func_80051044(void *prim, void *template_data, u16 state)
{
    assert(prim == &mine_backdrop_primitive && template_data == D_800A6518 + 20 * D_800AF630.template_index);
    assert(state == D_800B9A78[0].state);
    assert(mine_backdrop_transform.w[0] == D_800B9A78[0].transform[0]);
    assert(mine_backdrop_primitive.w[5] == 0x1f800000);
    ++rendered;
}

void func_8005080C(void *prim, void *template_data, u16 state)
{
    func_80051044(prim, template_data, state);
    ++alternate;
}

int main(void)
{
    unsigned i;
    for (i = 0; i < sizeof(D_8019F94C); ++i)
        D_8019F94C[i] = (u8)(i + 20);
    D_801A0D70 = 240;
    D_800B9A78[0].state = 0x7ffd;
    D_800B9A78[1].state = 0x7ffe;
    D_800B9A78[1].off_y = (u16)-15;
    D_800B9A78[1].height = 258;
    func_8012956C();
    assert(calls == 0 && gradients == 0 && rendered == 0);

    D_80114EE0 = -1;
    func_8012956C();
    assert(calls == 2 && selection_indices[0] == 1 && selection_indices[1] == 2);
    assert(gradients == 4 && rendered == 0);
    assert(geometry[0][0] == -160 && geometry[0][1] == -116 && geometry[0][2] == 320 && geometry[0][3] == 140);
    assert(geometry[1][1] == 24 && geometry[1][3] == 60);
    assert(geometry[2][1] == 84 && geometry[2][3] == 40);
    assert(geometry[3][1] == -15 && geometry[3][3] == 258);
    assert(colors[0][0] == D_8019F94C[4] && colors[0][5] == D_8019F94C[10]);
    assert(colors[3][0] == D_8019F94C[12] && colors[3][5] == D_8019F94C[18]);

    D_800B9A78[0].state = 0x3a;
    D_800B9A78[1].state = 0x7fff;
    D_800B9A78[0].base_x = 65535;
    D_800B9A78[0].off_x = 3;
    D_800B9A78[0].base_y = 12345;
    D_800B9A78[0].off_y = 54321;
    D_800AF630.template_index = 1;
    calls = 0;
    func_8012956C();
    assert(calls == 1 && selection_indices[0] == 3 && rendered == 1 && alternate == 0);
    assert(D_800B9A78[0].x == 2 && D_800B9A78[0].y == (u16)(12345 + 54321));

    calls = 0;
    change = 1;
    D_800B9A78[1].state = 0x7ffd;
    func_8012956C();
    assert(calls == 1 && gradients == 4 && rendered == 2 && alternate == 1);
    assert(mine_backdrop_transform.w[0] == 75 && D_800AF630.template_index == 3);
    return 0;
}
