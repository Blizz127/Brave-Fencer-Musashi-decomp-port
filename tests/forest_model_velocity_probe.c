#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include "forest_action.h"
#include "forest_ground_orientation.h"

static ForestActionPlayer player;
static u8 model[0x16];
static u8 *matrix_seen;
static s32 *output;
static unsigned call_count;
static jmp_buf missing;

static void set_angle(unsigned offset, s16 value)
{
    memcpy(model + offset, &value, sizeof value);
}

u8 *bfm_forest_orientation_bytes(u32 address, u32 span, enum ForestOrientationRole role)
{
    assert(call_count++ == 0 && span == sizeof model && role == FOREST_ORIENTATION_SUBOBJECT);
    return address == 9 ? model : NULL;
}

_Noreturn void bfm_forest_ground_attachment_missing(u32 address)
{
    assert(address == 0);
    longjmp(missing, 1);
}

void bfm_forest_orientation_identity(u8 *matrix)
{
    assert(call_count++ == 1);
    matrix_seen = matrix;
    memset(matrix, 0, 32);
    player.model = 17;
    set_angle(0x10, -7);
}

void bfm_forest_orientation_rotate(u32 service, u8 *matrix, s32 angle)
{
    static const u32 services[] = {0x800126C4, 0x80012558, 0x800123F0};
    static const s32 angles[] = {-7, -46, 129};
    unsigned axis = call_count++ - 2;
    assert(axis < 3 && matrix == matrix_seen);
    assert(service == services[axis] && angle == angles[axis]);
    matrix[axis] = (u8)(axis + 1);
    if (axis < 2)
        set_angle(0x12 + axis * 2, (s16)angles[axis + 1]);
    else {
        const s32 velocity[] = {-118, 202, 93};
        memcpy((u8 *)&player + 0x24, velocity, sizeof velocity);
    }
}

void func_800484EC(void *matrix, void *input, void *result)
{
    s32 velocity[3];
    unsigned axis;
    assert(call_count++ == 5 && matrix == matrix_seen);
    assert(input == (u8 *)&player + 0x24 && result == output);
    memcpy(velocity, input, sizeof velocity);
    for (axis = 0; axis < 3; ++axis)
        velocity[axis] += matrix_seen[axis];
    memcpy(result, velocity, sizeof velocity);
}

int main(void)
{
    const s32 expected[] = {-117, 204, 96};
    s32 separate[3];
    unsigned alias;
    for (alias = 0; alias < 2; ++alias) {
        memset(&player, 0, sizeof player);
        memset(model, 0, sizeof model);
        player.model = 9;
        output = alias ? (s32 *)((u8 *)&player + 0x24) : separate;
        call_count = 0;
        func_80147DC0(&player, output);
        assert(call_count == 6 && memcmp(output, expected, sizeof expected) == 0);
    }
    player.model = 0;
    call_count = 0;
    if (setjmp(missing) == 0) {
        func_80147DC0(&player, separate);
        abort();
    }
    assert(call_count == 1);
    return 0;
}
