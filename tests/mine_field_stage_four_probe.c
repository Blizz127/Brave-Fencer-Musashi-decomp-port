#include <assert.h>
#include <stdint.h>
#include <string.h>

_Alignas(4) unsigned char D_800AF630[0xa3dc];
static unsigned char before[sizeof(D_800AF630)], expected[sizeof(D_800AF630)];
static unsigned visits;

void func_8012853C(void);

void func_80146074(void)
{
    assert(!memcmp(before, D_800AF630, sizeof(before)));
    ++visits;
}

static void store_half(unsigned char *p, uint16_t value)
{
    memcpy(p, &value, sizeof(value));
}

int main(void)
{
    static const uint16_t phases[] = {4, 5, 0, 1, 0x7fff, 0x8000, 0xfffe, 0xffff};
    static const unsigned cleared[] = {0xa3c8, 0xa3cc, 0xa3ce, 0xa3d0};
    for (unsigned i = 0; i < sizeof(phases) / sizeof(phases[0]); ++i) {
        for (unsigned j = 0; j < sizeof(before); ++j)
            before[j] = (unsigned char)(j * 53 + i * 19 + 1);
        store_half(before + 0xa3c6, phases[i]);
        memcpy(D_800AF630, before, sizeof(before));
        memcpy(expected, before, sizeof(expected));
        store_half(expected + 0xa3c6, (uint16_t)(phases[i] + 1u));
        for (unsigned j = 0; j < sizeof(cleared) / sizeof(cleared[0]); ++j)
            store_half(expected + cleared[j], 0);
        func_8012853C();
        assert(visits == i + 1);
        assert(!memcmp(expected, D_800AF630, sizeof(expected)));
    }
    return 0;
}
