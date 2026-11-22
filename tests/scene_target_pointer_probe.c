#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "scene_object_probes.h"

extern s32 func_801508B4(void *obj);
Obj8014A2E4 D_801202A0[96];

static char actor[32];
static int distances[96];
static unsigned comparisons, expected_comparisons, services;
static Obj8014A2E4 *expected;

int func_80013328(char *origin, char *candidate)
{
    assert(origin == actor + 4);
    for (unsigned i = 0; i < 96; ++i) {
        if (candidate == (char *)&D_801202A0[i] + 4) {
            ++comparisons;
            return distances[i];
        }
    }
    assert(0);
    return 0;
}

void func_8014A380(char *obj, void *target)
{
    assert(obj == actor && expected && target == expected);
    ++services;
}

int main(void)
{
    if (sizeof(void *) > 4 && (uintptr_t)D_801202A0 <= UINT32_MAX)
        return 77;
    for (unsigned run = 0; run < 384; ++run) {
        memset(D_801202A0, 0, sizeof D_801202A0);
        expected = 0;
        services = comparisons = expected_comparisons = 0;
        for (unsigned i = 0; i < 96; ++i) {
            Obj8014A2E4 *slot = &D_801202A0[i];
            slot->active = 1;
            slot->flags = 1u << 15;
            slot->hp = (i & 1) ? -1 : 1;
            distances[i] = 48;
            if (i == run % 96) {
                switch (run / 96) {
                case 0: distances[i] = 47; break;
                case 1: slot->active = 0; break;
                case 2: slot->flags = 0; break;
                default: slot->hp = 0; break;
                }
            }
            if (!expected && slot->active && slot->flags && slot->hp) {
                ++expected_comparisons;
                if (distances[i] < 48)
                    expected = slot;
            }
        }
        assert(func_801508B4(actor) == (expected != 0));
        assert(services == (expected != 0) && comparisons == expected_comparisons);
    }
    return 0;
}
