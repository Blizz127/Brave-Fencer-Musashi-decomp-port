#include "mine_pose_points.h"
#include "mine_model_sequence.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

u8 D_80126B58[576];

static void check(MineSequenceModel *model, u8 *storage, unsigned step)
{
    u8 first[10], second[10], expected[2][10];
    for (unsigned k = 0; k < 32; ++k)
        storage[k] = (u8)(step * 17 + k * 31);
    memset(first, 0xb1, sizeof first);
    memset(second, 0x7c, sizeof second);
    memcpy(expected[0], first, sizeof first);
    memcpy(expected[1], second, sizeof second);
    for (unsigned k = 0; k < 6; ++k) {
        u8 *point = expected[k / 3] + 1 + 2 * (k % 3);
        point[0] = storage[k];
        point[1] = (u8)(storage[k] >= 128 ? 255 : 0);
    }
    model->pose = (uintptr_t)(storage + 16);
    memcpy(D_80126B58 + 0x20, &model, sizeof model);
    func_801484B0(first + 1, second + 1);
    assert(!memcmp(first, expected[0], sizeof first));
    assert(!memcmp(second, expected[1], sizeof second));
}

int main(void)
{
    MineSequenceModel models[2] = {0};
    u8 frames[3][32];
    for (unsigned i = 0; i < 256; ++i)
        check(&models[i % 2], frames[i % 3], i);
    return 0;
}
