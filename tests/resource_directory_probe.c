#define _GNU_SOURCE
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include "../src/main/8001971c.c"

_Static_assert(sizeof(DiscFile) == 48 && sizeof(DirectoryRecord) == 8 &&
               sizeof(FileLocation) == 8, "disc directory layouts");
DiscFile D_80062C24[24];
FileLocation D_800AE830[456];
int D_800747D0, D_800747D4, D_800747D8;
static const unsigned sentinel = 0x5a5a5a5a;
static unsigned scenario, special, search, retry, load, waits, group, conversions;
static unsigned expected_conversions, expected_output[456][2];
static unsigned conversion_row[456], conversion_input[456], conversion_sector[456];
static unsigned directory_entries;

void func_80019990(void)
{
    assert(special == 0 && search == 0 && group == 0);
}

static int special_result(void)
{
    if (scenario % 4 == 0) return -1;
    if (scenario % 4 == 1) return 0;
    if (scenario % 4 == 2) return 1;
    return -2;
}

int func_80045374(int *position, char *name)
{
    if (position == &D_80063044.position) {
        assert(name == D_80063044.name && search == 0 && special < 16);
        ++special;
        return special <= scenario % 5 ? -1 : special_result();
    }
    assert(special == (special_result() == -1 ? 16 : scenario % 5 + 1));
    assert(search < 21 && position == &D_80062C24[search].position);
    assert(name == D_80062C24[search].name);
    assert(D_800747D0 == (special_result() != 0) && D_800747D4 == 1);
    assert(D_80063074 == 0 && D_800747D8 == 0);
    if (retry++ < (scenario + search) % 3) return retry == 1 ? -1 : 0;
    *position = (int)(1000 + search);
    ++search;
    retry = 0;
    return search % 2 ? 1 : -2;
}

int func_80019A24(int *file, int buffer, int bytes, int flags)
{
    assert(search == 21 && file == &D_80062C24[0].position);
    assert((uint32_t)buffer == 0x80180000u && bytes == 3648 && flags == 0);
    assert(waits == load && group == 0);
    return ++load > scenario % 4 ? -1 : 0;
}

int func_8004239C(int mode)
{
    assert(mode == 0 && waits + 1 == load);
    ++waits;
    return 17;
}

static unsigned group_base(unsigned index)
{
    return index % 2 ? 3000u + index : 0x7fffffffu - index;
}

int func_80043B1C(int *position)
{
    assert(group < 8 && position == &D_80062C24[group + 1].position);
    assert(*position == (int)(1001 + group));
    return (int)group_base(group++);
}

void func_80043A18(unsigned int sector, int *position)
{
    assert(conversions < expected_conversions);
    const unsigned row = conversion_row[conversions];
    const unsigned input = conversion_input[conversions];
    assert(position == &D_800AE830[row].position);
    assert(sector == conversion_sector[conversions]);
    *position = (int)(sector ^ 0x55aa00ffu);
    /* The loader reads the directory length after conversion returns. */
    RESOURCE_DIRECTORY[input].length += 9;
    ++conversions;
}

static void prepare_directory(void)
{
    unsigned input = 0, output = 0;
    for (unsigned g = 0; g < 8; ++g) {
        int count = (int)((g + scenario) % 6) - 1;
        RESOURCE_DIRECTORY[input++] = (DirectoryRecord){count, 709};
        for (int j = 0; j < count; ++j, ++input, ++output) {
            int offset = (int)((input * 19 + scenario) % 61) - 24;
            if ((output + scenario) % 3 == 0) offset = 0;
            const int length = (int)(500 + 9 * input);
            RESOURCE_DIRECTORY[input] = (DirectoryRecord){offset, length};
            if (offset) {
                const unsigned sector = group_base(g) + (unsigned)offset;
                conversion_row[expected_conversions] = output;
                conversion_input[expected_conversions] = input;
                conversion_sector[expected_conversions++] = sector;
                expected_output[output][0] = sector ^ 0x55aa00ffu;
                expected_output[output][1] = (unsigned)length + 9;
            }
        }
    }
    directory_entries = input;
}

int main(void)
{
    const size_t mapped = 4096;
    void *p = mmap(RESOURCE_DIRECTORY, mapped, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    assert(p == RESOURCE_DIRECTORY);
    for (scenario = 0; scenario < 24; ++scenario) {
        special = search = retry = load = waits = group = conversions = expected_conversions = 0;
        memset(D_80062C24, 0xc3, sizeof D_80062C24);
        memset(D_800AE830, 0x5a, sizeof D_800AE830);
        memset(expected_output, 0x5a, sizeof expected_output);
        memset(p, 0x5a, mapped);
        D_800747D0 = D_800747D4 = D_800747D8 = -1;
        prepare_directory();
        func_8001971C();
        assert(search == 21 && group == 8 && conversions == expected_conversions);
        assert(waits == scenario % 4 && load == waits + 1);
        assert(memcmp(D_800AE830, expected_output, sizeof expected_output) == 0);
        assert((unsigned)RESOURCE_DIRECTORY[directory_entries].length == sentinel);
    }
    assert(munmap(p, mapped) == 0);
    return 0;
}
