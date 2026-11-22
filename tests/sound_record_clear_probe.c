#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "musashi_sound_record_clear.h"

enum { pool_offset = 0x1200, handle_offset = 0x1300 };
static const uint32_t base = 0x80000000u;
static MusashiBootMemory memory, expected;

static void setup(const uint32_t counts[4], const uint32_t handles[4])
{
    memset(&memory, 0x6d, sizeof memory);
    for (unsigned i = 0; i < 4; ++i) {
        assert(musashi_boot_write32(&memory, base + pool_offset + 32u + 4u * i, counts[i]));
        assert(musashi_boot_write32(&memory, base + handle_offset + 4u * i, handles[i]));
    }
    memcpy(&expected, &memory, sizeof expected);
}

static void check(uint32_t counts, uint32_t handles, int want)
{
    assert(musashi_sound_clear_record_flags(&memory, counts, handles) == want);
    assert(memcmp(&memory, &expected, sizeof memory) == 0);
}

int main(void)
{
    const uint32_t counts = base + pool_offset + 32u;
    const uint32_t handles = base + handle_offset;
    const uint32_t unmapped = 0xb0000000u;

    /* Negative and empty groups skip their records even with unusable handles. */
    setup((uint32_t[]){UINT32_MAX,0,3,1}, (uint32_t[]){unmapped,unmapped,base+0x600,base+0x700});
    for (unsigned i = 0; i < 3; ++i)
        expected.bytes[0x600 + 12*i + 10] = 0;
    expected.bytes[0x70a] = 0;
    check(counts, handles, 1);

    /* Initial entry at pool+24 clears byte 2 of the aligned count at pool+32.
     * The new bound is one. A stale bound would clear another flag at pool+46. */
    setup((uint32_t[]){0x10001,0,0x80000000u,UINT32_MAX},
          (uint32_t[]){base+pool_offset+24u,0,0,0});
    expected.bytes[pool_offset+34] = 0;
    check(counts, handles, 1);

    /* The first group shortens the second group's count from 65539 to 3. */
    setup((uint32_t[]){1,0x10003,0,0}, (uint32_t[]){counts+4u-8u,base+0x700,0,0});
    expected.bytes[pool_offset+38] = 0;
    for (unsigned i = 0; i < 3; ++i)
        expected.bytes[0x700 + 12*i + 10] = 0;
    check(counts, handles, 1);

    /* The first group's flag changes the next handle from base+634 to base+600. */
    setup((uint32_t[]){1,2,0,0}, (uint32_t[]){handles+4u-10u,base+0x634,0,0});
    expected.bytes[handle_offset+4] = 0;
    expected.bytes[0x60a] = 0;
    expected.bytes[0x616] = 0;
    check(counts, handles, 1);

    setup((uint32_t[]){0,0,0,0}, (uint32_t[]){0,0,0,0});
    assert(!musashi_sound_clear_record_flags(NULL,counts,handles));
    check(counts+2u,handles,0);
    check(unmapped,handles,0);
    check(counts,handles+1u,0);
    check(counts,unmapped,0);
    /* Even a skipped group still reads its handle. The next handle is unmapped. */
    check(counts,base+MUSASHI_RAM_SIZE-4u,0);

    setup((uint32_t[]){1,0,0,0}, (uint32_t[]){unmapped,0,0,0});
    check(counts,handles,0);

    setup((uint32_t[]){2,0,0,0}, (uint32_t[]){base+MUSASHI_RAM_SIZE-12u,0,0,0});
    expected.bytes[MUSASHI_RAM_SIZE-2u] = 0;
    check(counts,handles,0);
    return 0;
}
